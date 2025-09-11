#include "led_controller.h"
#include "event_bus.h"
#include "bf0_hal.h"
#include <board.h>
#include <string.h>
#include <math.h>

// RGB LED设备名称和命令定义（来自drv_rgbled.c）
#define RGBLED_NAME              "rgbled"
#define PWM_CMD_SET_LED_COUNT    (128 + 8)   // Initialize LED strip
#define PWM_CMD_SET_LED_COLOR    (128 + 9)   // Set single LED color
#define PWM_CMD_SET_ALL_LEDS     (128 + 10)  // Set all LEDs to same color  
#define PWM_CMD_UPDATE_STRIP     (128 + 11)  // Update LED strip display
#define PWM_CMD_CLEAR_ALL_LEDS   (128 + 12)  // Clear all LEDs

// RGB LED多灯配置结构（来自drv_rgbled.c）
struct rt_rgbled_multi_config
{
    rt_uint16_t led_index;      // LED index for single LED operations
    rt_uint32_t color;          // Color value
    rt_uint16_t led_count;      // Number of LEDs (for initialization)
    rt_uint32_t *color_array;   // Array of colors for batch operations
    rt_uint16_t start_index;    // Starting index for partial updates
    rt_uint16_t update_count;   // Number of LEDs to update
};

// 简化的LED控制器状态结构
typedef struct {
    led_info_t leds[LED_COUNT];            // LED信息数组
    struct rt_device *rgbled_device;       // RGB LED设备句柄
    rt_mutex_t lock;                       // 互斥锁
    bool initialized;                      // 初始化标志
    uint8_t global_brightness;             // 全局亮度
} led_controller_t;

static led_controller_t g_led_ctrl = {0};

// 内部函数声明
static int led_feedback_event_handler(const event_t *event, void *user_data);
static void rgb_led_hardware_init(void);

/**
 * @brief RGB LED硬件初始化（基于参考实现）
 */
static void rgb_led_hardware_init(void)
{
    /*rgbled poweron*/
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO3_3V3, true, true);
    HAL_PIN_Set(PAD_PA10, GPTIM2_CH1, PIN_NOPULL, 1);   // RGB LED 52x

    g_led_ctrl.rgbled_device = rt_device_find(RGBLED_NAME);
    if (!g_led_ctrl.rgbled_device) {
        rt_kprintf("[LED] Error: Cannot find RGB LED device!\n");
        return;
    }

    rt_kprintf("[LED] RGB LED device found successfully\n");
}

/**
 * @brief 直接更新LED显示（基于工作例程的模式）
 */
static rt_err_t led_direct_update(void)
{
    if (!g_led_ctrl.rgbled_device) {
        return -RT_ERROR;
    }
    
    return rt_device_control(g_led_ctrl.rgbled_device, PWM_CMD_UPDATE_STRIP, NULL);
}

/**
 * @brief 处理LED反馈事件
 */
static int led_feedback_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type == EVENT_LED_FEEDBACK_REQUEST) {
        const event_data_led_t *led_data = &event->data.led;
        
        rt_kprintf("[LED] Received feedback request: LED %d, color 0x%06X, duration %ums\n",
                  led_data->led_index, led_data->color, led_data->duration_ms);
        
        // 处理LED反馈请求
        led_controller_light_led(led_data->led_index, led_data->color, led_data->duration_ms);
        
        return 0;
    }
    
    return -1;
}

int led_controller_init(void)
{
    if (g_led_ctrl.initialized) {
        rt_kprintf("[LED] Already initialized\n");
        return 0;
    }
    
    rt_kprintf("[LED] Initializing LED controller...\n");
    
    // 清零控制器结构
    memset(&g_led_ctrl, 0, sizeof(led_controller_t));
    
    // 硬件初始化（包括设备查找）
    rgb_led_hardware_init();
    if (!g_led_ctrl.rgbled_device) {
        rt_kprintf("[LED] Failed to initialize RGB LED hardware\n");
        return -RT_ERROR;
    }
    
    // 初始化LED条带（使用rgbled驱动的初始化命令）
    struct rt_rgbled_multi_config config;
    config.led_count = LED_COUNT;
    
    rt_err_t result = rt_device_control(g_led_ctrl.rgbled_device, PWM_CMD_SET_LED_COUNT, &config);
    if (result != RT_EOK) {
        rt_kprintf("[LED] Failed to initialize LED strip with %d LEDs\n", LED_COUNT);
        return -RT_ERROR;
    }
    
    // 创建互斥锁
    g_led_ctrl.lock = rt_mutex_create("led_ctrl", RT_IPC_FLAG_PRIO);
    if (!g_led_ctrl.lock) {
        rt_kprintf("[LED] Failed to create mutex\n");
        return -RT_ENOMEM;
    }
    
    // 初始化LED状态
    for (int i = 0; i < LED_COUNT; i++) {
        g_led_ctrl.leds[i].current_color = LED_COLOR_BLACK;
        g_led_ctrl.leds[i].current_state = LED_STATE_OFF;
        g_led_ctrl.leds[i].current_brightness = 255;
        g_led_ctrl.leds[i].active = false;
        g_led_ctrl.leds[i].last_update_tick = 0;
    }
    
    g_led_ctrl.global_brightness = 255;
    
    // 订阅LED反馈事件
    event_bus_subscribe(EVENT_LED_FEEDBACK_REQUEST, led_feedback_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    
    g_led_ctrl.initialized = true;
    
    rt_kprintf("[LED] LED controller initialized successfully\n");
    rt_kprintf("[LED] LED count: %d, using rgbled driver\n", LED_COUNT);
    
    // 清除所有LED
    led_controller_turn_off_all();
    
    return 0;
}

int led_controller_deinit(void)
{
    if (!g_led_ctrl.initialized) {
        return 0;
    }
    
    rt_kprintf("[LED] Deinitializing LED controller...\n");
    
    // 取消事件订阅
    event_bus_unsubscribe(EVENT_LED_FEEDBACK_REQUEST, led_feedback_event_handler);
    
    // 关闭所有LED
    led_controller_turn_off_all();
    
    // 释放资源
    if (g_led_ctrl.lock) {
        rt_mutex_delete(g_led_ctrl.lock);
        g_led_ctrl.lock = NULL;
    }
    
    g_led_ctrl.initialized = false;
    rt_kprintf("[LED] LED controller deinitialized\n");
    
    return 0;
}

int led_controller_light_led(int led_index, uint32_t color, uint32_t duration_ms)
{
    if (!g_led_ctrl.initialized || led_index < 0 || led_index >= LED_COUNT) {
        rt_kprintf("[LED] Invalid parameters: initialized=%d, index=%d\n", 
                  g_led_ctrl.initialized, led_index);
        return -RT_EINVAL;
    }
    
    if (!g_led_ctrl.rgbled_device) {
        rt_kprintf("[LED] RGB LED device not available\n");
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_led_ctrl.lock, RT_WAITING_FOREVER);
    
    rt_kprintf("[LED] Setting LED %d to color 0x%06X\n", led_index, color);
    
    // 更新LED状态
    g_led_ctrl.leds[led_index].current_color = color;
    g_led_ctrl.leds[led_index].current_state = LED_STATE_STATIC;
    g_led_ctrl.leds[led_index].current_brightness = 255;
    g_led_ctrl.leds[led_index].active = true;
    g_led_ctrl.leds[led_index].last_update_tick = rt_tick_get();
    
    // 使用rgbled驱动设置LED颜色 - 直接模式，类似工作例程
    struct rt_rgbled_multi_config config;
    config.led_index = led_index;
    config.color = color;
    
    rt_err_t result = rt_device_control(g_led_ctrl.rgbled_device, PWM_CMD_SET_LED_COLOR, &config);
    if (result == RT_EOK) {
        // 立即更新显示 - 关键修复！
        result = led_direct_update();
        if (result == RT_EOK) {
            rt_kprintf("[LED] LED %d successfully set and updated\n", led_index);
        } else {
            rt_kprintf("[LED] Failed to update LED display\n");
        }
    } else {
        rt_kprintf("[LED] Failed to set LED %d color\n", led_index);
    }
    
    rt_mutex_release(g_led_ctrl.lock);
    
    // 如果指定了持续时间，设置自动关闭定时器
    if (duration_ms > 0) {
        rt_timer_t timeout_timer = rt_timer_create("led_timeout", 
            (void (*)(void *))led_controller_turn_off_led, 
            (void *)(rt_base_t)led_index, 
            rt_tick_from_millisecond(duration_ms), 
            RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
        
        if (timeout_timer) {
            rt_timer_start(timeout_timer);
        }
    }
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int led_controller_turn_off_led(int led_index)
{
    if (!g_led_ctrl.initialized || led_index < 0 || led_index >= LED_COUNT) {
        return -RT_EINVAL;
    }
    
    if (!g_led_ctrl.rgbled_device) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_led_ctrl.lock, RT_WAITING_FOREVER);
    
    // 更新LED状态
    g_led_ctrl.leds[led_index].current_color = LED_COLOR_BLACK;
    g_led_ctrl.leds[led_index].current_state = LED_STATE_OFF;
    g_led_ctrl.leds[led_index].active = false;
    
    // 使用rgbled驱动设置LED颜色为黑色
    struct rt_rgbled_multi_config config;
    config.led_index = led_index;
    config.color = LED_COLOR_BLACK;
    
    rt_err_t result = rt_device_control(g_led_ctrl.rgbled_device, PWM_CMD_SET_LED_COLOR, &config);
    if (result == RT_EOK) {
        // 立即更新显示
        result = led_direct_update();
    }
    
    rt_mutex_release(g_led_ctrl.lock);
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int led_controller_turn_off_all(void)
{
    if (!g_led_ctrl.initialized || !g_led_ctrl.rgbled_device) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_led_ctrl.lock, RT_WAITING_FOREVER);
    
    // 更新所有LED状态
    for (int i = 0; i < LED_COUNT; i++) {
        g_led_ctrl.leds[i].current_color = LED_COLOR_BLACK;
        g_led_ctrl.leds[i].current_state = LED_STATE_OFF;
        g_led_ctrl.leds[i].active = false;
    }
    
    // 使用rgbled驱动清除所有LED
    rt_err_t result = rt_device_control(g_led_ctrl.rgbled_device, PWM_CMD_CLEAR_ALL_LEDS, NULL);
    if (result == RT_EOK) {
        // 立即更新显示
        result = led_direct_update();
    }
    
    rt_mutex_release(g_led_ctrl.lock);
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

// 其他函数保持简化实现
int led_controller_set_brightness(int led_index, uint8_t brightness)
{
    if (!g_led_ctrl.initialized || led_index < 0 || led_index >= LED_COUNT) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_led_ctrl.lock, RT_WAITING_FOREVER);
    
    g_led_ctrl.leds[led_index].current_brightness = brightness;
    
    // 重新设置LED颜色以应用新亮度
    uint32_t original_color = g_led_ctrl.leds[led_index].current_color;
    uint8_t r = (uint8_t)((original_color & 0xFF0000) >> 16);
    uint8_t g = (uint8_t)((original_color & 0x00FF00) >> 8);
    uint8_t b = (uint8_t)((original_color & 0x0000FF) >> 0);
    
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    uint32_t adjusted_color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    
    struct rt_rgbled_multi_config config;
    config.led_index = led_index;
    config.color = adjusted_color;
    
    rt_err_t result = rt_device_control(g_led_ctrl.rgbled_device, PWM_CMD_SET_LED_COLOR, &config);
    if (result == RT_EOK) {
        result = led_direct_update();
    }
    
    rt_mutex_release(g_led_ctrl.lock);
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int led_controller_set_all_brightness(uint8_t brightness)
{
    if (!g_led_ctrl.initialized) {
        return -RT_ERROR;
    }
    
    g_led_ctrl.global_brightness = brightness;
    
    // 对每个LED应用新亮度
    for (int i = 0; i < LED_COUNT; i++) {
        led_controller_set_brightness(i, brightness);
    }
    
    return 0;
}

int led_controller_get_led_info(int led_index, led_info_t *info)
{
    if (!g_led_ctrl.initialized || led_index < 0 || led_index >= LED_COUNT || !info) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_led_ctrl.lock, RT_WAITING_FOREVER);
    *info = g_led_ctrl.leds[led_index];
    rt_mutex_release(g_led_ctrl.lock);
    
    return 0;
}

int led_controller_get_led_count(void)
{
    return LED_COUNT;
}

bool led_controller_is_initialized(void)
{
    return g_led_ctrl.initialized;
}

int led_controller_update_display(void)
{
    if (!g_led_ctrl.initialized || !g_led_ctrl.rgbled_device) {
        return -RT_ERROR;
    }
    
    return led_direct_update();
}

// 简化的效果函数实现
int led_controller_breathing_effect(int led_index, uint32_t color, uint32_t period_ms)
{
    // 简化实现：直接点亮LED
    return led_controller_light_led(led_index, color, period_ms);
}

int led_controller_flash_effect(int led_index, uint32_t color, 
                               uint32_t on_time_ms, uint32_t off_time_ms, 
                               uint32_t repeat_count)
{
    // 简化实现：直接点亮LED
    return led_controller_light_led(led_index, color, on_time_ms);
}

int led_controller_set_effect(int led_index, const led_effect_config_t *config)
{
    if (!config) return -RT_EINVAL;
    
    switch (config->state) {
        case LED_STATE_STATIC:
            return led_controller_light_led(led_index, config->color, config->duration_ms);
        case LED_STATE_BREATHING:
            return led_controller_breathing_effect(led_index, config->color, 2000);
        case LED_STATE_FLASHING:
            return led_controller_flash_effect(led_index, config->color, 500, 500, config->repeat_count);
        default:
            return led_controller_turn_off_led(led_index);
    }
}

int led_controller_rainbow_effect(uint32_t speed_ms)
{
    // 简化的彩虹效果实现
    static uint32_t rainbow_colors[] = {
        LED_COLOR_RED, LED_COLOR_ORANGE, LED_COLOR_YELLOW,
        LED_COLOR_GREEN, LED_COLOR_CYAN, LED_COLOR_BLUE,
        LED_COLOR_PURPLE, LED_COLOR_MAGENTA
    };
    
    static int color_index = 0;
    
    for (int i = 0; i < LED_COUNT; i++) {
        led_controller_light_led(i, rainbow_colors[(color_index + i) % 8], speed_ms);
    }
    
    color_index = (color_index + 1) % 8;
    return 0;
}

int led_controller_pause_effect(int led_index)
{
    // 简化实现：暂停就是关闭
    if (led_index == -1) {
        return led_controller_turn_off_all();
    } else {
        return led_controller_turn_off_led(led_index);
    }
}

int led_controller_resume_effect(int led_index)
{
    // 简化实现：恢复为静态点亮
    if (led_index == -1) {
        for (int i = 0; i < LED_COUNT; i++) {
            led_controller_light_led(i, g_led_ctrl.leds[i].current_color, 0);
        }
        return 0;
    } else {
        return led_controller_light_led(led_index, g_led_ctrl.leds[led_index].current_color, 0);
    }
}