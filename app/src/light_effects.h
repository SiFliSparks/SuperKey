#ifndef LIGHT_EFFECTS_H
#define LIGHT_EFFECTS_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 灯光效果类型枚举
typedef enum {
    LIGHT_EFFECT_NONE = 0,          // 无效果
    LIGHT_EFFECT_FLOWING_LR,        // 从左到右流水
    LIGHT_EFFECT_FLOWING_RL,        // 从右到左流水
    LIGHT_EFFECT_FLOWING_PINGPONG,  // 乒乓球式流水（左右往返）
    LIGHT_EFFECT_BREATHING,         // 呼吸灯效果
    LIGHT_EFFECT_FLASH,             // 闪烁效果
    LIGHT_EFFECT_RAINBOW,           // 彩虹效果
    LIGHT_EFFECT_WAVE,              // 波浪效果
    LIGHT_EFFECT_CHASE,             // 追逐效果
    LIGHT_EFFECT_STATIC             // 静态显示
} light_effect_type_t;

// 流水灯方向枚举
typedef enum {
    FLOW_DIR_LEFT_TO_RIGHT = 0,     // 从左到右
    FLOW_DIR_RIGHT_TO_LEFT,         // 从右到左
    FLOW_DIR_PINGPONG               // 来回流动
} flow_direction_t;

// 灯光效果配置结构
typedef struct {
    light_effect_type_t type;       // 效果类型
    uint32_t primary_color;         // 主颜色
    uint32_t secondary_color;       // 次颜色（某些效果使用）
    uint32_t speed_ms;              // 速度（ms）
    uint8_t brightness;             // 整体亮度 (0-255)
    uint8_t intensity;              // 效果强度 (0-100)
    flow_direction_t direction;     // 流动方向
    uint32_t cycle_count;           // 循环次数，0为无限循环，1为执行一次（默认）
    bool fade_edges;                // 是否边缘渐变
    uint8_t tail_length;            // 拖尾长度 (1-LED_COUNT)
} light_effect_config_t;

// 效果状态结构
typedef struct {
    light_effect_type_t current_type;
    bool is_running;
    rt_tick_t start_time;
    rt_tick_t last_update;
    uint32_t step_count;
    uint32_t current_cycle;         // 当前循环次数
    uint32_t total_cycles;          // 总循环次数
    void *effect_data;              // 效果特定数据
} light_effect_status_t;

/**
 * @brief 初始化灯光效果模块
 * @return 0成功，负数失败
 */
int light_effects_init(void);

/**
 * @brief 反初始化灯光效果模块
 * @return 0成功，负数失败
 */
int light_effects_deinit(void);

/**
 * @brief 启动指定的灯光效果
 * @param config 效果配置
 * @return 0成功，负数失败
 */
int light_effects_start(const light_effect_config_t *config);

/**
 * @brief 停止当前灯光效果
 * @return 0成功，负数失败
 */
int light_effects_stop(void);

/**
 * @brief 暂停当前灯光效果
 * @return 0成功，负数失败
 */
int light_effects_pause(void);

/**
 * @brief 恢复当前灯光效果
 * @return 0成功，负数失败
 */
int light_effects_resume(void);

/**
 * @brief 更新灯光效果 - 需要在主循环中定期调用
 */
void light_effects_update(void);

/**
 * @brief 获取当前效果状态
 * @param status 状态结构指针
 * @return 0成功，负数失败
 */
int light_effects_get_status(light_effect_status_t *status);

/**
 * @brief 检查效果是否正在运行
 * @return true正在运行，false已停止
 */
bool light_effects_is_running(void);

// === 快捷函数接口（支持循环次数控制） ===

/**
 * @brief 启动流水灯效果
 * @param color 颜色值
 * @param speed_ms 流动速度(ms)
 * @param direction 流动方向
 * @param tail_length 拖尾长度 (1-3)
 * @param cycles 循环次数，0为无限循环，1为执行一次（默认）
 * @return 0成功，负数失败
 */
int light_effects_flowing(uint32_t color, uint32_t speed_ms, flow_direction_t direction, 
                         uint8_t tail_length, uint32_t cycles);

/**
 * @brief 启动呼吸灯效果
 * @param color 颜色值
 * @param period_ms 呼吸周期(ms)
 * @param brightness 最大亮度 (0-255)
 * @param cycles 循环次数，0为无限循环，1为执行一次（默认）
 * @return 0成功，负数失败
 */
int light_effects_breathing(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t cycles);

/**
 * @brief 启动闪烁效果
 * @param color 颜色值
 * @param on_time_ms 点亮时间(ms)
 * @param off_time_ms 熄灭时间(ms)
 * @param cycles 循环次数，0为无限循环，1为执行一次（默认）
 * @return 0成功，负数失败
 */
int light_effects_flash(uint32_t color, uint32_t on_time_ms, uint32_t off_time_ms, uint32_t cycles);

/**
 * @brief 启动彩虹效果
 * @param speed_ms 颜色变化速度(ms)
 * @param cycles 循环次数，0为无限循环，1为执行一次（默认）
 * @return 0成功，负数失败
 */
int light_effects_rainbow(uint32_t speed_ms, uint32_t cycles);

/**
 * @brief 启动波浪效果
 * @param color 基础颜色
 * @param speed_ms 波浪速度(ms)
 * @param intensity 波浪强度 (0-100)
 * @param cycles 循环次数，0为无限循环，1为执行一次（默认）
 * @return 0成功，负数失败
 */
int light_effects_wave(uint32_t color, uint32_t speed_ms, uint8_t intensity, uint32_t cycles);

/**
 * @brief 设置所有LED为同一颜色（静态显示）
 * @param color 颜色值
 * @param brightness 亮度 (0-255)
 * @return 0成功，负数失败
 */
int light_effects_static(uint32_t color, uint8_t brightness);

/**
 * @brief 修改当前效果的颜色
 * @param new_color 新颜色
 * @return 0成功，负数失败
 */
int light_effects_change_color(uint32_t new_color);

/**
 * @brief 修改当前效果的速度
 * @param new_speed_ms 新速度(ms)
 * @return 0成功，负数失败
 */
int light_effects_change_speed(uint32_t new_speed_ms);

/**
 * @brief 修改当前效果的亮度
 * @param new_brightness 新亮度 (0-255)
 * @return 0成功，负数失败
 */
int light_effects_change_brightness(uint8_t new_brightness);

// === 简化版快捷函数（使用默认循环次数1次） ===

/**
 * @brief 简化版流水灯效果（执行一次）
 */
int light_effects_flowing_once(uint32_t color, uint32_t speed_ms, flow_direction_t direction, uint8_t tail_length);

/**
 * @brief 简化版呼吸灯效果（执行一次）
 */
int light_effects_breathing_once(uint32_t color, uint32_t period_ms, uint8_t brightness);

/**
 * @brief 简化版闪烁效果（执行一次）
 */
int light_effects_flash_once(uint32_t color, uint32_t on_time_ms, uint32_t off_time_ms);

/**
 * @brief 简化版彩虹效果（执行一次）
 */
int light_effects_rainbow_once(uint32_t speed_ms);

/**
 * @brief 简化版波浪效果（执行一次）
 */
int light_effects_wave_once(uint32_t color, uint32_t speed_ms, uint8_t intensity);

// === 预设效果宏定义 ===

// 常用颜色预设
#define LIGHT_COLOR_RED         0xFF0000
#define LIGHT_COLOR_GREEN       0x00FF00
#define LIGHT_COLOR_BLUE        0x0000FF
#define LIGHT_COLOR_CYAN        0x00FFFF
#define LIGHT_COLOR_YELLOW      0xFFFF00
#define LIGHT_COLOR_MAGENTA     0xFF00FF
#define LIGHT_COLOR_WHITE       0xFFFFFF
#define LIGHT_COLOR_ORANGE      0xFF8000
#define LIGHT_COLOR_PURPLE      0x8000FF
#define LIGHT_COLOR_PINK        0xFF69B4
#define LIGHT_COLOR_LIME        0x32CD32
#define LIGHT_COLOR_AQUA        0x00FFFF

// 预设效果配置宏
#define LIGHT_EFFECT_CONFIG_DEFAULT() { \
    .type = LIGHT_EFFECT_FLOWING_PINGPONG, \
    .primary_color = LIGHT_COLOR_CYAN, \
    .secondary_color = LIGHT_COLOR_BLUE, \
    .speed_ms = 200, \
    .brightness = 255, \
    .intensity = 80, \
    .direction = FLOW_DIR_PINGPONG, \
    .cycle_count = 1, \
    .fade_edges = true, \
    .tail_length = 2 \
}

// 循环次数常量
#define LIGHT_CYCLES_INFINITE   0       // 无限循环
#define LIGHT_CYCLES_ONCE       1       // 执行一次（默认）

#ifdef __cplusplus
}
#endif

#endif // LIGHT_EFFECTS_H