// led_effects_manager.h - 更新版本

#ifndef LED_EFFECTS_MANAGER_H
#define LED_EFFECTS_MANAGER_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "drv_rgbled.h"  // 使用RGB LED驱动

#ifdef __cplusplus
extern "C" {
#endif

/* LED效果类型定义 */
typedef enum {
    LED_EFFECT_NONE = 0,        // 无效果
    LED_EFFECT_STATIC,          // 静态显示
    LED_EFFECT_BREATHING,       // 呼吸灯
    LED_EFFECT_FLOWING,         // 流水灯
    LED_EFFECT_RAINBOW,         // 彩虹效果
    LED_EFFECT_BLINK,           // 闪烁
    LED_EFFECT_WAVE,            // 波浪效果
    LED_EFFECT_CUSTOM,          // 自定义效果
    LED_EFFECT_MAX
} led_effect_type_t;

/* LED效果状态 */
typedef enum {
    LED_EFFECT_STATE_STOPPED = 0,
    LED_EFFECT_STATE_RUNNING,
    LED_EFFECT_STATE_PAUSED,
    LED_EFFECT_STATE_FINISHED
} led_effect_state_t;

/* LED效果参数结构体 */
typedef struct {
    led_effect_type_t type;         // 效果类型
    uint32_t duration_ms;           // 效果持续时间（0表示无限循环）
    uint32_t period_ms;             // 效果周期
    uint8_t brightness;             // 亮度 (0-255)
    uint32_t colors[4];             // 效果用到的颜色数组
    uint8_t color_count;            // 颜色数量
    bool reverse;                   // 是否反向
    uint8_t led_start;              // 起始LED索引
    uint8_t led_count;              // 影响的LED数量
    void *custom_data;              // 自定义数据指针
} led_effect_config_t;

/* LED效果句柄 - 修改为不透明指针 */
typedef void* led_effect_handle_t;

/* 自定义效果回调函数类型 */
typedef int (*led_custom_effect_func_t)(uint32_t tick, const led_effect_config_t *config, uint32_t *led_buffer);

/* LED效果管理器初始化和去初始化 */
int led_effects_manager_init(void);
int led_effects_manager_deinit(void);

/* 全局亮度控制 */
int led_effects_set_global_brightness(uint8_t brightness);
uint8_t led_effects_get_global_brightness(void);

/* 单个LED控制 - 线程安全版本 */
int led_effects_set_led(uint8_t led_index, uint32_t color);
int led_effects_get_led(uint8_t led_index, uint32_t *color);
int led_effects_turn_off_led(uint8_t led_index);
int led_effects_turn_on_led(uint8_t led_index, uint32_t color);

/* 所有LED控制 - 线程安全版本 */
int led_effects_set_all_leds(uint32_t color);
int led_effects_turn_off_all_leds(void);

/* LED效果控制 - 线程安全版本 */
led_effect_handle_t led_effects_start_effect(const led_effect_config_t *config);
int led_effects_stop_effect(led_effect_handle_t handle);
int led_effects_pause_effect(led_effect_handle_t handle);
int led_effects_resume_effect(led_effect_handle_t handle);
int led_effects_stop_all_effects(void);

/* LED效果状态查询 */
led_effect_state_t led_effects_get_effect_state(led_effect_handle_t handle);
int led_effects_get_active_effects_count(void);

/* 预设效果快速启动函数 */
led_effect_handle_t led_effects_breathing(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t duration_ms);
led_effect_handle_t led_effects_flowing(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t duration_ms);
led_effect_handle_t led_effects_rainbow(uint32_t period_ms, uint8_t brightness, uint32_t duration_ms);
led_effect_handle_t led_effects_blink(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t duration_ms);

/* 自定义效果注册 */
int led_effects_register_custom_effect(const char *name, led_custom_effect_func_t func);
led_effect_handle_t led_effects_start_custom_effect(const char *name, const led_effect_config_t *config);

/* 注意：led_effects_update() 函数已移除 - 现在由内部线程自动处理 */

/* 使用drv_rgbled.h中的颜色定义，避免重复定义 */
#define LED_COLOR_OFF           RGB_COLOR_BLACK
#define LED_COLOR_WHITE         RGB_COLOR_WHITE
#define LED_COLOR_RED           RGB_COLOR_RED
#define LED_COLOR_GREEN         RGB_COLOR_GREEN
#define LED_COLOR_BLUE          RGB_COLOR_BLUE
#define LED_COLOR_YELLOW        RGB_COLOR_YELLOW
#define LED_COLOR_CYAN          RGB_COLOR_CYAN
#define LED_COLOR_MAGENTA       RGB_COLOR_MAGENTA

/* 扩展颜色定义（drv_rgbled.h中没有的） */
#define LED_COLOR_ORANGE        0xFF8000
#define LED_COLOR_PURPLE        0x8000FF
#define LED_COLOR_PINK          0xFF80C0

/* 使用drv_rgbled.h中的工具宏 */
#define LED_RGB(r,g,b)          RGB_MAKE_COLOR(r,g,b)
#define LED_GET_RED(color)      RGB_GET_RED(color)
#define LED_GET_GREEN(color)    RGB_GET_GREEN(color)
#define LED_GET_BLUE(color)     RGB_GET_BLUE(color)

/* 亮度调节工具函数 */
uint32_t led_effects_apply_brightness(uint32_t color, uint8_t brightness);
uint32_t led_effects_interpolate_color(uint32_t color1, uint32_t color2, uint8_t ratio);

/* 新增：系统状态查询函数 */
bool led_effects_is_initialized(void);
int led_effects_get_led_count(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_EFFECTS_MANAGER_H */