

#include "key_manager.h"
#include "buttons_board.h"
#include "event_bus.h"
#include <string.h>
#include "led_effects_manager.h"
#include "led_effects_manager.h"
#define MAX_CONTEXT_STACK_DEPTH 4
#define KEY_THREAD_STACK_SIZE   4096
#define KEY_THREAD_PRIORITY     10
#define KEY_MSG_QUEUE_SIZE      16


typedef enum {
    KEY_MSG_BUTTON_EVENT,       // 按键事件
    KEY_MSG_ACTIVATE_CONTEXT,   // 激活上下文
    KEY_MSG_DEACTIVATE_CONTEXT, // 停用上下文
    KEY_MSG_ENABLE_LED_FEEDBACK,// 启用/禁用LED反馈
    KEY_MSG_SHUTDOWN            // 关闭
} key_msg_type_t;


typedef struct {
    key_msg_type_t type;
    union {
        struct {
            int key_idx;
            button_action_t action;
        } button_event;
        struct {
            key_context_id_t ctx_id;
        } context_op;
        struct {
            bool enable;
        } led_feedback;
    } data;
} key_message_t;

typedef struct {
    key_context_config_t config;
    bool registered;
    bool active;
} context_info_t;

static struct {
    context_info_t contexts[KEY_CTX_MAX];
    key_context_id_t current_ctx;
    key_context_id_t context_stack[MAX_CONTEXT_STACK_DEPTH];
    int stack_top;
    bool led_feedback_enabled;
    

    rt_thread_t key_thread;
    rt_mq_t key_msg_queue;
    rt_sem_t shutdown_sem;
    
    bool initialized;
    bool running;
} g_key_mgr = {0};


static void key_thread_entry(void *parameter);
static void key_process_message(const key_message_t *msg);
static int key_send_message(const key_message_t *msg);
static void key_handle_led_feedback(int key_idx, button_action_t action);
static void key_isr_callback(int32_t pin, button_action_t action);


static void key_isr_callback(int32_t pin, button_action_t action)
{
    int key_idx = buttons_board_pin_to_idx(pin);
    if (key_idx < 0) {
        return;
    }


    key_message_t msg = {
        .type = KEY_MSG_BUTTON_EVENT,
        .data.button_event = {
            .key_idx = key_idx,
            .action = action
        }
    };


    rt_mq_send(g_key_mgr.key_msg_queue, &msg, sizeof(msg));
}


static void key_thread_entry(void *parameter)
{
    (void)parameter;
    key_message_t msg;
    rt_err_t result;
    
    while (g_key_mgr.running) {
        result = rt_mq_recv(g_key_mgr.key_msg_queue, &msg, sizeof(msg), 100);
        
        if (result == RT_EOK) {
            key_process_message(&msg);
        } else if (result == -RT_ETIMEOUT) {
            continue;
        } else {
            rt_thread_mdelay(10);
        }
    }
    rt_sem_release(g_key_mgr.shutdown_sem);
}


static void key_process_message(const key_message_t *msg)
{
    switch (msg->type) {
        case KEY_MSG_BUTTON_EVENT:
            {
                int key_idx = msg->data.button_event.key_idx;
                button_action_t action = msg->data.button_event.action;
                

                if (g_key_mgr.current_ctx != KEY_CTX_NONE && 
                    g_key_mgr.current_ctx < KEY_CTX_MAX) {
                    
                    context_info_t *ctx = &g_key_mgr.contexts[g_key_mgr.current_ctx];
                    if (ctx->registered && ctx->active && ctx->config.handler) {
                        int ret = ctx->config.handler(key_idx, action, ctx->config.user_data);
                    }
            }
            break;
            
        case KEY_MSG_ACTIVATE_CONTEXT:
            {
                key_context_id_t ctx_id = msg->data.context_op.ctx_id;
                

                if (g_key_mgr.current_ctx != KEY_CTX_NONE && 
                    g_key_mgr.current_ctx < KEY_CTX_MAX) {
                    g_key_mgr.contexts[g_key_mgr.current_ctx].active = false;
                }
                

                if (ctx_id != KEY_CTX_NONE) {
                    if (g_key_mgr.contexts[ctx_id].registered) {
                        g_key_mgr.contexts[ctx_id].active = true;
                        g_key_mgr.current_ctx = ctx_id;
                    } 
                } else {
                    g_key_mgr.current_ctx = KEY_CTX_NONE;
                }
            }
            break;
            
        case KEY_MSG_DEACTIVATE_CONTEXT:
            {
                key_context_id_t ctx_id = msg->data.context_op.ctx_id;
                
                if (g_key_mgr.current_ctx == ctx_id) {
                    if (ctx_id != KEY_CTX_NONE) {
                        g_key_mgr.contexts[ctx_id].active = false;
                    }
                    g_key_mgr.current_ctx = KEY_CTX_NONE;
                }
            }
            break;
            
        case KEY_MSG_ENABLE_LED_FEEDBACK:
            g_key_mgr.led_feedback_enabled = msg->data.led_feedback.enable;
            break;
            
        case KEY_MSG_SHUTDOWN:
            g_key_mgr.running = false;
            break;
            
        default:
            break;
    }
}
}
static int key_send_message(const key_message_t *msg)
{
    if (!g_key_mgr.key_msg_queue) {
        return -RT_ERROR;
    }
    
    rt_err_t result = rt_mq_send(g_key_mgr.key_msg_queue, (void*)msg, sizeof(*msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}


int key_manager_init(void)
{
    if (g_key_mgr.initialized) {
        return 0;
    }

    memset(&g_key_mgr, 0, sizeof(g_key_mgr));
    g_key_mgr.current_ctx = KEY_CTX_NONE;
    g_key_mgr.stack_top = -1;
    g_key_mgr.led_feedback_enabled = true;
    g_key_mgr.running = true;


    g_key_mgr.key_msg_queue = rt_mq_create("key_mq", 
                                          sizeof(key_message_t), 
                                          KEY_MSG_QUEUE_SIZE, 
                                          RT_IPC_FLAG_PRIO);
    if (!g_key_mgr.key_msg_queue) {
        return -RT_ENOMEM;
    }


    g_key_mgr.shutdown_sem = rt_sem_create("key_shutdown", 0, RT_IPC_FLAG_PRIO);
    if (!g_key_mgr.shutdown_sem) {
        rt_mq_delete(g_key_mgr.key_msg_queue);
        return -RT_ENOMEM;
    }


    g_key_mgr.key_thread = rt_thread_create("key_mgr",
                                           key_thread_entry,
                                           RT_NULL,
                                           KEY_THREAD_STACK_SIZE,
                                           KEY_THREAD_PRIORITY,
                                           10);
    if (!g_key_mgr.key_thread) {
        rt_sem_delete(g_key_mgr.shutdown_sem);
        rt_mq_delete(g_key_mgr.key_msg_queue);
        return -RT_ENOMEM;
    }


    if (buttons_board_init(key_isr_callback) != RT_EOK) {
        rt_sem_delete(g_key_mgr.shutdown_sem);
        rt_mq_delete(g_key_mgr.key_msg_queue);
        return -RT_ERROR;
    }


    rt_thread_startup(g_key_mgr.key_thread);

    g_key_mgr.initialized = true;
    return 0;
}


int key_manager_deinit(void)
{
    if (!g_key_mgr.initialized) {
        return 0;
    }


    key_message_t shutdown_msg = {.type = KEY_MSG_SHUTDOWN};
    key_send_message(&shutdown_msg);


    rt_sem_take(g_key_mgr.shutdown_sem, 5000);


    buttons_board_deinit();


    if (g_key_mgr.shutdown_sem) {
        rt_sem_delete(g_key_mgr.shutdown_sem);
        g_key_mgr.shutdown_sem = RT_NULL;
    }

    if (g_key_mgr.key_msg_queue) {
        rt_mq_delete(g_key_mgr.key_msg_queue);
        g_key_mgr.key_msg_queue = RT_NULL;
    }

    memset(&g_key_mgr, 0, sizeof(g_key_mgr));
    return 0;
}


int key_manager_register_context(const key_context_config_t *config)
{
    if (!config || config->id >= KEY_CTX_MAX || config->id == KEY_CTX_NONE) {
        return -RT_EINVAL;
    }

    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    if (g_key_mgr.contexts[config->id].registered) {
        return -RT_EBUSY;
    }

    g_key_mgr.contexts[config->id].config = *config;
    g_key_mgr.contexts[config->id].registered = true;
    g_key_mgr.contexts[config->id].active = false;
    return 0;
}


int key_manager_unregister_context(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX || ctx_id == KEY_CTX_NONE) {
        return -RT_EINVAL;
    }

    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    if (!g_key_mgr.contexts[ctx_id].registered) {
        return -RT_ERROR;
    }

    if (g_key_mgr.current_ctx == ctx_id) {
        key_message_t msg = {
            .type = KEY_MSG_DEACTIVATE_CONTEXT,
            .data.context_op.ctx_id = ctx_id
        };
        key_send_message(&msg);
    }

    memset(&g_key_mgr.contexts[ctx_id], 0, sizeof(context_info_t));
    return 0;
}


int key_manager_activate_context(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX) {
        return -RT_EINVAL;
    }

    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    key_message_t msg = {
        .type = KEY_MSG_ACTIVATE_CONTEXT,
        .data.context_op.ctx_id = ctx_id
    };

    return key_send_message(&msg);
}


int key_manager_deactivate_context(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX) {
        return -RT_EINVAL;
    }

    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    key_message_t msg = {
        .type = KEY_MSG_DEACTIVATE_CONTEXT,
        .data.context_op.ctx_id = ctx_id
    };

    return key_send_message(&msg);
}


key_context_id_t key_manager_get_active_context(void)
{
    return g_key_mgr.current_ctx;
}


int key_manager_enable_led_feedback(bool enable)
{
    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    key_message_t msg = {
        .type = KEY_MSG_ENABLE_LED_FEEDBACK,
        .data.led_feedback.enable = enable
    };

    return key_send_message(&msg);
}


const char* key_manager_get_context_name(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX) {
        return "INVALID";
    }
    
    if (ctx_id == KEY_CTX_NONE) {
        return "NONE";
    }

    if (g_key_mgr.contexts[ctx_id].registered) {
        return g_key_mgr.contexts[ctx_id].config.name;
    }
    
    return "UNREGISTERED";
}


bool key_manager_is_led_feedback_enabled(void)
{
    return g_key_mgr.led_feedback_enabled;
}


int key_manager_push_context(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX || !g_key_mgr.initialized) {
        return -RT_EINVAL;
    }

    if (g_key_mgr.stack_top >= MAX_CONTEXT_STACK_DEPTH - 1) {
        rt_kprintf("[key_mgr] Context stack overflow\n");
        return -RT_EFULL;
    }

    g_key_mgr.context_stack[++g_key_mgr.stack_top] = g_key_mgr.current_ctx;
    
    return key_manager_activate_context(ctx_id);
}

int key_manager_pop_context(void)
{
    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    if (g_key_mgr.stack_top < 0) {
        rt_kprintf("[key_mgr] Context stack is empty\n");
        return -RT_EEMPTY;
    }

    key_context_id_t prev_ctx = g_key_mgr.context_stack[g_key_mgr.stack_top--];
    
    return key_manager_activate_context(prev_ctx);
}