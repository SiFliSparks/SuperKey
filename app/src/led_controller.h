#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED配置参数
#define LED_COUNT                3        // LED数量
#define LED_MAX_BRIGHTNESS      255      // 最大亮度
#define LED_DEFAULT_DURATION    200      // 默认持续时间(ms)

// 预定义颜色常量
#define LED_COLOR_BLACK         0x000000
#define LED_COLOR_OFF           LED_COLOR_BLACK  // LED_COLOR_OFF 别名
#define LED_COLOR_WHITE         0xFFFFFF
#define LED_COLOR_RED           0xFF0000
#define LED_COLOR_GREEN         0x00FF00
#define LED_COLOR_BLUE          0x0000FF
#define LED_COLOR_YELLOW        0xFFFF00
#define LED_COLOR_CYAN          0x00FFFF
#define LED_COLOR_MAGENTA       0xFF00FF
#define LED_COLOR_ORANGE        0xFF8000
#define LED_COLOR_PURPLE        0x8000FF

// LED状态枚举
typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_STATIC,
    LED_STATE_BREATHING,
    LED_STATE_FLASHING,
    LED_STATE_RAINBOW,
    LED_STATE_CUSTOM
} led_state_t;

// LED效果配置
typedef struct {
    uint32_t color;                 // 颜色值 (RGB)
    uint32_t duration_ms;          // 持续时间
    uint32_t repeat_count;         // 重复次数，0为无限循环
    uint8_t brightness;            // 亮度 (0-255)
    led_state_t state;             // LED状态
    bool fade_in;                  // 是否淡入
    bool fade_out;                 // 是否淡出
} led_effect_config_t;

// LED信息结构
typedef struct {
    uint32_t current_color;
    led_state_t current_state;
    uint8_t current_brightness;
    rt_tick_t last_update_tick;
    bool active;
} led_info_t;

/**
 * @brief 初始化LED控制器
 * @return 0成功，负数失败
 */
int led_controller_init(void);

/**
 * @brief 反初始化LED控制器
 * @return 0成功，负数失败
 */
int led_controller_deinit(void);

/**
 * @brief 点亮指定LED
 * @param led_index LED索引 (0-2)
 * @param color RGB颜色值
 * @param duration_ms 持续时间(ms)，0为永久点亮
 * @return 0成功，负数失败
 */
int led_controller_light_led(int led_index, uint32_t color, uint32_t duration_ms);

/**
 * @brief 关闭指定LED
 * @param led_index LED索引 (0-2)
 * @return 0成功，负数失败
 */
int led_controller_turn_off_led(int led_index);

/**
 * @brief 关闭所有LED
 * @return 0成功，负数失败
 */
int led_controller_turn_off_all(void);

/**
 * @brief 设置LED亮度
 * @param led_index LED索引 (0-2)
 * @param brightness 亮度值 (0-255)
 * @return 0成功，负数失败
 */
int led_controller_set_brightness(int led_index, uint8_t brightness);

/**
 * @brief 设置所有LED亮度
 * @param brightness 亮度值 (0-255)
 * @return 0成功，负数失败
 */
int led_controller_set_all_brightness(uint8_t brightness);

/**
 * @brief 设置LED效果
 * @param led_index LED索引 (0-2)
 * @param config 效果配置
 * @return 0成功，负数失败
 */
int led_controller_set_effect(int led_index, const led_effect_config_t *config);

/**
 * @brief 应用呼吸灯效果
 * @param led_index LED索引 (0-2)
 * @param color 颜色值
 * @param period_ms 呼吸周期(ms)
 * @return 0成功，负数失败
 */
int led_controller_breathing_effect(int led_index, uint32_t color, uint32_t period_ms);

/**
 * @brief 应用闪烁效果
 * @param led_index LED索引 (0-2)
 * @param color 颜色值
 * @param on_time_ms 点亮时间(ms)
 * @param off_time_ms 熄灭时间(ms)
 * @param repeat_count 重复次数，0为无限
 * @return 0成功，负数失败
 */
int led_controller_flash_effect(int led_index, uint32_t color, 
                               uint32_t on_time_ms, uint32_t off_time_ms, 
                               uint32_t repeat_count);

/**
 * @brief 应用彩虹效果
 * @param speed_ms 颜色变化速度(ms)
 * @return 0成功，负数失败
 */
int led_controller_rainbow_effect(uint32_t speed_ms);

/**
 * @brief 获取LED状态信息
 * @param led_index LED索引 (0-2)
 * @param info LED信息结构指针
 * @return 0成功，负数失败
 */
int led_controller_get_led_info(int led_index, led_info_t *info);

/**
 * @brief 获取LED数量
 * @return LED总数量
 */
int led_controller_get_led_count(void);

/**
 * @brief 检查LED控制器是否已初始化
 * @return true已初始化，false未初始化
 */
bool led_controller_is_initialized(void);

/**
 * @brief 暂停LED效果
 * @param led_index LED索引 (0-2)，-1为所有LED
 * @return 0成功，负数失败
 */
int led_controller_pause_effect(int led_index);

/**
 * @brief 恢复LED效果
 * @param led_index LED索引 (0-2)，-1为所有LED
 * @return 0成功，负数失败
 */
int led_controller_resume_effect(int led_index);

/**
 * @brief 立即更新LED显示
 * @return 0成功，负数失败
 */
int led_controller_update_display(void);

#ifdef __cplusplus
}
#endif
#endif // LED_CONTROLLER_H