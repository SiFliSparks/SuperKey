/**
 * @file widget_context.c
 * @brief 小工具按键上下文处理
 * 
 * 处理Group1第三板块的小工具按键逻辑
 */

#include "widget_manager.h"
#include "widget_storage.h"
#include "key_manager.h"
#include "screen.h"
#include "screen_core.h"
#include "event_bus.h"
#include "led_effects_manager.h"
#include <rtthread.h>
#include <string.h>
/* ============================================================================
 * 外部依赖声明
 * ============================================================================ */

/* LED映射函数 (从screen_context.c) */
static int get_led_index_for_key(int key_idx)
{
    switch (key_idx) {
        case 0: return 2;
        case 1: return 1;
        case 2: return 0;
        case 3: return 1;
        default: return key_idx;
    }
}

/* ============================================================================
 * 长按检测
 * ============================================================================ */

#define LONG_PRESS_THRESHOLD_MS  800  /* 长按阈值 */

static struct {
    bool key2_pressed;
    rt_tick_t key2_press_start;
    bool long_press_triggered;
} g_widget_key_state = {0};

/* ============================================================================
 * Group1小工具按键处理 (L1层级)
 * ============================================================================ */

/**
 * @brief Group1第三板块(key_idx=2)按键处理
 * 
 * 功能:
 * - 长按: 进入L2小工具选择页面
 * - 单击/双击: 由当前激活的小工具处理
 */
int widget_handle_group1_key(int key_idx, button_action_t action)
{
    if (key_idx != 2) {
        return -1;  /* 不是第三板块的按键 */
    }
    
    widget_type_t active_type = widget_manager_get_active_type();
    
    if (action == BUTTON_PRESSED) {
        /* 记录按下时间用于长按检测 */
        g_widget_key_state.key2_pressed = true;
        g_widget_key_state.key2_press_start = rt_tick_get();
        g_widget_key_state.long_press_triggered = false;
        return 0;
    }
    
    if (action == BUTTON_RELEASED) {
        rt_tick_t press_duration = rt_tick_get() - g_widget_key_state.key2_press_start;
        uint32_t duration_ms = press_duration * 1000 / RT_TICK_PER_SECOND;
        
        g_widget_key_state.key2_pressed = false;
        
        /* 如果已触发长按，忽略释放事件 */
        if (g_widget_key_state.long_press_triggered) {
            return 0;
        }
        
        /* 检测长按 */
        if (duration_ms >= LONG_PRESS_THRESHOLD_MS) {
            g_widget_key_state.long_press_triggered = true;
            rt_kprintf("[Widget] Long press detected, enter selector\n");
            
            /* 进入L2小工具选择页面 */
            widget_selector_activate();
            screen_enter_level2(SCREEN_L2_WIDGET_SELECTOR_GROUP, SCREEN_L2_WIDGET_SELECTOR);
            
            /* 白色LED反馈 */
            event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 0xFFFFFF, 500);
            return 0;
        }
        
        /* 短按: 交给当前激活的小工具处理 */
        switch (active_type) {
            case WIDGET_TYPE_RANDOM_NUMBER:
                widget_random_handle_key(key_idx);
                /* 蓝色LED快闪两下 */
                event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 0x0000FF, 150);
                rt_thread_mdelay(200);
                event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 0x0000FF, 150);
                break;
                
            case WIDGET_TYPE_OFF_WORK_COUNTDOWN:
                widget_offwork_handle_key(key_idx);
                /* 绿色LED反馈 */
                event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 0x00FF00, 300);
                break;
                
            case WIDGET_TYPE_WHAT_TO_EAT:
                widget_food_handle_key(key_idx);
                /* 橙色LED快闪 */
                event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 0xFF8000, 150);
                rt_thread_mdelay(200);
                event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 0xFF8000, 150);
                break;
                
            default:
                break;
        }
        
        /* 触发UI刷新 */
        screen_core_post_update_time();
        return 0;
    }
    
    return 0;
}

/* ============================================================================
 * L2小工具选择器按键处理
 * ============================================================================ */

/**
 * @brief L2小工具选择器按键处理
 * 
 * 功能:
 * - 按键0/1/2: 选择对应位置的小工具并返回L1
 * - 按键3: 直接返回L1
 * - 编码器: 左右移动选择
 */
int widget_handle_selector_key(int key_idx, button_action_t action)
{
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    widget_selector_state_t state;
    widget_selector_get_state(&state);
    
    /* LED颜色定义 */
    static const uint32_t selector_led_colors[] = {
        0x00FF00,  /* key0: 绿色 - 随机数 */
        0xFF8000,  /* key1: 橙色 - 下班倒计时 */
        0xFF00FF,  /* key2: 紫色 - 今天吃什么 */
        0xFFFFFF   /* key3: 白色 - 返回 */
    };
    
    /* LED反馈 */
    if (key_idx >= 0 && key_idx < 4) {
        event_bus_publish_led_feedback(get_led_index_for_key(key_idx), 
                                       selector_led_colors[key_idx], 500);
    }
    
    switch (key_idx) {
        case 0:  /* 选择第一个工具: 随机数 */
            widget_manager_set_active_type(WIDGET_TYPE_RANDOM_NUMBER);
            widget_selector_deactivate();
            screen_return_to_level1();
            rt_kprintf("[Selector] Selected: Random Number\n");
            break;
            
        case 1:  /* 选择第二个工具: 下班倒计时 */
            widget_manager_set_active_type(WIDGET_TYPE_OFF_WORK_COUNTDOWN);
            widget_selector_deactivate();
            screen_return_to_level1();
            rt_kprintf("[Selector] Selected: Off Work Countdown\n");
            break;
            
        case 2:  /* 选择第三个工具: 今天吃什么 */
            widget_manager_set_active_type(WIDGET_TYPE_WHAT_TO_EAT);
            widget_selector_deactivate();
            screen_return_to_level1();
            rt_kprintf("[Selector] Selected: What To Eat\n");
            break;
            
        case 3:  /* 返回L1，不改变选择 */
            widget_selector_deactivate();
            screen_return_to_level1();
            rt_kprintf("[Selector] Cancelled\n");
            break;
    }
    
    return 0;
}

/* ============================================================================
 * 编码器事件处理 (小工具选择器/设置模式)
 * ============================================================================ */

/**
 * @brief 小工具编码器事件处理
 * @param delta 编码器增量 (+1右/-1左)
 * @return 0成功, -1未处理
 */
int widget_handle_encoder(int delta)
{
    widget_selector_state_t selector_state;
    widget_selector_get_state(&selector_state);
    
    /* 如果选择器激活，处理选择移动 */
    if (selector_state.active) {
        widget_selector_move(delta);
        screen_core_post_update_time();  /* 触发UI刷新 */
        return 0;
    }
    
    /* 检查是否在设置模式 */
    widget_type_t active = widget_manager_get_active_type();
    
    if (active == WIDGET_TYPE_RANDOM_NUMBER) {
        widget_random_state_t random_state;
        widget_random_get_state(&random_state);
        
        if (random_state.in_setting_mode) {
            widget_random_change_max(delta);
            screen_core_post_update_time();
            return 0;
        }
    }
    else if (active == WIDGET_TYPE_OFF_WORK_COUNTDOWN) {
        widget_offwork_state_t offwork_state;
        widget_offwork_get_state(&offwork_state);
        
        if (offwork_state.in_setting_mode) {
            widget_offwork_change_time(delta);
            screen_core_post_update_time();
            return 0;
        }
    }
    
    return -1;  /* 未处理 */
}

/* ============================================================================
 * 初始化与注销
 * ============================================================================ */

/**
 * @brief 初始化小工具上下文
 * @return 0成功, <0失败
 */
int widget_context_init(void)
{
    int ret = widget_manager_init();
    if (ret != 0) {
        return ret;
    }
    
    memset(&g_widget_key_state, 0, sizeof(g_widget_key_state));
    
    rt_kprintf("[WidgetCtx] Initialized\n");
    return 0;
}

/**
 * @brief 反初始化小工具上下文
 */
void widget_context_deinit(void)
{
    widget_manager_deinit();
}

/**
 * @brief 获取当前小工具是否在设置模式
 * @return true在设置模式, false正常模式
 */
bool widget_is_in_setting_mode(void)
{
    widget_type_t active = widget_manager_get_active_type();
    
    if (active == WIDGET_TYPE_RANDOM_NUMBER) {
        widget_random_state_t state;
        widget_random_get_state(&state);
        return state.in_setting_mode;
    }
    else if (active == WIDGET_TYPE_OFF_WORK_COUNTDOWN) {
        widget_offwork_state_t state;
        widget_offwork_get_state(&state);
        return state.in_setting_mode;
    }
    
    return false;
}