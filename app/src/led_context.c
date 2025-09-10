#include "led_context.h"
#include "led_controller.h"
#include "buttons_board.h"
#include <rtthread.h>

static int led_context_handler(int key_idx, button_action_t action, void *user_data)
{
    if (action != BUTTON_PRESSED) {
        return 0;
    }

    switch (key_idx) {
        case 0:
            led_controller_light_led(0, LED_COLOR_RED, 1000);
            break;
            
        case 1:
            led_controller_light_led(1, LED_COLOR_GREEN, 1000);
            break;
            
        case 2:
            led_controller_light_led(2, LED_COLOR_BLUE, 1000);
            break;
            
        case 3:
            led_controller_turn_off_all();
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

int led_context_init(void)
{
    key_context_config_t config = {
        .id = KEY_CTX_HID_SHORTCUT,
        .name = "LED_CONTROL",
        .handler = led_context_handler,
        .user_data = RT_NULL,
        .priority = 100,
        .exclusive = false
    };
    
    int ret = key_manager_register_context(&config);
    if (ret != 0) {
        rt_kprintf("[LED_CTX] Failed to register LED context: %d\n", ret);
        return ret;
    }
    
    rt_kprintf("[LED_CTX] LED context initialized\n");
    return 0;
}

int led_context_deinit(void)
{
    int ret = key_manager_unregister_context(KEY_CTX_HID_SHORTCUT);
    if (ret != 0) {
        rt_kprintf("[LED_CTX] Failed to unregister LED context: %d\n", ret);
        return ret;
    }
    
    rt_kprintf("[LED_CTX] LED context deinitialized\n");
    return 0;
}

int led_context_activate(void)
{
    int ret = key_manager_activate_context(KEY_CTX_HID_SHORTCUT);
    if (ret != 0) {
        rt_kprintf("[LED_CTX] Failed to activate LED context: %d\n", ret);
        return ret;
    }
    
    rt_kprintf("[LED_CTX] LED control mode activated\n");
    rt_kprintf("[LED_CTX] [1]Red LED [2]Green LED [3]Blue LED [4]All Off\n");
    return 0;
}

int led_context_deactivate(void)
{
    int ret = key_manager_deactivate_context(KEY_CTX_HID_SHORTCUT);
    if (ret != 0) {
        rt_kprintf("[LED_CTX] Failed to deactivate LED context: %d\n", ret);
        return ret;
    }
    
    led_controller_turn_off_all();
    
    rt_kprintf("[LED_CTX] LED control mode deactivated\n");
    return 0;
}