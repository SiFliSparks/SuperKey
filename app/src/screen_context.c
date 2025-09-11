#include "screen_context.h"
#include "app_controller.h"
#include "led_controller.h"
#include <rtthread.h>
#include "hid_device.h"  
#include "event_bus.h"

static int screen_group1_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_group2_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_group3_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_time_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_media_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_web_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_shortcut_key_handler(int key_idx, button_action_t action, void *user_data);

static key_context_config_t g_l2_time_config = {
    .id = KEY_CTX_L2_TIME,
    .name = "SCREEN_L2_TIME",
    .handler = screen_l2_time_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_media_config = {
    .id = KEY_CTX_L2_MEDIA,
    .name = "SCREEN_L2_MEDIA", 
    .handler = screen_l2_media_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_web_config = {
    .id = KEY_CTX_L2_WEB,
    .name = "SCREEN_L2_WEB",
    .handler = screen_l2_web_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_shortcut_config = {
    .id = KEY_CTX_L2_SHORTCUT,
    .name = "SCREEN_L2_SHORTCUT",
    .handler = screen_l2_shortcut_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static int screen_group1_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen1] Key 1: Show time details\n");
            event_bus_publish_led_feedback(0, LED_COLOR_BLUE, 300);
            
            if (screen_enter_level2_auto(SCREEN_GROUP_1) != 0) {
                rt_kprintf("[Screen1] Failed to enter level 2\n");
            }
            break;
            
        case 1:
            rt_kprintf("[Screen1] Key 2: Refresh weather data\n");
            event_bus_publish_led_feedback(1, LED_COLOR_YELLOW, 300);
            screen_update_sensor_data();
            break;
            
        case 2:
            rt_kprintf("[Screen1] Key 3: Stock info toggle\n");
            event_bus_publish_led_feedback(2, LED_COLOR_PURPLE, 300);
            break;
            
        case 3:
            rt_kprintf("[Screen1] Key 4: Switch to next screen group\n");
            event_bus_publish_led_feedback(0, LED_COLOR_WHITE, 100);
            event_bus_publish_led_feedback(1, LED_COLOR_WHITE, 100);
            event_bus_publish_led_feedback(2, LED_COLOR_WHITE, 100);
            screen_next_group();
            break;
    }
    
    return 0;
}

static int screen_group2_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen2] Key 1: CPU/GPU info toggle\n");
            screen_update_cpu_usage(45.6);
            screen_update_gpu_usage(32.1);
            break;
            
        case 1:
            rt_kprintf("[Screen2] Key 2: Memory/Disk info toggle\n");
            screen_update_ram_usage(67.8);
            break;
            
        case 2:
            rt_kprintf("[Screen2] Key 3: Network info toggle\n");
            screen_update_net_speeds(2.5, 12.8);
            break;
            
        case 3:
            rt_kprintf("[Screen2] Key 4: Switch to next screen group\n");
                event_bus_publish_led_feedback(2, LED_COLOR_WHITE, 100);
                event_bus_publish_led_feedback(1, LED_COLOR_WHITE, 100);
                event_bus_publish_led_feedback(0, LED_COLOR_WHITE, 100);
            screen_next_group();
            break;
    }
    
    return 0;
}

static int screen_group3_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen3] Key 1: Enter media control\n");
            if (screen_enter_level2(SCREEN_L2_MEDIA_GROUP, SCREEN_L2_MEDIA_CONTROL) != 0) {
                rt_kprintf("[Screen3] Failed to enter media control L2\n");
            }
            break;
            
        case 1:
            rt_kprintf("[Screen3] Key 2: Enter web control\n");
            if (screen_enter_level2(SCREEN_L2_WEB_GROUP, SCREEN_L2_WEB_CONTROL) != 0) {
                rt_kprintf("[Screen3] Failed to enter web control L2\n");
            }
            break;
            
        case 2:
            rt_kprintf("[Screen3] Key 3: Enter shortcut control\n");
            if (screen_enter_level2(SCREEN_L2_SHORTCUT_GROUP, SCREEN_L2_SHORTCUT_CONTROL) != 0) {
                rt_kprintf("[Screen3] Failed to enter shortcut control L2\n");
            }
            break;
            
        case 3:
            rt_kprintf("[Screen3] Key 4: Switch to next screen group\n");
            screen_next_group();
            break;
    }
    
    return 0;
}

static int screen_l2_time_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen L2] Key 1: Time detail function 1\n");
            break;
            
        case 1:
            rt_kprintf("[Screen L2] Key 2: Time detail function 2\n");
            break;
            
        case 2:
            rt_kprintf("[Screen L2] Key 3: Time detail function 3\n");
            break;
            
        case 3:
            rt_kprintf("[Screen L2] Key 4: Return to Level 1\n");
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static int screen_l2_media_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    bool hid_ready = hid_device_ready();
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen L2 Media] Key 1: Volume Up\n");
            if (hid_ready) {
                hid_consumer_click(CC_VOL_UP);
                rt_kprintf("[Screen L2 Media] Volume+ sent\n");
            } else {
                rt_kprintf("[Screen L2 Media] HID not ready\n");
            }
            break;
            
        case 1:
            rt_kprintf("[Screen L2 Media] Key 2: Volume Down\n");
            if (hid_ready) {
                hid_consumer_click(CC_VOL_DOWN);
                rt_kprintf("[Screen L2 Media] Volume- sent\n");
            } else {
                rt_kprintf("[Screen L2 Media] HID not ready\n");
            }
            break;
            
        case 2:
            rt_kprintf("[Screen L2 Media] Key 3: Play/Pause\n");
            if (hid_ready) {
                hid_consumer_click(CC_PLAY_PAUSE);
                rt_kprintf("[Screen L2 Media] Play/Pause sent\n");
            } else {
                rt_kprintf("[Screen L2 Media] HID not ready\n");
            }
            break;
            
        case 3:
            rt_kprintf("[Screen L2 Media] Key 4: Return to Level 1\n");
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static int screen_l2_web_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    bool hid_ready = hid_device_ready();
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen L2 Web] Key 1: Page Up\n");
            if (hid_ready) {
                hid_kbd_send_combo(0, KEY_PAGE_UP);
                rt_kprintf("[Screen L2 Web] Page Up sent\n");
            } else {
                rt_kprintf("[Screen L2 Web] HID not ready\n");
            }
            break;
            
        case 1:
            rt_kprintf("[Screen L2 Web] Key 2: Page Down\n");
            if (hid_ready) {
                hid_kbd_send_combo(0, KEY_PAGE_DOWN);
                rt_kprintf("[Screen L2 Web] Page Down sent\n");
            } else {
                rt_kprintf("[Screen L2 Web] HID not ready\n");
            }
            break;
            
        case 2:
            rt_kprintf("[Screen L2 Web] Key 3: Refresh (F5)\n");
            if (hid_ready) {
                hid_kbd_send_combo(0, KEY_F5);
                rt_kprintf("[Screen L2 Web] F5 sent\n");
            } else {
                rt_kprintf("[Screen L2 Web] HID not ready\n");
            }
            break;
            
        case 3:
            rt_kprintf("[Screen L2 Web] Key 4: Return to Level 1\n");
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static int screen_l2_shortcut_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_CLICKED) {
        return 0;
    }
    
    bool hid_ready = hid_device_ready();
    
    switch (key_idx) {
        case 0:
            rt_kprintf("[Screen L2 Shortcut] Key 1: Copy (Ctrl+C)\n");
            if (hid_ready) {
                hid_kbd_send_combo(OS_MODIFIER, KEY_C);
                rt_kprintf("[Screen L2 Shortcut] Copy sent\n");
            } else {
                rt_kprintf("[Screen L2 Shortcut] HID not ready\n");
            }
            break;
            
        case 1:
            rt_kprintf("[Screen L2 Shortcut] Key 2: Paste (Ctrl+V)\n");
            if (hid_ready) {
                hid_kbd_send_combo(OS_MODIFIER, KEY_V);
                rt_kprintf("[Screen L2 Shortcut] Paste sent\n");
            } else {
                rt_kprintf("[Screen L2 Shortcut] HID not ready\n");
            }
            break;
            
        case 2:
            rt_kprintf("[Screen L2 Shortcut] Key 3: Undo (Ctrl+Z)\n");
            if (hid_ready) {
                hid_kbd_send_combo(OS_MODIFIER, KEY_Z);
                rt_kprintf("[Screen L2 Shortcut] Undo sent\n");
            } else {
                rt_kprintf("[Screen L2 Shortcut] HID not ready\n");
            }
            break;
            
        case 3:
            rt_kprintf("[Screen L2 Shortcut] Key 4: Return to Level 1\n");
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static bool g_contexts_initialized = false;

int screen_context_init_all(void)
{
    if (g_contexts_initialized) {
        rt_kprintf("[ScreenCtx] Already initialized\n");
        return 0;
    }
    
    int ret;
    
    key_context_config_t config1 = {
        .id = KEY_CTX_MENU_NAVIGATION,
        .name = "SCREEN_GROUP_1",
        .handler = screen_group1_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config1);
    if (ret != 0) {
        rt_kprintf("[ScreenCtx] Failed to register screen group 1 context: %d\n", ret);
        return ret;
    }
    
    key_context_config_t config2 = {
        .id = KEY_CTX_SYSTEM,
        .name = "SCREEN_GROUP_2",
        .handler = screen_group2_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config2);
    if (ret != 0) {
        rt_kprintf("[ScreenCtx] Failed to register screen group 2 context: %d\n", ret);
        return ret;
    }
    
    key_context_config_t config3 = {
        .id = KEY_CTX_SETTINGS,
        .name = "SCREEN_GROUP_3", 
        .handler = screen_group3_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config3);
    if (ret != 0) {
        rt_kprintf("[ScreenCtx] Failed to register screen group 3 context: %d\n", ret);
        return ret;
    }
    
    g_contexts_initialized = true;
    rt_kprintf("[ScreenCtx] All screen contexts initialized (simplified HID integration)\n");
    return 0;
}

int screen_context_deinit_all(void)
{
    if (!g_contexts_initialized) {
        return 0;
    }
    
    key_manager_unregister_context(KEY_CTX_MENU_NAVIGATION);
    key_manager_unregister_context(KEY_CTX_SYSTEM);
    key_manager_unregister_context(KEY_CTX_SETTINGS);
    key_manager_unregister_context(KEY_CTX_L2_TIME);
    key_manager_unregister_context(KEY_CTX_L2_MEDIA);
    key_manager_unregister_context(KEY_CTX_L2_WEB);
    key_manager_unregister_context(KEY_CTX_L2_SHORTCUT);
    
    g_contexts_initialized = false;
    rt_kprintf("[ScreenCtx] All screen contexts deinitialized\n");
    return 0;
}

int screen_context_activate_for_group(screen_group_t group)
{
    int ret = 0;
    
    if (!g_contexts_initialized) {
        rt_kprintf("[ScreenCtx] Contexts not initialized\n");
        return -RT_ERROR;
    }
    
    screen_context_deactivate_all();
    
    switch (group) {
        case SCREEN_GROUP_1:
            ret = key_manager_activate_context(KEY_CTX_MENU_NAVIGATION);
            rt_kprintf("[ScreenCtx] Activated Group 1: [1]时间详情 [2]刷新天气 [3]股票切换 [4]下一组\n");
            break;
            
        case SCREEN_GROUP_2:
            ret = key_manager_activate_context(KEY_CTX_SYSTEM);
            rt_kprintf("[ScreenCtx] Activated Group 2: [1]CPU/GPU [2]内存/磁盘 [3]网络 [4]下一组\n");
            break;
            
        case SCREEN_GROUP_3:
            ret = key_manager_activate_context(KEY_CTX_SETTINGS);
            if (ret == 0) {
                rt_kprintf("[ScreenCtx] Activated Group 3: [1]媒体控制 [2]网页控制 [3]快捷键 [4]下一组\n");
                rt_kprintf("           按键直接进入对应L2功能页面\n");
            }
            break;
            
        default:
            rt_kprintf("[ScreenCtx] Invalid screen group: %d\n", group);
            return -RT_EINVAL;
    }
    
    if (ret != 0) {
        rt_kprintf("[ScreenCtx] Failed to activate context for group %d: %d\n", group, ret);
    }
    
    return ret;
}

int screen_context_deactivate_all(void)
{
    key_manager_deactivate_context(KEY_CTX_MENU_NAVIGATION);
    key_manager_deactivate_context(KEY_CTX_SYSTEM);
    key_manager_deactivate_context(KEY_CTX_SETTINGS);
    
    return 0;
}

int screen_context_activate_for_level2(screen_l2_group_t l2_group)
{
    if (!g_contexts_initialized) {
        rt_kprintf("[ScreenCtx] Contexts not initialized for L2\n");
        return -RT_ERROR;
    }
    
    screen_context_deactivate_all();
    
    int ret = 0;
    switch (l2_group) {
        case SCREEN_L2_TIME_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_TIME) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_time_config);
                if (ret != 0) {
                    rt_kprintf("[ScreenCtx] Failed to register L2 time context: %d\n", ret);
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_TIME);
            if (ret == 0) {
                rt_kprintf("[ScreenCtx] Activated L2 Time Group: [1]详情1 [2]详情2 [3]详情3 [4]返回\n");
            }
            break;
            
        case SCREEN_L2_MEDIA_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_MEDIA) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_media_config);
                if (ret != 0) {
                    rt_kprintf("[ScreenCtx] Failed to register L2 media context: %d\n", ret);
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_MEDIA);
            if (ret == 0) {
                rt_kprintf("[ScreenCtx] Activated L2 Media Group: [1]音量+ [2]音量- [3]播放/暂停 [4]返回\n");
            }
            break;
            
        case SCREEN_L2_WEB_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_WEB) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_web_config);
                if (ret != 0) {
                    rt_kprintf("[ScreenCtx] Failed to register L2 web context: %d\n", ret);
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_WEB);
            if (ret == 0) {
                rt_kprintf("[ScreenCtx] Activated L2 Web Group: [1]上翻页 [2]下翻页 [3]刷新F5 [4]返回\n");
            }
            break;
            
        case SCREEN_L2_SHORTCUT_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_SHORTCUT) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_shortcut_config);
                if (ret != 0) {
                    rt_kprintf("[ScreenCtx] Failed to register L2 shortcut context: %d\n", ret);
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_SHORTCUT);
            if (ret == 0) {
                rt_kprintf("[ScreenCtx] Activated L2 Shortcut Group: [1]复制 [2]粘贴 [3]撤销 [4]返回\n");
            }
            break;
            
        default:
            rt_kprintf("[ScreenCtx] Unknown L2 group: %d\n", l2_group);
            ret = -RT_EINVAL;
            break;
    }
    
    return ret;
}

int screen_context_deactivate_level2(void)
{
    key_manager_deactivate_context(KEY_CTX_L2_TIME);
    key_manager_deactivate_context(KEY_CTX_L2_MEDIA);
    key_manager_deactivate_context(KEY_CTX_L2_WEB);
    key_manager_deactivate_context(KEY_CTX_L2_SHORTCUT);
    
    rt_kprintf("[ScreenCtx] All L2 contexts deactivated\n");
    return 0;
}