#include "key_manager.h"
#include "buttons_board.h"
#include "led_controller.h"
#include "event_bus.h"
#include <string.h>

#define MAX_CONTEXT_STACK_DEPTH 4

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
    rt_mutex_t lock;
    bool initialized;
    bool led_feedback_enabled;
} g_key_mgr = {0};

static void handle_led_feedback(int key_idx, button_action_t action)
{
    if (!g_key_mgr.led_feedback_enabled) {
        return;
    }

    if (action == BUTTON_PRESSED) {
        switch (key_idx) {
            case 0:
                event_bus_publish_led_feedback(0, LED_COLOR_BLUE, 150);
                break;
                
            case 1:
                event_bus_publish_led_feedback(1, LED_COLOR_GREEN, 150);
                break;
                
            case 2:
                event_bus_publish_led_feedback(2, LED_COLOR_RED, 150);
                break;
                
            case 3:
                event_bus_publish_led_feedback(0, LED_COLOR_WHITE, 100);
                event_bus_publish_led_feedback(1, LED_COLOR_WHITE, 100);
                event_bus_publish_led_feedback(2, LED_COLOR_WHITE, 100);
                break;
        }
    }
}

static void key_event_dispatcher(int32_t pin, button_action_t action)
{
    int key_idx = buttons_board_pin_to_idx(pin);
    if (key_idx < 0) {
        rt_kprintf("[key_mgr] Invalid pin %d\n", pin);
        return;
    }

    if (!g_key_mgr.initialized || !g_key_mgr.lock) {
        return;
    }

    handle_led_feedback(key_idx, action);

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);
    
    if (g_key_mgr.current_ctx != KEY_CTX_NONE && 
        g_key_mgr.current_ctx < KEY_CTX_MAX) {
        
        context_info_t *ctx = &g_key_mgr.contexts[g_key_mgr.current_ctx];
        if (ctx->registered && ctx->active && ctx->config.handler) {
            int ret = ctx->config.handler(key_idx, action, ctx->config.user_data);
            
            if (ret != 0) {
                rt_kprintf("[key_mgr] Context %s didn't handle key %d, action %d\n", 
                          ctx->config.name, key_idx, action);
            }
        }
    } else {
        rt_kprintf("[key_mgr] No active context, key %d ignored (but LED feedback provided)\n", key_idx);
    }
    
    rt_mutex_release(g_key_mgr.lock);
}

int key_manager_init(void)
{
    if (g_key_mgr.initialized) {
        rt_kprintf("[key_mgr] Already initialized\n");
        return 0;
    }

    memset(&g_key_mgr, 0, sizeof(g_key_mgr));
    g_key_mgr.current_ctx = KEY_CTX_NONE;
    g_key_mgr.stack_top = -1;
    g_key_mgr.led_feedback_enabled = true;

    g_key_mgr.lock = rt_mutex_create("key_mgr", RT_IPC_FLAG_PRIO);
    if (!g_key_mgr.lock) {
        rt_kprintf("[key_mgr] Failed to create mutex\n");
        return -RT_ENOMEM;
    }

    if (buttons_board_init(key_event_dispatcher) != RT_EOK) {
        rt_kprintf("[key_mgr] Failed to init buttons board with SDK driver\n");
        rt_mutex_delete(g_key_mgr.lock);
        g_key_mgr.lock = RT_NULL;
        return -RT_ERROR;
    }

    g_key_mgr.initialized = true;
    rt_kprintf("[key_mgr] Key manager initialized with SDK driver (LED feedback enabled)\n");
    return 0;
}

int key_manager_deinit(void)
{
    if (!g_key_mgr.initialized) {
        return 0;
    }

    rt_kprintf("[key_mgr] Deinitializing key manager...\n");

    if (g_key_mgr.lock) {
        rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);
        g_key_mgr.current_ctx = KEY_CTX_NONE;
        rt_mutex_release(g_key_mgr.lock);
    }

    buttons_board_deinit();

    if (g_key_mgr.lock) {
        rt_mutex_delete(g_key_mgr.lock);
        g_key_mgr.lock = RT_NULL;
    }

    memset(&g_key_mgr, 0, sizeof(g_key_mgr));
    
    rt_kprintf("[key_mgr] Key manager deinitialized\n");
    return 0;
}

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

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);
    
    if (g_key_mgr.contexts[config->id].registered) {
        rt_mutex_release(g_key_mgr.lock);
        rt_kprintf("[key_mgr] Context %d already registered\n", config->id);
        return -RT_EBUSY;
    }

    g_key_mgr.contexts[config->id].config = *config;
    g_key_mgr.contexts[config->id].registered = true;
    g_key_mgr.contexts[config->id].active = false;

    rt_mutex_release(g_key_mgr.lock);
    
    rt_kprintf("[key_mgr] Registered context: %s (ID=%d)\n", 
              config->name, config->id);
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

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);

    if (!g_key_mgr.contexts[ctx_id].registered) {
        rt_mutex_release(g_key_mgr.lock);
        return -RT_ERROR;
    }

    if (g_key_mgr.current_ctx == ctx_id) {
        g_key_mgr.current_ctx = KEY_CTX_NONE;
    }

    memset(&g_key_mgr.contexts[ctx_id], 0, sizeof(context_info_t));

    rt_mutex_release(g_key_mgr.lock);

    rt_kprintf("[key_mgr] Unregistered context ID=%d\n", ctx_id);
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

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);

    if (g_key_mgr.current_ctx != KEY_CTX_NONE && g_key_mgr.current_ctx < KEY_CTX_MAX) {
        g_key_mgr.contexts[g_key_mgr.current_ctx].active = false;
    }

    if (ctx_id != KEY_CTX_NONE) {
        if (g_key_mgr.contexts[ctx_id].registered) {
            g_key_mgr.contexts[ctx_id].active = true;
            g_key_mgr.current_ctx = ctx_id;
            rt_kprintf("[key_mgr] Activated context: %s\n", 
                      g_key_mgr.contexts[ctx_id].config.name);
        } else {
            rt_mutex_release(g_key_mgr.lock);
            rt_kprintf("[key_mgr] Context %d not registered\n", ctx_id);
            return -RT_ERROR;
        }
    } else {
        g_key_mgr.current_ctx = KEY_CTX_NONE;
        rt_kprintf("[key_mgr] Activated NONE context (idle)\n");
    }

    rt_mutex_release(g_key_mgr.lock);
    return 0;
}

int key_manager_deactivate_context(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX) {
        return -RT_EINVAL;
    }

    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);

    if (g_key_mgr.current_ctx == ctx_id) {
        if (ctx_id != KEY_CTX_NONE) {
            g_key_mgr.contexts[ctx_id].active = false;
        }
        g_key_mgr.current_ctx = KEY_CTX_NONE;
        rt_kprintf("[key_mgr] Deactivated context ID=%d\n", ctx_id);
    }

    rt_mutex_release(g_key_mgr.lock);
    return 0;
}

key_context_id_t key_manager_get_active_context(void)
{
    return g_key_mgr.current_ctx;
}

int key_manager_push_context(key_context_id_t ctx_id)
{
    if (ctx_id >= KEY_CTX_MAX || !g_key_mgr.initialized) {
        return -RT_EINVAL;
    }

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);

    if (g_key_mgr.stack_top >= MAX_CONTEXT_STACK_DEPTH - 1) {
        rt_mutex_release(g_key_mgr.lock);
        rt_kprintf("[key_mgr] Context stack overflow\n");
        return -RT_EFULL;
    }

    g_key_mgr.context_stack[++g_key_mgr.stack_top] = g_key_mgr.current_ctx;
    
    rt_mutex_release(g_key_mgr.lock);

    int ret = key_manager_activate_context(ctx_id);
    if (ret != 0) {
        rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);
        g_key_mgr.stack_top--;
        rt_mutex_release(g_key_mgr.lock);
    }

    return ret;
}

int key_manager_pop_context(void)
{
    if (!g_key_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_mutex_take(g_key_mgr.lock, RT_WAITING_FOREVER);

    if (g_key_mgr.stack_top < 0) {
        rt_mutex_release(g_key_mgr.lock);
        rt_kprintf("[key_mgr] Context stack is empty\n");
        return -RT_EEMPTY;
    }

    key_context_id_t prev_ctx = g_key_mgr.context_stack[g_key_mgr.stack_top--];
    
    rt_mutex_release(g_key_mgr.lock);

    return key_manager_activate_context(prev_ctx);
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

int key_manager_enable_led_feedback(bool enable)
{
    g_key_mgr.led_feedback_enabled = enable;
    rt_kprintf("[key_mgr] LED feedback %s\n", enable ? "enabled" : "disabled");
    return 0;
}

bool key_manager_is_led_feedback_enabled(void)
{
    return g_key_mgr.led_feedback_enabled;
}