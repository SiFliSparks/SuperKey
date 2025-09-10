#include "app_controller.h"
#include "key_manager.h"
#include "encoder_context.h"       
#include "encoder_controller.h"    
#include "hid_device.h"
#include "led_controller.h"
#include "led_context.h"
#include "screen_context.h"
#include "screen.h"
#include "event_bus.h"
#include <string.h>

#include <board.h>
#include "bf0_hal.h"
#include "rtthread.h"

#ifndef USBC_BASE
    #ifdef SOC_SF32LB52X
        #define USBC_BASE 0x40053000UL
    #elif defined(SOC_SF32LB58X)
        #define USBC_BASE 0x40053000UL
    #else
        #define USBC_BASE 0x40053000UL
        #warning "Using default USB base address 0x40053000. Please verify this is correct for your chip."
    #endif
#endif

static uintptr_t HAL_Get_USB_Base(void)
{
    return USBC_BASE;
}

static struct {
    bool initialized;
    char current_mode[32];
    bool encoder_enabled;
    bool hid_context_activated;
} g_app_ctrl = {0};

static bool g_app_initialized = false;

int app_controller_init(void)
{
    if (g_app_initialized) {
        rt_kprintf("[App] Already initialized\n");
        return 0;
    }

    rt_kprintf("[App] Initializing application controller...\n");

    if (event_bus_init() != 0) {
        rt_kprintf("[App] Failed to init event bus\n");
        return -1;
    }

    hid_device_init(0, HAL_Get_USB_Base());
    rt_kprintf("[App] HID device initialized\n");
    
    if (key_manager_init() != 0) {
        rt_kprintf("[App] Failed to init key manager\n");
        return -1;
    }
    rt_kprintf("[App] Key manager initialized\n");

    if (led_controller_init() != 0) {
        rt_kprintf("[App] Failed to init LED controller\n");
        return -1;
    }
    rt_kprintf("[App] LED controller initialized\n");

    if (encoder_controller_init() != 0) {
        rt_kprintf("[App] Failed to init encoder controller\n");
        return -1;
    }
    rt_kprintf("[App] Encoder controller initialized\n");

    if (encoder_context_init() != 0) {
        rt_kprintf("[App] Failed to init encoder context\n");
        return -1;
    }
    rt_kprintf("[App] Encoder context initialized\n");

    if (screen_context_init_all() != 0) {
        rt_kprintf("[App] Failed to init screen contexts\n");
        return -1;
    }
    rt_kprintf("[App] Screen contexts initialized\n");

    if (led_context_init() != 0) {
        rt_kprintf("[App] Failed to init LED context\n");
        return -1;
    }
    rt_kprintf("[App] LED context initialized\n");

    if (encoder_context_activate() != 0) {
        rt_kprintf("[App] Failed to activate encoder context\n");
        return -1;
    }
    rt_kprintf("[App] Encoder context activated\n");

    screen_group_t current_group = screen_get_current_group();
    if (screen_context_activate_for_group(current_group) != 0) {
        rt_kprintf("[App] Failed to activate screen context for group %d\n", current_group);
    }

    g_app_ctrl.initialized = true;
    g_app_ctrl.encoder_enabled = true;
    g_app_ctrl.hid_context_activated = false;
    strcpy(g_app_ctrl.current_mode, "none");

    g_app_initialized = true;
    rt_kprintf("[App] Application controller initialized successfully\n");
    rt_kprintf("[App] System ready - encoder, HID, LED, and screen controls active\n");

    return 0;
}

int app_controller_deinit(void)
{
    if (!g_app_initialized) {
        return 0;
    }

    rt_kprintf("[App] Deinitializing application controller...\n");

    encoder_context_deactivate();
    encoder_context_deinit();
    encoder_controller_deinit();
    rt_kprintf("[App] Encoder system deinitialized\n");

    screen_context_deactivate_all();
    screen_context_deinit_all();
    rt_kprintf("[App] Screen contexts deinitialized\n");

    led_context_deinit();
    rt_kprintf("[App] LED context deinitialized\n");

    led_controller_deinit();
    rt_kprintf("[App] LED controller deinitialized\n");

    key_manager_deinit();
    rt_kprintf("[App] Key manager deinitialized\n");

    g_app_initialized = false;
    g_app_ctrl.initialized = false;
    rt_kprintf("[App] Application controller deinitialized\n");

    return 0;
}

int app_controller_set_encoder_mode(encoder_mode_t mode)
{
    if (!g_app_ctrl.initialized) {
        rt_kprintf("[app_ctrl] Not initialized\n");
        return -RT_ERROR;
    }
    
    return encoder_controller_set_mode(mode);
}

encoder_mode_t app_controller_get_encoder_mode(void)
{
    if (!g_app_ctrl.initialized) {
        return ENCODER_MODE_IDLE;
    }
    
    return encoder_controller_get_mode();
}

bool app_controller_is_encoder_available(void)
{
    return g_app_ctrl.encoder_enabled && encoder_controller_is_ready();
}

int app_controller_switch_mode(const char *mode_name)
{
    if (!mode_name || !g_app_ctrl.initialized) {
        rt_kprintf("[app_ctrl] Invalid parameter or not initialized\n");
        return -RT_EINVAL;
    }

    key_context_id_t new_ctx = KEY_CTX_NONE;
    int ret = 0;

    if (strcmp(mode_name, "hid") == 0) {
        new_ctx = KEY_CTX_HID_SHORTCUT;
    } else if (strcmp(mode_name, "encoder") == 0) {
        if (!g_app_ctrl.encoder_enabled) {
            rt_kprintf("[app_ctrl] Encoder not available\n");
            return -RT_ERROR;
        }
        new_ctx = KEY_CTX_VOLUME_CONTROL;
    } else if (strcmp(mode_name, "none") == 0) {
        new_ctx = KEY_CTX_NONE;
    } else {
        rt_kprintf("[app_ctrl] Unsupported mode: %s (supported: 'hid', 'encoder', 'none')\n", mode_name);
        return -RT_EINVAL;
    }

    if (new_ctx == KEY_CTX_VOLUME_CONTROL) {
        key_manager_activate_context(KEY_CTX_NONE);
        ret = encoder_context_activate();
        if (ret == 0) {
            strcpy(g_app_ctrl.current_mode, mode_name);
            rt_kprintf("[app_ctrl] Switched to encoder mode\n");
        }
    } else {
        if (strcmp(g_app_ctrl.current_mode, "encoder") == 0) {
            encoder_context_deactivate();
        }
        
        ret = key_manager_activate_context(new_ctx);
        if (ret == 0) {
            strcpy(g_app_ctrl.current_mode, mode_name);
            rt_kprintf("[app_ctrl] Switched to mode: %s\n", mode_name);
            
            g_app_ctrl.hid_context_activated = (new_ctx == KEY_CTX_HID_SHORTCUT);
            
            if (new_ctx == KEY_CTX_HID_SHORTCUT) {
                g_app_ctrl.hid_context_activated = (new_ctx == KEY_CTX_HID_SHORTCUT);
            }
        }
    }

    if (ret != 0) {
        rt_kprintf("[app_ctrl] Failed to switch to mode %s: %d\n", mode_name, ret);
    }

    return ret;
}

const char* app_controller_get_current_mode(void)
{
    return g_app_ctrl.current_mode;
}

bool app_controller_is_hid_activated(void)
{
    return g_app_ctrl.hid_context_activated;
}

int app_controller_force_activate_hid(void)
{
    return app_controller_switch_mode("hid");
}

int app_controller_force_activate_none(void)
{
    return app_controller_switch_mode("none");
}