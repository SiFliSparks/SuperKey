// key_manager.c - 重新设计版本

#include "key_manager.h"
#include "buttons_board.h"
#include "event_bus.h"
#include <string.h>
#include "led_compat.h"
#include "led_effects_manager.h"
#define MAX_CONTEXT_STACK_DEPTH 4
#define KEY_THREAD_STACK_SIZE   4096
#define KEY_THREAD_PRIORITY     10
#define KEY_MSG_QUEUE_SIZE      16

/* 按键消息类型 */
typedef enum {
    KEY_MSG_BUTTON_EVENT,       // 按键事件
    KEY_MSG_ACTIVATE_CONTEXT,   // 激活上下文
    KEY_MSG_DEACTIVATE_CONTEXT, // 停用上下文
    KEY_MSG_ENABLE_LED_FEEDBACK,// 启用/禁用LED反馈
    KEY_MSG_SHUTDOWN            // 关闭
} key_msg_type_t;

/* 按键消息结构体 */
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
    
    // 线程和通信
    rt_thread_t key_thread;
    rt_mq_t key_msg_queue;
    rt_sem_t shutdown_sem;
    
    bool initialized;
    bool running;
} g_key_mgr = {0};

/* 前向声明 */
static void key_thread_entry(void *parameter);
static void key_process_message(const key_message_t *msg);
static int key_send_message(const key_message_t *msg);
static void key_handle_led_feedback(int key_idx, button_action_t action);
static void key_isr_callback(int32_t pin, button_action_t action);

/* 中断服务程序回调 - 只发送消息到队列 */
static void key_isr_callback(int32_t pin, button_action_t action)
{
    int key_idx = buttons_board_pin_to_idx(pin);
    if (key_idx < 0) {
        return;
    }

    // 在中断上下文中只发送消息，不执行复杂操作
    key_message_t msg = {
        .type = KEY_MSG_BUTTON_EVENT,
        .data.button_event = {
            .key_idx = key_idx,
            .action = action
        }
    };

    // 使用非阻塞发送，如果队列满了就丢弃
    rt_mq_send(g_key_mgr.key_msg_queue, &msg, sizeof(msg));
}

/* 按键处理线程 */
static void key_thread_entry(void *parameter)
{
    (void)parameter;
    key_message_t msg;
    rt_err_t result;
    
    rt_kprintf("[Key_Thread] Key manager thread started\n");
    
    while (g_key_mgr.running) {
        result = rt_mq_recv(g_key_mgr.key_msg_queue, &msg, sizeof(msg), 100);
        
        if (result == RT_EOK) {
            key_process_message(&msg);
        } else if (result == -RT_ETIMEOUT) {
            continue;
        } else {
            rt_kprintf("[Key_Thread] Message queue error: %d\n", result);
            rt_thread_mdelay(10);
        }
    }
    
    rt_kprintf("[Key_Thread] Key manager thread stopped\n");
    rt_sem_release(g_key_mgr.shutdown_sem);
}

/* 处理按键消息 - 在线程上下文中执行 */
static void key_process_message(const key_message_t *msg)
{
    switch (msg->type) {
        case KEY_MSG_BUTTON_EVENT:
            {
                int key_idx = msg->data.button_event.key_idx;
                button_action_t action = msg->data.button_event.action;
                
                // 处理按键事件
                if (g_key_mgr.current_ctx != KEY_CTX_NONE && 
                    g_key_mgr.current_ctx < KEY_CTX_MAX) {
                    
                    context_info_t *ctx = &g_key_mgr.contexts[g_key_mgr.current_ctx];
                    if (ctx->registered && ctx->active && ctx->config.handler) {
                        int ret = ctx->config.handler(key_idx, action, ctx->config.user_data);
                        
                        if (ret != 0) {
                            rt_kprintf("[Key_Thread] Context %s didn't handle key %d, action %d\n", 
                                      ctx->config.name, key_idx, action);
                        }
                    }
                } else {
                    rt_kprintf("[Key_Thread] No active context, key %d ignored\n", key_idx);
                }
            }
            break;
            
        case KEY_MSG_ACTIVATE_CONTEXT:
            {
                key_context_id_t ctx_id = msg->data.context_op.ctx_id;
                
                // 停用当前上下文
                if (g_key_mgr.current_ctx != KEY_CTX_NONE && 
                    g_key_mgr.current_ctx < KEY_CTX_MAX) {
                    g_key_mgr.contexts[g_key_mgr.current_ctx].active = false;
                }
                
                // 激活新上下文
                if (ctx_id != KEY_CTX_NONE) {
                    if (g_key_mgr.contexts[ctx_id].registered) {
                        g_key_mgr.contexts[ctx_id].active = true;
                        g_key_mgr.current_ctx = ctx_id;
                        rt_kprintf("[Key_Thread] Activated context: %s\n", 
                                  g_key_mgr.contexts[ctx_id].config.name);
                    } else {
                        rt_kprintf("[Key_Thread] Context %d not registered\n", ctx_id);
                    }
                } else {
                    g_key_mgr.current_ctx = KEY_CTX_NONE;
                    rt_kprintf("[Key_Thread] Activated NONE context\n");
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
                    rt_kprintf("[Key_Thread] Deactivated context ID=%d\n", ctx_id);
                }
            }
            break;
            
        case KEY_MSG_ENABLE_LED_FEEDBACK:
            g_key_mgr.led_feedback_enabled = msg->data.led_feedback.enable;
            rt_kprintf("[Key_Thread] LED feedback %s\n", 
                      msg->data.led_feedback.enable ? "enabled" : "disabled");
            break;
            
        case KEY_MSG_SHUTDOWN:
            g_key_mgr.running = false;
            break;
            
        default:
            rt_kprintf("[Key_Thread] Unknown message type: %d\n", msg->type);
            break;
    }
}




/* 发送按键消息 */
static int key_send_message(const key_message_t *msg)
{
    if (!g_key_mgr.key_msg_queue) {
        return -RT_ERROR;
    }
    
    rt_err_t result = rt_mq_send(g_key_mgr.key_msg_queue, (void*)msg, sizeof(*msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

/* 按键管理器初始化 */
int key_manager_init(void)
{
    if (g_key_mgr.initialized) {
        rt_kprintf("[key_mgr] Already initialized\n");
        return 0;
    }

    rt_kprintf("[key_mgr] Initializing key manager (thread-based)...\n");

    memset(&g_key_mgr, 0, sizeof(g_key_mgr));
    g_key_mgr.current_ctx = KEY_CTX_NONE;
    g_key_mgr.stack_top = -1;
    g_key_mgr.led_feedback_enabled = true;
    g_key_mgr.running = true;

    // 1. 创建消息队列
    g_key_mgr.key_msg_queue = rt_mq_create("key_mq", 
                                          sizeof(key_message_t), 
                                          KEY_MSG_QUEUE_SIZE, 
                                          RT_IPC_FLAG_PRIO);
    if (!g_key_mgr.key_msg_queue) {
        rt_kprintf("[key_mgr] Failed to create message queue\n");
        return -RT_ENOMEM;
    }

    // 2. 创建关闭信号量
    g_key_mgr.shutdown_sem = rt_sem_create("key_shutdown", 0, RT_IPC_FLAG_PRIO);
    if (!g_key_mgr.shutdown_sem) {
        rt_kprintf("[key_mgr] Failed to create shutdown semaphore\n");
        rt_mq_delete(g_key_mgr.key_msg_queue);
        return -RT_ENOMEM;
    }

    // 3. 创建按键处理线程
    g_key_mgr.key_thread = rt_thread_create("key_mgr",
                                           key_thread_entry,
                                           RT_NULL,
                                           KEY_THREAD_STACK_SIZE,
                                           KEY_THREAD_PRIORITY,
                                           10);
    if (!g_key_mgr.key_thread) {
        rt_kprintf("[key_mgr] Failed to create key thread\n");
        rt_sem_delete(g_key_mgr.shutdown_sem);
        rt_mq_delete(g_key_mgr.key_msg_queue);
        return -RT_ENOMEM;
    }

    // 4. 初始化按键板硬件
    if (buttons_board_init(key_isr_callback) != RT_EOK) {
        rt_kprintf("[key_mgr] Failed to init buttons board\n");
        rt_sem_delete(g_key_mgr.shutdown_sem);
        rt_mq_delete(g_key_mgr.key_msg_queue);
        return -RT_ERROR;
    }

    // 5. 启动线程
    rt_thread_startup(g_key_mgr.key_thread);

    g_key_mgr.initialized = true;
    rt_kprintf("[key_mgr] Key manager initialized (thread-based, LED feedback enabled)\n");
    return 0;
}

/* 按键管理器去初始化 */
int key_manager_deinit(void)
{
    if (!g_key_mgr.initialized) {
        return 0;
    }

    rt_kprintf("[key_mgr] Deinitializing key manager...\n");

    // 1. 发送关闭消息
    key_message_t shutdown_msg = {.type = KEY_MSG_SHUTDOWN};
    key_send_message(&shutdown_msg);

    // 2. 等待线程结束
    rt_sem_take(g_key_mgr.shutdown_sem, 5000);

    // 3. 清理硬件
    buttons_board_deinit();

    // 4. 清理资源
    if (g_key_mgr.shutdown_sem) {
        rt_sem_delete(g_key_mgr.shutdown_sem);
        g_key_mgr.shutdown_sem = RT_NULL;
    }

    if (g_key_mgr.key_msg_queue) {
        rt_mq_delete(g_key_mgr.key_msg_queue);
        g_key_mgr.key_msg_queue = RT_NULL;
    }

    memset(&g_key_mgr, 0, sizeof(g_key_mgr));
    
    rt_kprintf("[key_mgr] Key manager deinitialized\n");
    return 0;
}

/* 注册上下文 - 直接操作，不需要消息队列 */
int key_manager_register_context(const key_context_config_t *config)
{
    if (!config || config->id >= KEY_CTX_MAX || config->id == KEY_CTX_NONE) {
        rt_kprintf("[key_mgr] Invalid config parameter\n");
        return -RT_EINVAL;
    }

    if (!g_key_mgr.initialized) {
        rt_kprintf("[key_mgr] Manager not initialized\n");
        return -RT_ERROR;
    }

    if (g_key_mgr.contexts[config->id].registered) {
        rt_kprintf("[key_mgr] Context %d already registered\n", config->id);
        return -RT_EBUSY;
    }

    g_key_mgr.contexts[config->id].config = *config;
    g_key_mgr.contexts[config->id].registered = true;
    g_key_mgr.contexts[config->id].active = false;

    rt_kprintf("[key_mgr] Registered context: %s (ID=%d)\n", 
              config->name, config->id);
    return 0;
}

/* 注销上下文 */
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

    // 如果是当前激活的上下文，先停用
    if (g_key_mgr.current_ctx == ctx_id) {
        key_message_t msg = {
            .type = KEY_MSG_DEACTIVATE_CONTEXT,
            .data.context_op.ctx_id = ctx_id
        };
        key_send_message(&msg);
    }

    memset(&g_key_mgr.contexts[ctx_id], 0, sizeof(context_info_t));

    rt_kprintf("[key_mgr] Unregistered context ID=%d\n", ctx_id);
    return 0;
}

/* 激活上下文 - 通过消息队列 */
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

/* 停用上下文 - 通过消息队列 */
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

/* 获取当前激活的上下文 */
key_context_id_t key_manager_get_active_context(void)
{
    return g_key_mgr.current_ctx;
}

/* 启用/禁用LED反馈 */
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

/* 获取上下文名称 */
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

/* 检查LED反馈是否启用 */
bool key_manager_is_led_feedback_enabled(void)
{
    return g_key_mgr.led_feedback_enabled;
}

/* 上下文堆栈操作 */
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