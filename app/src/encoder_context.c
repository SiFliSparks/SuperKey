#include "encoder_context.h"
#include "encoder_controller.h"
#include "hid_device.h"
#include "led_controller.h" 
#include "event_bus.h"
#include <rtthread.h>
#include <string.h>

static struct {
    bool initialized;
    bool activated;
    encoder_mode_t current_encoder_mode;
    rt_mutex_t mode_lock;
} g_enc_ctx = {0};

static int encoder_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type != EVENT_ENCODER_ROTATED) {
        return -1;
    }
    
    int32_t delta = event->data.encoder.delta;
    if (delta == 0) return 0;
    
    bool hid_ready = hid_device_ready();
    
    rt_mutex_take(g_enc_ctx.mode_lock, RT_WAITING_FOREVER);
    encoder_mode_t current_mode = g_enc_ctx.current_encoder_mode;
    rt_mutex_release(g_enc_ctx.mode_lock);
    
    switch (current_mode) {
        case ENCODER_MODE_VOLUME:
            if (hid_ready) {
                uint8_t command = (delta > 0) ? CC_VOL_UP : CC_VOL_DOWN;
                hid_consumer_click(command);
                rt_kprintf("[ENC_CTX] Volume %s\n", (delta > 0) ? "UP" : "DOWN");
                
                uint32_t led_color = (delta > 0) ? LED_COLOR_GREEN : LED_COLOR_RED;
                event_bus_publish_led_feedback(1, led_color, 150);
            }
            break;
            
        case ENCODER_MODE_SCROLL:
            if (hid_ready) {
                uint8_t key = (delta > 0) ? KEY_PAGE_DOWN : KEY_PAGE_UP;
                hid_kbd_send_combo(0, key);
                rt_kprintf("[ENC_CTX] Scroll %s\n", (delta > 0) ? "DOWN" : "UP");
                
                uint32_t led_color = (delta > 0) ? LED_COLOR_BLUE : LED_COLOR_CYAN;
                event_bus_publish_led_feedback(2, led_color, 150);
            }
            break;
            
        case ENCODER_MODE_BRIGHTNESS:
            rt_kprintf("[ENC_CTX] Brightness %s\n", (delta > 0) ? "UP" : "DOWN");
            
            if (delta > 0) {
                event_bus_publish_led_feedback(0, LED_COLOR_WHITE, 100);
                event_bus_publish_led_feedback(1, LED_COLOR_WHITE, 100);
                event_bus_publish_led_feedback(2, LED_COLOR_WHITE, 100);
            } else {
                event_bus_publish_led_feedback(1, LED_COLOR_YELLOW, 200);
            }
            break;
            
        case ENCODER_MODE_MENU_NAV:
            rt_kprintf("[ENC_CTX] Menu %s\n", (delta > 0) ? "Next" : "Previous");
            uint32_t nav_color = LED_COLOR_PURPLE;
            int nav_led = (delta > 0) ? 2 : 0;
            event_bus_publish_led_feedback(nav_led, nav_color, 150);
            break;
            
        case ENCODER_MODE_IDLE:
        default:
            if (delta != 0) {
                rt_kprintf("[ENC_CTX] Idle rotation: delta=%d\n", delta);
            }
            break;
    }
    
    return 0;
}

static int encoder_context_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    rt_mutex_take(g_enc_ctx.mode_lock, RT_WAITING_FOREVER);
    
    encoder_mode_t new_mode = g_enc_ctx.current_encoder_mode;
    uint8_t new_sensitivity = 1;
    const char* mode_desc = "";
    uint32_t led_color = LED_COLOR_WHITE;
    
    switch (key_idx) {
        case 0:
            new_mode = ENCODER_MODE_VOLUME;
            new_sensitivity = 2;
            mode_desc = "VOLUME mode";
            led_color = LED_COLOR_GREEN;
            break;
            
        case 1:
            new_mode = ENCODER_MODE_SCROLL;
            new_sensitivity = 4;
            mode_desc = "SCROLL mode";
            led_color = LED_COLOR_BLUE;
            break;
            
        case 2:
            new_mode = ENCODER_MODE_BRIGHTNESS;
            new_sensitivity = 3;
            mode_desc = "BRIGHTNESS mode";
            led_color = LED_COLOR_YELLOW;
            break;
            
        case 3:
            new_mode = ENCODER_MODE_IDLE;
            new_sensitivity = 1;
            mode_desc = "IDLE mode (reset)";
            led_color = LED_COLOR_WHITE;
            
            encoder_controller_reset_count();
            event_bus_publish_led_feedback(0, LED_COLOR_OFF, 0);
            event_bus_publish_led_feedback(1, LED_COLOR_OFF, 0);
            event_bus_publish_led_feedback(2, LED_COLOR_OFF, 0);
            break;
            
        default:
            rt_mutex_release(g_enc_ctx.mode_lock);
            return 0;
    }
    
    g_enc_ctx.current_encoder_mode = new_mode;
    rt_mutex_release(g_enc_ctx.mode_lock);
    
    encoder_controller_set_mode(new_mode);
    encoder_controller_set_sensitivity(new_sensitivity);
    
    rt_kprintf("[ENC_CTX] Switched to %s\n", mode_desc);
    
    event_bus_publish_led_feedback(key_idx, led_color, 500);
    
    return 0;
}

int encoder_context_init(void)
{
    if (g_enc_ctx.initialized) {
        rt_kprintf("[ENC_CTX] Already initialized\n");
        return 0;
    }
    
    memset(&g_enc_ctx, 0, sizeof(g_enc_ctx));
    g_enc_ctx.current_encoder_mode = ENCODER_MODE_IDLE;
    
    g_enc_ctx.mode_lock = rt_mutex_create("enc_mode_lock", RT_IPC_FLAG_PRIO);
    if (!g_enc_ctx.mode_lock) {
        rt_kprintf("[ENC_CTX] Failed to create mode lock\n");
        return -RT_ENOMEM;
    }
    
    key_context_config_t config = {
        .id = KEY_CTX_VOLUME_CONTROL,
        .name = "ENCODER_CONTROL",
        .handler = encoder_context_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    
    int ret = key_manager_register_context(&config);
    if (ret != 0) {
        rt_kprintf("[ENC_CTX] Failed to register context: %d\n", ret);
        rt_mutex_delete(g_enc_ctx.mode_lock);
        return ret;
    }
    
    event_bus_subscribe(EVENT_ENCODER_ROTATED, encoder_event_handler, NULL, EVENT_PRIORITY_HIGH);
    
    g_enc_ctx.initialized = true;
    rt_kprintf("[ENC_CTX] Simplified encoder context initialized\n");
    return 0;
}

int encoder_context_deinit(void)
{
    if (!g_enc_ctx.initialized) {
        return 0;
    }
    
    if (g_enc_ctx.activated) {
        encoder_context_deactivate();
    }
    
    event_bus_unsubscribe(EVENT_ENCODER_ROTATED, encoder_event_handler);
    
    key_manager_unregister_context(KEY_CTX_VOLUME_CONTROL);
    
    if (g_enc_ctx.mode_lock) {
        rt_mutex_delete(g_enc_ctx.mode_lock);
        g_enc_ctx.mode_lock = NULL;
    }
    
    memset(&g_enc_ctx, 0, sizeof(g_enc_ctx));
    
    rt_kprintf("[ENC_CTX] Simplified encoder context deinitialized\n");
    return 0;
}

int encoder_context_activate(void)
{
    if (!g_enc_ctx.initialized) {
        rt_kprintf("[ENC_CTX] Not initialized\n");
        return -RT_ERROR;
    }
    
    int ret = key_manager_activate_context(KEY_CTX_VOLUME_CONTROL);
    if (ret != 0) {
        rt_kprintf("[ENC_CTX] Failed to activate context: %d\n", ret);
        return ret;
    }
    
    ret = encoder_controller_start_polling();
    if (ret != 0) {
        rt_kprintf("[ENC_CTX] Failed to start encoder polling: %d\n", ret);
        key_manager_deactivate_context(KEY_CTX_VOLUME_CONTROL);
        return ret;
    }
    
    g_enc_ctx.activated = true;
    g_enc_ctx.current_encoder_mode = ENCODER_MODE_IDLE;
    encoder_controller_set_mode(ENCODER_MODE_IDLE);
    
    rt_kprintf("[ENC_CTX] Simplified encoder control activated\n");
    rt_kprintf("[ENC_CTX] Keys: [0]Volume [1]Scroll [2]Brightness [3]Reset\n");
    rt_kprintf("[ENC_CTX] Rotate encoder to control selected function\n");
    
    return 0;
}

int encoder_context_deactivate(void)
{
    if (!g_enc_ctx.initialized || !g_enc_ctx.activated) {
        return 0;
    }
    
    encoder_controller_stop_polling();
    
    key_manager_deactivate_context(KEY_CTX_VOLUME_CONTROL);
    
    encoder_controller_set_mode(ENCODER_MODE_IDLE);
    encoder_controller_reset_count();
    
    g_enc_ctx.activated = false;
    
    rt_kprintf("[ENC_CTX] Encoder control deactivated\n");
    return 0;
}