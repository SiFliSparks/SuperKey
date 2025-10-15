#include "app_controller.h"
#include "key_manager.h"    
#include "encoder_controller.h"    
#include "hid_device.h"
#include "screen_context.h"
#include "screen.h"
#include "event_bus.h"
#include <string.h>
#include "led_compat.h"
#include <board.h>
#include "bf0_hal.h"
#include "rtthread.h"



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
        return 0;
    }

    if (event_bus_init() != 0) {
        return -1;
    }

    hid_device_init(0, HAL_Get_USB_Base());
    
    if (key_manager_init() != 0) {
        return -1;
    }

    if (encoder_controller_init() != 0) {
        return -1;
    }

    if (screen_context_init_all() != 0) {
        return -1;
    }

    screen_group_t current_group = screen_get_current_group();

    g_app_ctrl.initialized = true;
    g_app_ctrl.encoder_enabled = true;
    g_app_ctrl.hid_context_activated = false;
    strcpy(g_app_ctrl.current_mode, "none");

    g_app_initialized = true;

    return 0;
}

int app_controller_deinit(void)
{
    if (!g_app_initialized) {
        return 0;
    }
    encoder_controller_deinit();

    screen_context_deactivate_all();
    screen_context_deinit_all();


    key_manager_deinit();

    g_app_initialized = false;
    g_app_ctrl.initialized = false;

    return 0;
}

int app_controller_set_encoder_mode(encoder_mode_t mode)
{
    if (!g_app_ctrl.initialized) {
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
    key_context_id_t new_ctx = KEY_CTX_NONE;
    
    if (strcmp(mode_name, "hid") == 0) {
        new_ctx = KEY_CTX_HID_SHORTCUT;
    } else if (strcmp(mode_name, "none") == 0) {
        new_ctx = KEY_CTX_NONE;
    } else {
        return -RT_EINVAL;
    }
    
    int ret = key_manager_activate_context(new_ctx);
    if (ret == 0) {
        strcpy(g_app_ctrl.current_mode, mode_name);
        g_app_ctrl.hid_context_activated = (new_ctx == KEY_CTX_HID_SHORTCUT);
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