/**
 * @file screen.h - 重构版本
 * @brief 线程安全的屏幕系统公共API接口
 * 
 * 重要变更：
 * - 所有API现在都是线程安全的
 * - UI更新通过消息系统异步处理
 * - 修复了timer线程死机问题
 * - 采用模块化架构，便于维护
 */

#ifndef SCREEN_H
#define SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 导入所有数据结构定义 */
#include "screen_types.h"

/**********************
 * 系统生命周期管理
 **********************/

/**
 * @brief 创建线程安全的三联屏显示系统
 * 
 * 关键改进：
 * - 消息驱动架构，解决timer线程死机问题
 * - 模块化设计，便于维护和调试
 * - 线程安全的状态管理
 * - 修复数据过期显示bug
 * 
 * @note 必须在主线程中调用
 */
void create_triple_screen_display(void);

/**
 * @brief 清理三联屏显示系统资源
 * 
 * @note 安全清理，无需复杂的同步逻辑
 */
void cleanup_triple_screen_display(void);

/**********************
 * 屏幕切换API - 线程安全
 **********************/

/**
 * @brief 切换屏幕组（线程安全）
 * 
 * @param group 目标屏幕组
 * @return 0成功，负数失败
 * 
 * @note 可以从任意线程调用，切换通过消息系统异步处理
 */
int screen_switch_group(screen_group_t group);

/**
 * @brief 获取当前屏幕组（线程安全）
 * 
 * @return 当前屏幕组
 */
screen_group_t screen_get_current_group(void);

/**
 * @brief 切换到下一组屏幕（线程安全）
 * 
 * 循环切换：Group1 → Group2 → Group3 → Group1
 */
void screen_next_group(void);

/**
 * @brief 处理屏幕切换请求
 * 
 * ⚠️ 重要：必须在GUI主线程的主循环中调用此函数
 * 
 * 这是唯一可以执行LVGL操作的地方，用于处理所有异步屏幕消息：
 * - 屏幕组切换
 * - 数据更新显示
 * - UI刷新请求
 * 
 * @note 在main()的while循环中调用，在lv_timer_handler()之后
 * 
 * @example
 * while (1) {
 *     uint32_t ms = lv_timer_handler();
 *     screen_process_switch_request();  // 处理屏幕消息
 *     rt_thread_mdelay(ms);
 * }
 */
void screen_process_switch_request(void);

/**********************
 * 数据更新API - 线程安全版本
 **********************/

/**
 * @brief 更新天气数据（线程安全）
 * 
 * @param data 天气数据指针，可以为NULL
 * @return 0成功，负数失败
 * 
 * @note 
 * - 数据存储和UI更新异步进行
 * - 可以从任意线程调用
 * - 如果data为NULL，将从data_manager获取最新数据
 */
int screen_update_weather(const weather_data_t *data);

/**
 * @brief 更新股票数据（线程安全）
 * 
 * @param data 股票数据指针，可以为NULL
 * @return 0成功，负数失败
 */
int screen_update_stock(const stock_data_t *data);

/**
 * @brief 更新系统监控数据（线程安全）
 * 
 * @param data 系统监控数据指针，可以为NULL
 * @return 0成功，负数失败
 * 
 * @note 修复了原代码中导致死机的直接LVGL调用问题
 */
int screen_update_system_monitor(const system_monitor_data_t *data);

/**
 * @brief 更新传感器数据显示（线程安全）
 * 
 * @return 0成功，负数失败
 * 
 * @note 自动从SHT30控制器获取最新数据
 */
int screen_update_sensor_data(void);

/**********************
 * 系统监控单项更新API
 * 
 * ⚠️ 注意：这些函数现在只记录日志，实际更新通过system_monitor_data_t结构进行
 **********************/

int screen_update_cpu_usage(float usage);
int screen_update_cpu_temp(float temp);
int screen_update_gpu_usage(float usage);
int screen_update_gpu_temp(float temp);
int screen_update_ram_usage(float usage);
int screen_update_net_speeds(float upload_mbps, float download_mbps);

/**********************
 * 层级管理API - 线程安全
 **********************/

/**
 * @brief 获取当前界面层级（线程安全）
 */
screen_level_t screen_get_current_level(void);

/**
 * @brief 进入第二层级界面（线程安全）
 * 
 * @param l2_group 第二层级组
 * @param l2_page 第二层级页面
 * @return 0成功，负数失败
 */
int screen_enter_level2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);

/**
 * @brief 返回第一层级界面（线程安全）
 * 
 * @return 0成功，负数失败
 */
int screen_return_to_level1(void);

/**
 * @brief 根据第一层级组自动进入对应的第二层级
 * 
 * @param from_l1_group 来源第一层级组
 * @return 0成功，负数失败
 */
int screen_enter_level2_auto(screen_group_t from_l1_group);

/**
 * @brief 处理返回按键（第四个按键）
 * 
 * @return 0成功，负数失败
 */
int screen_handle_back_button(void);


/**********************
 * 性能和内存使用
 **********************/

/*
 * 新架构的性能特点：
 * 
 * - 消息队列大小：32条消息
 * - 内存使用：比原来略多（约2-3KB额外开销）
 * - 响应性：异步处理，不阻塞数据源
 * - CPU使用：定时器开销减少，LVGL调用集中
 * - 稳定性：消除了timer线程死机问题
 */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* SCREEN_H */