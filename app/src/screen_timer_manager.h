#ifndef SCREEN_TIMER_MANAGER_H
#define SCREEN_TIMER_MANAGER_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 定时器类型 */
typedef enum {
    SCREEN_TIMER_CLOCK = 0,      /* 时钟更新 - 1秒 */
    SCREEN_TIMER_WEATHER,        /* 天气更新 - 30秒 */
    SCREEN_TIMER_STOCK,          /* 股票更新 - 10秒 */
    SCREEN_TIMER_SYSTEM,         /* 系统监控 - 2秒 */
    SCREEN_TIMER_SENSOR,         /* 传感器 - 5秒 */
    SCREEN_TIMER_MUYU,           /* 木鱼 - 0.2秒 */
    SCREEN_TIMER_CLEANUP,        /* 清理 - 60秒 */
    SCREEN_TIMER_MAX
} screen_timer_type_t;

/* 定时器配置 */
typedef struct {
    screen_timer_type_t type;
    uint32_t interval_ms;
    bool enabled;
    bool periodic;
    const char* name;
} screen_timer_config_t;

/* 定时器管理器状态 */
typedef struct {
    rt_timer_t timers[SCREEN_TIMER_MAX];
    screen_timer_config_t configs[SCREEN_TIMER_MAX];
    uint32_t trigger_counts[SCREEN_TIMER_MAX];
    rt_tick_t last_trigger_times[SCREEN_TIMER_MAX];
    rt_mutex_t lock;
    bool initialized;
} screen_timer_manager_t;

/* 定时器管理 */
int screen_timer_manager_init(void);
int screen_timer_manager_deinit(void);

/* 定时器控制 - 线程安全 */
int screen_timer_start(screen_timer_type_t type);
int screen_timer_stop(screen_timer_type_t type);
int screen_timer_restart(screen_timer_type_t type);

/* 批量控制 */
int screen_timer_start_group1_timers(void);    /* 启动Group1相关定时器 */
int screen_timer_start_group2_timers(void);    /* 启动Group2相关定时器 */
int screen_timer_stop_all_group_timers(void);  /* 停止所有组定时器 */

/* 配置修改 */
int screen_timer_set_interval(screen_timer_type_t type, uint32_t interval_ms);
int screen_timer_enable(screen_timer_type_t type, bool enabled);

/* 状态查询 */
bool screen_timer_is_running(screen_timer_type_t type);
uint32_t screen_timer_get_trigger_count(screen_timer_type_t type);
rt_tick_t screen_timer_get_last_trigger_time(screen_timer_type_t type);
int screen_timer_get_status_string(char *buffer, size_t buffer_size);
/**
 * @brief 启动L2层级专用定时器
 * 
 * 主要用于L2时间详情页面的数字时钟更新
 * 
 * @return 0成功，负数失败
 */
int screen_timer_start_l2_timers(void);

// 在文件末尾添加函数声明
/**
 * @brief 启动L2木鱼页面定时器
 * @return 0成功，负数失败
 */
int screen_timer_start_l2_muyu_timers(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_TIMER_MANAGER_H */