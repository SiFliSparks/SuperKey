#ifndef SCREEN_CORE_H
#define SCREEN_CORE_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 屏幕消息类型 */
typedef enum {
    SCREEN_MSG_UPDATE_TIME = 0,
    SCREEN_MSG_UPDATE_WEATHER,
    SCREEN_MSG_UPDATE_STOCK,
    SCREEN_MSG_UPDATE_SYSTEM,
    SCREEN_MSG_UPDATE_SENSOR,
    SCREEN_MSG_SWITCH_GROUP,
    SCREEN_MSG_ENTER_L2,
    SCREEN_MSG_RETURN_L1,
    SCREEN_MSG_CLEANUP_REQUEST,
    SCREEN_MSG_MAX
} screen_msg_type_t;

/* 屏幕切换消息 */
typedef struct {
    screen_group_t target_group;
    bool force_switch;
} screen_switch_msg_t;

/* L2进入消息 */
typedef struct {
    screen_l2_group_t l2_group;
    screen_l2_page_t l2_page;
} screen_l2_enter_msg_t;

/* 屏幕消息结构 */
typedef struct {
    screen_msg_type_t type;
    rt_tick_t timestamp;
    union {
        screen_switch_msg_t switch_msg;
        screen_l2_enter_msg_t l2_enter_msg;
        weather_data_t weather_data;
        stock_data_t stock_data;
        system_monitor_data_t system_data;
    } data;
} screen_message_t;

/**
 * @brief 获取当前屏幕层级（线程安全）
 * @return 当前屏幕层级
 */
screen_level_t screen_core_get_current_level(void);

/**
 * @brief 获取当前L2组（线程安全）
 * @return 当前L2组，如果不在L2层级则返回默认值
 */
screen_l2_group_t screen_core_get_current_l2_group(void);


/* 屏幕核心管理器 */
typedef struct {
    /* 消息系统 */
    rt_mq_t message_queue;
    rt_mutex_t state_lock;
    
    /* 当前状态 */
    screen_group_t current_group;
    screen_level_t current_level;
    screen_l2_group_t l2_current_group;
    screen_l2_page_t l2_current_page;
    
    /* UI对象管理 */
    bool ui_initialized;
    bool switching_in_progress;
    
    /* 统计信息 */
    uint32_t messages_processed;
    uint32_t switch_count;
    uint32_t last_cleanup_time;
    
} screen_core_t;

/* 核心API */
int screen_core_init(void);
int screen_core_deinit(void);

/* 消息发送接口 - 线程安全 */
int screen_core_post_switch_group(screen_group_t target_group, bool force);
int screen_core_post_enter_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);
int screen_core_post_return_l1(void);
int screen_core_post_update_time(void);
int screen_core_post_update_weather(const weather_data_t *data);
int screen_core_post_update_stock(const stock_data_t *data);
int screen_core_post_update_system(const system_monitor_data_t *data);
int screen_core_post_cleanup_request(void);

/* 消息处理 - 仅在GUI线程调用 */
int screen_core_process_messages(void);

/* 状态查询 - 线程安全 */
screen_group_t screen_core_get_current_group(void);
screen_level_t screen_core_get_current_level(void);
bool screen_core_is_switching(void);


#ifdef __cplusplus
}
#endif

#endif /* SCREEN_CORE_H */