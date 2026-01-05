/**
 * @file widget_manager.h
 * @brief 小工具管理模块 - 头文件
 * 
 * 管理小工具的选择、激活和运行状态
 */

#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include "../widget/widget_storage.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 小工具运行时状态
 * ============================================================================ */

/* 随机数工具运行状态 */
typedef struct {
    int32_t current_value;           /* 当前显示的随机数 */
    random_max_option_t max_option;  /* 当前最大值选项 */
    bool in_setting_mode;            /* 是否在设置模式 */
    bool setting_flash_on;           /* 设置模式闪烁状态 */
    uint8_t click_count;             /* 连击计数 */
    uint32_t last_click_time;        /* 最后点击时间 */
} widget_random_state_t;

/* 下班倒计时运行状态 */
typedef struct {
    int32_t remaining_seconds;       /* 剩余秒数 (负数表示已过期) */
    uint8_t target_hour;             /* 目标小时 */
    uint8_t target_minute;           /* 目标分钟 */
    bool is_off_work;                /* 是否已下班 */
    bool in_setting_mode;            /* 是否在设置模式 */
    uint8_t setting_step;            /* 设置步骤: 0=小时, 1=分钟 */
    uint8_t temp_hour;               /* 临时小时值 */
    uint8_t temp_minute;             /* 临时分钟值 */
    uint8_t click_count;             /* 连击计数 */
    uint32_t last_click_time;        /* 最后点击时间 */
} widget_offwork_state_t;

/* 今天吃什么运行状态 */
typedef struct {
    char current_food[32];           /* 当前显示的菜名 */
    bool has_result;                 /* 是否有结果 */
} widget_food_state_t;

/* 小工具选择器状态 */
typedef struct {
    int current_index;               /* 当前选中的小工具索引 */
    bool active;                     /* 选择器是否激活 */
} widget_selector_state_t;

/* ============================================================================
 * API 函数
 * ============================================================================ */

/**
 * @brief 初始化小工具管理器
 * @return 0 成功, <0 失败
 */
int widget_manager_init(void);

/**
 * @brief 反初始化小工具管理器
 * @return 0 成功, <0 失败
 */
int widget_manager_deinit(void);

/**
 * @brief 获取当前激活的小工具类型
 * @return 小工具类型
 */
widget_type_t widget_manager_get_active_type(void);

/**
 * @brief 设置激活的小工具
 * @param type 小工具类型
 * @return 0 成功, <0 失败
 */
int widget_manager_set_active_type(widget_type_t type);

/* ============================================================================
 * 随机数工具接口
 * ============================================================================ */

/**
 * @brief 获取随机数工具状态
 * @param state 输出状态
 * @return 0 成功, <0 失败
 */
int widget_random_get_state(widget_random_state_t *state);

/**
 * @brief 生成新的随机数
 * @return 生成的随机数
 */
int32_t widget_random_generate(void);

/**
 * @brief 处理随机数工具按键
 * @param key_idx 按键索引
 * @return 0 成功, <0 失败
 */
int widget_random_handle_key(int key_idx);

/**
 * @brief 随机数设置模式 - 切换最大值
 * @param delta 增减方向 (+1/-1)
 */
void widget_random_change_max(int delta);

/**
 * @brief 退出随机数设置模式
 */
void widget_random_exit_setting(void);

/* ============================================================================
 * 下班倒计时接口
 * ============================================================================ */

/**
 * @brief 获取下班倒计时状态
 * @param state 输出状态
 * @return 0 成功, <0 失败
 */
int widget_offwork_get_state(widget_offwork_state_t *state);

/**
 * @brief 更新下班倒计时(每秒调用)
 */
void widget_offwork_tick(void);

/**
 * @brief 处理下班倒计时按键
 * @param key_idx 按键索引
 * @return 0 成功, <0 失败
 */
int widget_offwork_handle_key(int key_idx);

/**
 * @brief 设置模式 - 调整时间
 * @param delta 增减方向
 */
void widget_offwork_change_time(int delta);

/**
 * @brief 设置模式 - 下一步
 */
void widget_offwork_next_step(void);

/**
 * @brief 退出设置模式并保存
 */
void widget_offwork_save_setting(void);

/* ============================================================================
 * 今天吃什么接口
 * ============================================================================ */

/**
 * @brief 获取今天吃什么状态
 * @param state 输出状态
 * @return 0 成功, <0 失败
 */
int widget_food_get_state(widget_food_state_t *state);

/**
 * @brief 随机选择一道菜
 * @return 选中的菜名
 */
const char* widget_food_random_pick(void);

/**
 * @brief 处理今天吃什么按键
 * @param key_idx 按键索引
 * @return 0 成功, <0 失败
 */
int widget_food_handle_key(int key_idx);

/* ============================================================================
 * 小工具选择器接口 (L2层级)
 * ============================================================================ */

/**
 * @brief 获取选择器状态
 * @param state 输出状态
 * @return 0 成功, <0 失败
 */
int widget_selector_get_state(widget_selector_state_t *state);

/**
 * @brief 激活选择器
 */
void widget_selector_activate(void);

/**
 * @brief 关闭选择器
 */
void widget_selector_deactivate(void);

/**
 * @brief 选择器 - 移动选择
 * @param delta 移动方向 (+1右/-1左)
 */
void widget_selector_move(int delta);

/**
 * @brief 选择器 - 确认选择并绑定
 * @return 选中的小工具类型
 */
widget_type_t widget_selector_confirm(void);

/**
 * @brief 获取选择器当前高亮的类型
 * @return 当前高亮的类型
 */
widget_type_t widget_selector_get_current_type(void);

/* ============================================================================
 * 定时器回调 (UI刷新)
 * ============================================================================ */

/**
 * @brief 小工具定时器回调 (每500ms)
 */
void widget_timer_tick(void);

/**
 * @brief 设置模式闪烁定时器回调 (每300ms)
 */
void widget_setting_flash_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_MANAGER_H */
