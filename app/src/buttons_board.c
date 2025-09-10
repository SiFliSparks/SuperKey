#include "buttons_board.h"
#include "bf0_hal.h"
#include <board.h>
#include <string.h>

#ifndef BUTTON_ADV_ACTION_CHECK_DELAY
#define BUTTON_ADV_ACTION_CHECK_DELAY 800
#endif

static const struct {
    int32_t pin;
    int pad;
    pin_function func;
} s_button_configs[BUTTON_COUNT] = {
    {26, PAD_PA26, GPIO_A26},
    {33, PAD_PA33, GPIO_A33},
    {32, PAD_PA32, GPIO_A32},
    {40, PAD_PA40, GPIO_A40}
};

static int32_t s_button_ids[BUTTON_COUNT] = {-1, -1, -1, -1};
static button_handler_t s_unified_callback = NULL;

static void sdk_button_adapter(int32_t pin, button_action_t action)
{
    if (s_unified_callback) {
        s_unified_callback(pin, action);
    }
}

int buttons_board_init(button_handler_t unified_callback)
{
    if (!unified_callback) {
        rt_kprintf("[ButtonsBoard] Invalid callback\n");
        return -RT_EINVAL;
    }
    
    s_unified_callback = unified_callback;
    
    rt_kprintf("[ButtonsBoard] Initializing buttons with SDK driver...\n");
    
    for (int i = 0; i < BUTTON_COUNT; i++) {
        HAL_PIN_Set(s_button_configs[i].pad, s_button_configs[i].func, PIN_PULLUP, 1);
        
        button_cfg_t cfg = {
            .pin = s_button_configs[i].pin,
            .active_state = BUTTON_ACTIVE_LOW,
            .mode = PIN_MODE_INPUT_PULLUP,
            .button_handler = sdk_button_adapter,
            .debounce_time = 2
        };
        
        s_button_ids[i] = button_init(&cfg);
        if (s_button_ids[i] < 0) {
            rt_kprintf("[ButtonsBoard] Failed to init button %d: %d\n", i, s_button_ids[i]);
            for (int j = 0; j < i; j++) {
                if (s_button_ids[j] >= 0) {
                    button_disable(s_button_ids[j]);
                    s_button_ids[j] = -1;
                }
            }
            return -RT_ERROR;
        }
        
        if (button_enable(s_button_ids[i]) != SF_EOK) {
            rt_kprintf("[ButtonsBoard] Failed to enable button %d\n", i);
            return -RT_ERROR;
        }
    }
    
    rt_kprintf("[ButtonsBoard] Successfully initialized %d buttons using SDK driver\n", BUTTON_COUNT);
    return RT_EOK;
}

int buttons_board_pin_to_idx(int32_t pin)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (s_button_configs[i].pin == pin) {
            return i;
        }
    }
    return -1;
}

int buttons_board_count(void)
{
    return BUTTON_COUNT;
}

int buttons_board_deinit(void)
{
    rt_kprintf("[ButtonsBoard] Deinitializing buttons...\n");
    
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (s_button_ids[i] >= 0) {
            button_disable(s_button_ids[i]);
            s_button_ids[i] = -1;
        }
    }
    
    s_unified_callback = NULL;
    rt_kprintf("[ButtonsBoard] Buttons deinitialized\n");
    return RT_EOK;
}

int buttons_board_enable(int key_idx)
{
    if (key_idx < 0 || key_idx >= BUTTON_COUNT || s_button_ids[key_idx] < 0) {
        return -RT_EINVAL;
    }
    
    return (button_enable(s_button_ids[key_idx]) == SF_EOK) ? RT_EOK : -RT_ERROR;
}

int buttons_board_disable(int key_idx)
{
    if (key_idx < 0 || key_idx >= BUTTON_COUNT || s_button_ids[key_idx] < 0) {
        return -RT_EINVAL;
    }
    
    return (button_disable(s_button_ids[key_idx]) == SF_EOK) ? RT_EOK : -RT_ERROR;
}

bool buttons_board_is_pressed(int key_idx)
{
    if (key_idx < 0 || key_idx >= BUTTON_COUNT || s_button_ids[key_idx] < 0) {
        return false;
    }
    
    return button_is_pressed(s_button_ids[key_idx]);
}