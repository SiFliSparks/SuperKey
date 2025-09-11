#include "light_effects.h"
#include "rtthread.h"
#include "bf0_hal.h"
#include "drivers/rt_drv_pwm.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

// 直接使用硬件驱动命令（从工作例程和驱动复制）
#define RGBLED_NAME              "rgbled"
#define LED_COUNT                3

// 硬件驱动命令
#define PWM_CMD_SET_LED_COUNT    (128 + 8)   // Initialize LED strip
#define PWM_CMD_SET_LED_COLOR    (128 + 9)   // Set single LED color
#define PWM_CMD_SET_ALL_LEDS     (128 + 10)  // Set all LEDs to same color  
#define PWM_CMD_UPDATE_STRIP     (128 + 11)  // Update LED strip display
#define PWM_CMD_CLEAR_ALL_LEDS   (128 + 12)  // Clear all LEDs

// 硬件驱动配置结构（从驱动复制）
struct rt_rgbled_multi_config
{
    rt_uint16_t led_index;      // LED index for single LED operations
    rt_uint32_t color;          // Color value
    rt_uint16_t led_count;      // Number of LEDs (for initialization)
    rt_uint32_t *color_array;   // Array of colors for batch operations
    rt_uint16_t start_index;    // Starting index for partial updates
    rt_uint16_t update_count;   // Number of LEDs to update
};

// LED效果上下文
typedef struct {
    struct rt_device *rgbled_device;    // RGB LED设备句柄
    light_effect_config_t config;       // 当前配置
    light_effect_status_t status;       // 当前状态
    rt_mutex_t lock;                    // 互斥锁
    bool initialized;                   // 初始化标志
    bool paused;                        // 暂停标志
    
    // 流水灯数据（基于chasing_light效果）
    struct {
        int current_position;
        int cycle_step;
    } flowing_data;
    
    // 呼吸灯数据（基于breathing_sync效果）
    struct {
        int brightness;
        bool increasing;
    } breathing_data;
    
    // 闪烁数据
    struct {
        bool is_on;
        rt_tick_t phase_start;
    } flash_data;
    
    // 彩虹数据（基于rainbow_rotation效果）
    struct {
        int offset;
    } rainbow_data;
    
} light_effects_context_t;

static light_effects_context_t g_effects_ctx = {0};

// 内部函数声明
static rt_err_t hardware_led_init(rt_uint16_t led_count);
static rt_err_t hardware_led_set_color(rt_uint16_t index, rt_uint32_t color);
static rt_err_t hardware_led_set_all(rt_uint32_t color);
static rt_err_t hardware_led_update(void);
static rt_err_t hardware_led_clear(void);
static uint32_t hsv_to_rgb_hardware(float h, float s, float v);
static void effect_update_flowing_hardware(void);
static void effect_update_breathing_hardware(void);
static void effect_update_flash_hardware(void);
static void effect_update_rainbow_hardware(void);
static bool check_cycles_completed(void);
static void reset_effect_data(void);

/**
 * @brief 硬件LED初始化（基于工作例程的multi_led_init）
 */
static rt_err_t hardware_led_init(rt_uint16_t led_count)
{
    struct rt_rgbled_multi_config config;
    config.led_count = led_count;
    
    rt_err_t result = rt_device_control(g_effects_ctx.rgbled_device, PWM_CMD_SET_LED_COUNT, &config);
    if (result == RT_EOK) {
        rt_kprintf("[LightEffects] LED strip initialized with %d LEDs\n", led_count);
    } else {
        rt_kprintf("[LightEffects] Failed to initialize LED strip\n");
    }
    return result;
}

/**
 * @brief 设置单个LED颜色（基于工作例程的multi_led_set_color）
 */
static rt_err_t hardware_led_set_color(rt_uint16_t index, rt_uint32_t color)
{
    struct rt_rgbled_multi_config config;
    config.led_index = index;
    config.color = color;
    
    return rt_device_control(g_effects_ctx.rgbled_device, PWM_CMD_SET_LED_COLOR, &config);
}

/**
 * @brief 设置所有LED相同颜色（基于工作例程的multi_led_set_all）
 */
static rt_err_t hardware_led_set_all(rt_uint32_t color)
{
    struct rt_rgbled_multi_config config;
    config.color = color;
    
    return rt_device_control(g_effects_ctx.rgbled_device, PWM_CMD_SET_ALL_LEDS, &config);
}

/**
 * @brief 更新LED显示（基于工作例程的multi_led_update）
 */
static rt_err_t hardware_led_update(void)
{
    return rt_device_control(g_effects_ctx.rgbled_device, PWM_CMD_UPDATE_STRIP, NULL);
}

/**
 * @brief 清除所有LED（基于工作例程的multi_led_clear）
 */
static rt_err_t hardware_led_clear(void)
{
    rt_err_t result = rt_device_control(g_effects_ctx.rgbled_device, PWM_CMD_CLEAR_ALL_LEDS, NULL);
    if (result == RT_EOK) {
        result = hardware_led_update();
    }
    return result;
}

/**
 * @brief HSV转RGB（从工作例程复制）
 */
static uint32_t hsv_to_rgb_hardware(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    uint8_t red = (uint8_t)((r + m) * 255);
    uint8_t green = (uint8_t)((g + m) * 255);
    uint8_t blue = (uint8_t)((b + m) * 255);

    return (red << 16) | (green << 8) | blue;
}

/**
 * @brief 检查循环是否完成
 */
static bool check_cycles_completed(void)
{
    if (g_effects_ctx.config.cycle_count == 0) {
        return false;  // 无限循环
    }
    return g_effects_ctx.status.current_cycle >= g_effects_ctx.config.cycle_count;
}

/**
 * @brief 重置效果数据
 */
static void reset_effect_data(void)
{
    memset(&g_effects_ctx.flowing_data, 0, sizeof(g_effects_ctx.flowing_data));
    memset(&g_effects_ctx.breathing_data, 0, sizeof(g_effects_ctx.breathing_data));
    memset(&g_effects_ctx.flash_data, 0, sizeof(g_effects_ctx.flash_data));
    memset(&g_effects_ctx.rainbow_data, 0, sizeof(g_effects_ctx.rainbow_data));
}

/**
 * @brief 流水灯效果更新（修复中间LED闪烁问题）
 */
static void effect_update_flowing_hardware(void)
{
    rt_tick_t now = rt_tick_get();
    if ((now - g_effects_ctx.status.last_update) < rt_tick_from_millisecond(g_effects_ctx.config.speed_ms)) {
        return;
    }
    
    g_effects_ctx.status.last_update = now;
    g_effects_ctx.status.step_count++;
    
    int pos = g_effects_ctx.flowing_data.current_position;
    uint32_t main_color = g_effects_ctx.config.primary_color;
    
    // 清除所有LED
    hardware_led_clear();
    
    if (g_effects_ctx.config.direction == FLOW_DIR_PINGPONG) {
        // 乒乓球模式 - 简化逻辑避免闪烁
        
        // 判断当前方向
        bool moving_right = (g_effects_ctx.flowing_data.cycle_step < LED_COUNT);
        
        if (moving_right) {
            // 从左到右阶段
            int current_pos = g_effects_ctx.flowing_data.cycle_step;
            
            // 设置当前LED（最亮）
            hardware_led_set_color(current_pos, main_color);
            
            // 设置拖尾效果
            if (current_pos > 0) {
                uint32_t trail_color1 = (main_color & 0xFEFEFE) >> 1;  // 50% 亮度
                hardware_led_set_color(current_pos - 1, trail_color1);
            }
            if (current_pos > 1) {
                uint32_t trail_color2 = (main_color & 0xFCFCFC) >> 2;  // 25% 亮度
                hardware_led_set_color(current_pos - 2, trail_color2);
            }
            
            g_effects_ctx.flowing_data.cycle_step++;
            rt_kprintf("[LightEffects] Moving right: step=%d, pos=%d\n", g_effects_ctx.flowing_data.cycle_step, current_pos);
        } else {
            // 从右到左阶段
            int current_pos = (LED_COUNT * 2 - 1) - g_effects_ctx.flowing_data.cycle_step;
            
            // 设置当前LED（最亮）
            hardware_led_set_color(current_pos, main_color);
            
            // 设置拖尾效果
            if (current_pos < LED_COUNT - 1) {
                uint32_t trail_color1 = (main_color & 0xFEFEFE) >> 1;  // 50% 亮度
                hardware_led_set_color(current_pos + 1, trail_color1);
            }
            if (current_pos < LED_COUNT - 2) {
                uint32_t trail_color2 = (main_color & 0xFCFCFC) >> 2;  // 25% 亮度
                hardware_led_set_color(current_pos + 2, trail_color2);
            }
            
            g_effects_ctx.flowing_data.cycle_step++;
            rt_kprintf("[LightEffects] Moving left: step=%d, pos=%d\n", g_effects_ctx.flowing_data.cycle_step, current_pos);
        }
        
        // 检查是否完成一个完整的乒乓球循环
        if (g_effects_ctx.flowing_data.cycle_step >= (LED_COUNT * 2 - 1)) {
            g_effects_ctx.flowing_data.cycle_step = 0;
            g_effects_ctx.status.current_cycle++;
            rt_kprintf("[LightEffects] Flowing PingPong cycle %u completed\n", g_effects_ctx.status.current_cycle);
        }
        
    } else if (g_effects_ctx.config.direction == FLOW_DIR_LEFT_TO_RIGHT) {
        // 从左到右
        hardware_led_set_color(pos, main_color);
        
        // 拖尾效果
        if (pos > 0) {
            uint32_t trail_color = (main_color & 0xFEFEFE) >> 1;  // 50% 亮度
            hardware_led_set_color(pos - 1, trail_color);
        }
        
        g_effects_ctx.flowing_data.current_position++;
        if (g_effects_ctx.flowing_data.current_position >= LED_COUNT) {
            g_effects_ctx.flowing_data.current_position = 0;
            g_effects_ctx.status.current_cycle++;
            rt_kprintf("[LightEffects] Flowing L2R cycle %u completed\n", g_effects_ctx.status.current_cycle);
        }
    } else {
        // 从右到左
        hardware_led_set_color(pos, main_color);
        
        // 拖尾效果
        if (pos < LED_COUNT - 1) {
            uint32_t trail_color = (main_color & 0xFEFEFE) >> 1;  // 50% 亮度
            hardware_led_set_color(pos + 1, trail_color);
        }
        
        g_effects_ctx.flowing_data.current_position--;
        if (g_effects_ctx.flowing_data.current_position < 0) {
            g_effects_ctx.flowing_data.current_position = LED_COUNT - 1;
            g_effects_ctx.status.current_cycle++;
            rt_kprintf("[LightEffects] Flowing R2L cycle %u completed\n", g_effects_ctx.status.current_cycle);
        }
    }
    
    // 更新LED显示
    hardware_led_update();
    
    // 检查是否完成所有循环
    if (check_cycles_completed()) {
        rt_kprintf("[LightEffects] All flowing cycles completed (%u/%u), stopping\n", 
                  g_effects_ctx.status.current_cycle, g_effects_ctx.config.cycle_count);
        hardware_led_clear();
        g_effects_ctx.status.is_running = false;
        g_effects_ctx.status.current_type = LIGHT_EFFECT_NONE;
    }
}

/**
 * @brief 呼吸灯效果更新（基于工作例程的effect_breathing_sync）
 */
static void effect_update_breathing_hardware(void)
{
    rt_tick_t now = rt_tick_get();
    if ((now - g_effects_ctx.status.last_update) < rt_tick_from_millisecond(50)) {
        return;  // 50ms更新一次
    }
    
    g_effects_ctx.status.last_update = now;
    
    int step_size = 5;
    
    if (g_effects_ctx.breathing_data.increasing) {
        g_effects_ctx.breathing_data.brightness += step_size;
        if (g_effects_ctx.breathing_data.brightness >= g_effects_ctx.config.brightness) {
            g_effects_ctx.breathing_data.brightness = g_effects_ctx.config.brightness;
            g_effects_ctx.breathing_data.increasing = false;
        }
    } else {
        g_effects_ctx.breathing_data.brightness -= step_size;
        if (g_effects_ctx.breathing_data.brightness <= 0) {
            g_effects_ctx.breathing_data.brightness = 0;
            g_effects_ctx.breathing_data.increasing = true;
            g_effects_ctx.status.current_cycle++;
            rt_kprintf("[LightEffects] Breathing cycle %u completed\n", g_effects_ctx.status.current_cycle);
        }
    }
    
    // 计算当前颜色（基于工作例程的方法）
    uint32_t base_color = g_effects_ctx.config.primary_color;
    float brightness_factor = (float)g_effects_ctx.breathing_data.brightness / g_effects_ctx.config.brightness;
    
    uint8_t r = ((base_color >> 16) & 0xFF) * brightness_factor;
    uint8_t g = ((base_color >> 8) & 0xFF) * brightness_factor;
    uint8_t b = (base_color & 0xFF) * brightness_factor;
    
    uint32_t current_color = (r << 16) | (g << 8) | b;
    
    // 设置所有LED为相同颜色（三个灯一起亮一起灭）
    hardware_led_set_all(current_color);
    hardware_led_update();
    
    // 检查是否完成所有循环
    if (check_cycles_completed()) {
        rt_kprintf("[LightEffects] All breathing cycles completed (%u/%u), stopping\n", 
                  g_effects_ctx.status.current_cycle, g_effects_ctx.config.cycle_count);
        hardware_led_clear();
        g_effects_ctx.status.is_running = false;
        g_effects_ctx.status.current_type = LIGHT_EFFECT_NONE;
    }
}

/**
 * @brief 闪烁效果更新
 */
static void effect_update_flash_hardware(void)
{
    rt_tick_t now = rt_tick_get();
    rt_tick_t phase_duration = g_effects_ctx.flash_data.is_on ? 
        rt_tick_from_millisecond(g_effects_ctx.config.speed_ms) :
        rt_tick_from_millisecond(g_effects_ctx.config.secondary_color);
    
    if ((now - g_effects_ctx.flash_data.phase_start) < phase_duration) {
        return;
    }
    
    g_effects_ctx.flash_data.is_on = !g_effects_ctx.flash_data.is_on;
    g_effects_ctx.flash_data.phase_start = now;
    g_effects_ctx.status.last_update = now;
    
    if (g_effects_ctx.flash_data.is_on) {
        // 点亮所有LED
        hardware_led_set_all(g_effects_ctx.config.primary_color);
        hardware_led_update();
    } else {
        // 关闭所有LED，完成一个闪烁循环
        hardware_led_clear();
        g_effects_ctx.status.current_cycle++;
        rt_kprintf("[LightEffects] Flash cycle %u completed\n", g_effects_ctx.status.current_cycle);
        
        // 检查是否完成所有循环
        if (check_cycles_completed()) {
            rt_kprintf("[LightEffects] All flash cycles completed (%u/%u), stopping\n", 
                      g_effects_ctx.status.current_cycle, g_effects_ctx.config.cycle_count);
            hardware_led_clear();
            g_effects_ctx.status.is_running = false;
            g_effects_ctx.status.current_type = LIGHT_EFFECT_NONE;
        }
    }
}

/**
 * @brief 彩虹效果更新（基于工作例程的effect_rainbow_rotation）
 */
static void effect_update_rainbow_hardware(void)
{
    rt_tick_t now = rt_tick_get();
    if ((now - g_effects_ctx.status.last_update) < rt_tick_from_millisecond(g_effects_ctx.config.speed_ms)) {
        return;
    }
    
    g_effects_ctx.status.last_update = now;
    
    // 为每个LED设置不同的彩虹颜色
    for (int i = 0; i < LED_COUNT; i++) {
        float hue = fmodf((float)i / LED_COUNT * 360.0f + g_effects_ctx.rainbow_data.offset, 360.0f);
        uint32_t color = hsv_to_rgb_hardware(hue, 1.0f, 0.3f);  // 使用0.3f避免过亮
        hardware_led_set_color(i, color);
    }
    
    hardware_led_update();
    
    // 更新偏移
    g_effects_ctx.rainbow_data.offset += 10;
    if (g_effects_ctx.rainbow_data.offset >= 360) {
        g_effects_ctx.rainbow_data.offset = 0;
        g_effects_ctx.status.current_cycle++;
        rt_kprintf("[LightEffects] Rainbow cycle %u completed\n", g_effects_ctx.status.current_cycle);
        
        // 检查是否完成所有循环
        if (check_cycles_completed()) {
            rt_kprintf("[LightEffects] All rainbow cycles completed (%u/%u), stopping\n", 
                      g_effects_ctx.status.current_cycle, g_effects_ctx.config.cycle_count);
            hardware_led_clear();
            g_effects_ctx.status.is_running = false;
            g_effects_ctx.status.current_type = LIGHT_EFFECT_NONE;
        }
    }
}

// === 公共API实现 ===

int light_effects_init(void)
{
    if (g_effects_ctx.initialized) {
        return 0;
    }
    
    rt_kprintf("[LightEffects] Initializing light effects module...\n");
    
    // 清零上下文
    memset(&g_effects_ctx, 0, sizeof(light_effects_context_t));
    
    // 查找RGB LED设备
    g_effects_ctx.rgbled_device = rt_device_find(RGBLED_NAME);
    if (!g_effects_ctx.rgbled_device) {
        rt_kprintf("[LightEffects] Error: Cannot find RGB LED device!\n");
        return -RT_ERROR;
    }
    
    // 初始化LED条带
    if (hardware_led_init(LED_COUNT) != RT_EOK) {
        rt_kprintf("[LightEffects] Failed to initialize LED strip\n");
        return -RT_ERROR;
    }
    
    // 创建互斥锁
    g_effects_ctx.lock = rt_mutex_create("light_fx", RT_IPC_FLAG_PRIO);
    if (!g_effects_ctx.lock) {
        rt_kprintf("[LightEffects] Failed to create mutex\n");
        return -RT_ENOMEM;
    }
    
    // 初始化状态
    g_effects_ctx.status.current_type = LIGHT_EFFECT_NONE;
    g_effects_ctx.status.is_running = false;
    g_effects_ctx.initialized = true;
    g_effects_ctx.paused = false;
    
    reset_effect_data();
    
    rt_kprintf("[LightEffects] Light effects module initialized successfully\n");
    return 0;
}

int light_effects_deinit(void)
{
    if (!g_effects_ctx.initialized) {
        return 0;
    }
    
    rt_kprintf("[LightEffects] Deinitializing light effects module...\n");
    
    // 停止当前效果
    light_effects_stop();
    
    // 释放资源
    if (g_effects_ctx.lock) {
        rt_mutex_delete(g_effects_ctx.lock);
        g_effects_ctx.lock = NULL;
    }
    
    g_effects_ctx.initialized = false;
    rt_kprintf("[LightEffects] Light effects module deinitialized\n");
    
    return 0;
}

int light_effects_start(const light_effect_config_t *config)
{
    if (!g_effects_ctx.initialized || !config) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    
    // 停止当前效果
    light_effects_stop();
    
    // 复制配置
    memcpy(&g_effects_ctx.config, config, sizeof(light_effect_config_t));
    
    // 重置效果数据
    reset_effect_data();
    
    // 初始化状态
    g_effects_ctx.status.current_type = config->type;
    g_effects_ctx.status.is_running = true;
    g_effects_ctx.status.start_time = rt_tick_get();
    g_effects_ctx.status.last_update = 0;
    g_effects_ctx.status.step_count = 0;
    g_effects_ctx.status.current_cycle = 0;
    g_effects_ctx.status.total_cycles = config->cycle_count;
    g_effects_ctx.paused = false;
    
    // 特殊初始化
    if (config->type == LIGHT_EFFECT_FLASH) {
        g_effects_ctx.flash_data.phase_start = rt_tick_get();
        g_effects_ctx.flash_data.is_on = true;
    }
    if (config->type == LIGHT_EFFECT_BREATHING) {
        g_effects_ctx.breathing_data.increasing = true;
        g_effects_ctx.breathing_data.brightness = 0;
    }
    
    rt_mutex_release(g_effects_ctx.lock);
    
    rt_kprintf("[LightEffects] Started effect type %d with color 0x%06X, cycles: %u\n", 
              config->type, config->primary_color, config->cycle_count);
    
    return 0;
}

int light_effects_stop(void)
{
    if (!g_effects_ctx.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    
    g_effects_ctx.status.is_running = false;
    g_effects_ctx.status.current_type = LIGHT_EFFECT_NONE;
    g_effects_ctx.paused = false;
    
    // 清除所有LED
    hardware_led_clear();
    
    rt_mutex_release(g_effects_ctx.lock);
    
    rt_kprintf("[LightEffects] Effect stopped and all LEDs cleared\n");
    return 0;
}

int light_effects_pause(void)
{
    if (!g_effects_ctx.initialized || !g_effects_ctx.status.is_running) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    g_effects_ctx.paused = true;
    rt_mutex_release(g_effects_ctx.lock);
    
    return 0;
}

int light_effects_resume(void)
{
    if (!g_effects_ctx.initialized || !g_effects_ctx.status.is_running) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    g_effects_ctx.paused = false;
    rt_mutex_release(g_effects_ctx.lock);
    
    return 0;
}

void light_effects_update(void)
{
    if (!g_effects_ctx.initialized || !g_effects_ctx.status.is_running || g_effects_ctx.paused) {
        return;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    
    // 根据效果类型调用相应的更新函数
    switch (g_effects_ctx.status.current_type) {
        case LIGHT_EFFECT_FLOWING_LR:
        case LIGHT_EFFECT_FLOWING_RL:
        case LIGHT_EFFECT_FLOWING_PINGPONG:
            effect_update_flowing_hardware();
            break;
            
        case LIGHT_EFFECT_BREATHING:
            effect_update_breathing_hardware();
            break;
            
        case LIGHT_EFFECT_FLASH:
            effect_update_flash_hardware();
            break;
            
        case LIGHT_EFFECT_RAINBOW:
            effect_update_rainbow_hardware();
            break;
            
        default:
            break;
    }
    
    rt_mutex_release(g_effects_ctx.lock);
}

int light_effects_get_status(light_effect_status_t *status)
{
    if (!g_effects_ctx.initialized || !status) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    memcpy(status, &g_effects_ctx.status, sizeof(light_effect_status_t));
    rt_mutex_release(g_effects_ctx.lock);
    
    return 0;
}

bool light_effects_is_running(void)
{
    return g_effects_ctx.initialized && g_effects_ctx.status.is_running;
}

// === 快捷函数实现（支持循环次数） ===

int light_effects_flowing(uint32_t color, uint32_t speed_ms, flow_direction_t direction, 
                         uint8_t tail_length, uint32_t cycles)
{
    light_effect_config_t config = LIGHT_EFFECT_CONFIG_DEFAULT();
    config.type = (direction == FLOW_DIR_PINGPONG) ? LIGHT_EFFECT_FLOWING_PINGPONG :
                  (direction == FLOW_DIR_RIGHT_TO_LEFT) ? LIGHT_EFFECT_FLOWING_RL : LIGHT_EFFECT_FLOWING_LR;
    config.primary_color = color;
    config.speed_ms = speed_ms;
    config.direction = direction;
    config.tail_length = tail_length;
    config.cycle_count = cycles;
    
    return light_effects_start(&config);
}

int light_effects_breathing(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t cycles)
{
    light_effect_config_t config = LIGHT_EFFECT_CONFIG_DEFAULT();
    config.type = LIGHT_EFFECT_BREATHING;
    config.primary_color = color;
    config.speed_ms = period_ms;
    config.brightness = brightness;
    config.cycle_count = cycles;
    
    return light_effects_start(&config);
}

int light_effects_flash(uint32_t color, uint32_t on_time_ms, uint32_t off_time_ms, uint32_t cycles)
{
    light_effect_config_t config = LIGHT_EFFECT_CONFIG_DEFAULT();
    config.type = LIGHT_EFFECT_FLASH;
    config.primary_color = color;
    config.speed_ms = on_time_ms;
    config.secondary_color = off_time_ms;
    config.cycle_count = cycles;
    
    return light_effects_start(&config);
}

int light_effects_rainbow(uint32_t speed_ms, uint32_t cycles)
{
    light_effect_config_t config = LIGHT_EFFECT_CONFIG_DEFAULT();
    config.type = LIGHT_EFFECT_RAINBOW;
    config.speed_ms = speed_ms;
    config.cycle_count = cycles;
    
    return light_effects_start(&config);
}

int light_effects_wave(uint32_t color, uint32_t speed_ms, uint8_t intensity, uint32_t cycles)
{
    // 波浪效果暂时用彩虹效果代替
    return light_effects_rainbow(speed_ms, cycles);
}

int light_effects_static(uint32_t color, uint8_t brightness)
{
    // 计算带亮度的颜色
    float brightness_factor = (float)brightness / 255.0f;
    uint8_t r = ((color >> 16) & 0xFF) * brightness_factor;
    uint8_t g = ((color >> 8) & 0xFF) * brightness_factor;
    uint8_t b = (color & 0xFF) * brightness_factor;
    uint32_t dimmed_color = (r << 16) | (g << 8) | b;
    
    // 直接设置所有LED
    hardware_led_set_all(dimmed_color);
    hardware_led_update();
    
    return 0;
}

// === 简化版快捷函数（默认执行一次） ===

int light_effects_flowing_once(uint32_t color, uint32_t speed_ms, flow_direction_t direction, uint8_t tail_length)
{
    return light_effects_flowing(color, speed_ms, direction, tail_length, LIGHT_CYCLES_ONCE);
}

int light_effects_breathing_once(uint32_t color, uint32_t period_ms, uint8_t brightness)
{
    return light_effects_breathing(color, period_ms, brightness, LIGHT_CYCLES_ONCE);
}

int light_effects_flash_once(uint32_t color, uint32_t on_time_ms, uint32_t off_time_ms)
{
    return light_effects_flash(color, on_time_ms, off_time_ms, LIGHT_CYCLES_ONCE);
}

int light_effects_rainbow_once(uint32_t speed_ms)
{
    return light_effects_rainbow(speed_ms, LIGHT_CYCLES_ONCE);
}

int light_effects_wave_once(uint32_t color, uint32_t speed_ms, uint8_t intensity)
{
    return light_effects_wave(color, speed_ms, intensity, LIGHT_CYCLES_ONCE);
}

// === 其他函数实现 ===

int light_effects_change_color(uint32_t new_color)
{
    if (!g_effects_ctx.initialized || !g_effects_ctx.status.is_running) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    g_effects_ctx.config.primary_color = new_color;
    rt_mutex_release(g_effects_ctx.lock);
    
    return 0;
}

int light_effects_change_speed(uint32_t new_speed_ms)
{
    if (!g_effects_ctx.initialized || !g_effects_ctx.status.is_running) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    g_effects_ctx.config.speed_ms = new_speed_ms;
    rt_mutex_release(g_effects_ctx.lock);
    
    return 0;
}

int light_effects_change_brightness(uint8_t new_brightness)
{
    if (!g_effects_ctx.initialized || !g_effects_ctx.status.is_running) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_effects_ctx.lock, RT_WAITING_FOREVER);
    g_effects_ctx.config.brightness = new_brightness;
    rt_mutex_release(g_effects_ctx.lock);
    
    return 0;
}