#include "screen.h"
#include "screen_core.h"
#include "screen_ui_manager.h"
#include "screen_timer_manager.h"
#include "screen_context.h"
#include "data_manager.h"
#include "encoder_controller.h"
#include "event_bus.h"
#include <rtthread.h>
#include <time.h>
#include <string.h>  
#include <stdio.h>   
static bool g_screen_system_initialized = false;
static rt_tick_t g_system_start_time = 0;

static int screen_encoder_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type == EVENT_ENCODER_ROTATED) {
        const event_data_encoder_t *encoder_data = &event->data.encoder;
        
        // 检查当前层级
        screen_level_t current_level = screen_core_get_current_level();
        
        if (current_level == SCREEN_LEVEL_2) {
            // 在L2层级时，编码器不进行屏幕组切换
            return 0;
        }
        
        // 只在L1层级时进行屏幕组切换
        if (current_level == SCREEN_LEVEL_1) {
            // 处理原始 delta，支持多步翻页
            int32_t steps = encoder_data->delta;
            
            if (steps != 0) {
                screen_group_t current = screen_core_get_current_group();
                screen_group_t target;
                
                // 计算目标组（支持跨多组跳转）
                if (steps > 0) {
                    // 顺时针：向后翻 steps 页
                    target = (current + steps) % SCREEN_GROUP_MAX;
                } else {
                    // 逆时针：向前翻 |steps| 页
                    // 处理负数取模
                    int32_t new_group = (int32_t)current + steps;  // steps 本身是负数
                    while (new_group < 0) {
                        new_group += SCREEN_GROUP_MAX;
                    }
                    target = new_group % SCREEN_GROUP_MAX;
                }
                
                // 发布切换请求
                screen_core_post_switch_group(target, false);
            }
        }
        
        return 0;
    }
    
    return -1;
}



static int screen_data_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    switch (event->type) {
        case EVENT_DATA_WEATHER_UPDATED:
            screen_core_post_update_weather(&event->data.weather.weather);
            break;
            
        case EVENT_DATA_STOCK_UPDATED:
            screen_core_post_update_stock(&event->data.stock.stock);
            break;
            
        case EVENT_DATA_SYSTEM_UPDATED:
            screen_core_post_update_system(&event->data.system.system);
            break;
            
        case EVENT_DATA_SENSOR_UPDATED:
            /* 传感器数据作为天气更新的一部分 */
            screen_core_post_update_weather(NULL);
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

/**********************
 *   公共API实现
 **********************/

void create_triple_screen_display(void)
{
    g_system_start_time = rt_tick_get();
    
      if (screen_core_init() != 0) {
        return;
    }
    
    if (screen_ui_manager_init() != 0) {
        screen_core_deinit();
        return;
    }
    
    if (screen_timer_manager_init() != 0) {
        screen_ui_manager_deinit();
        screen_core_deinit();
        return;
    }
    
    if (screen_context_init_all() != 0) {
    }
    
    event_bus_subscribe(EVENT_ENCODER_ROTATED, screen_encoder_event_handler, 
                       NULL, EVENT_PRIORITY_HIGH);
    event_bus_subscribe(EVENT_DATA_WEATHER_UPDATED, screen_data_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    event_bus_subscribe(EVENT_DATA_STOCK_UPDATED, screen_data_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    event_bus_subscribe(EVENT_DATA_SYSTEM_UPDATED, screen_data_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    event_bus_subscribe(EVENT_DATA_SENSOR_UPDATED, screen_data_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    
    if (encoder_controller_is_ready()) {
        encoder_controller_stop_polling();
        rt_thread_mdelay(100);
        encoder_controller_reset_count();
        encoder_controller_set_mode(ENCODER_MODE_IDLE);
        encoder_controller_set_sensitivity(1);
        
        if (encoder_controller_start_polling() == 0) {
        }
    }

    if (screen_ui_build_group1() != 0) {
        cleanup_triple_screen_display();
        return;
    }
    screen_timer_start_group1_timers();
    screen_context_activate_for_group(SCREEN_GROUP_1);
    g_screen_system_initialized = true;
    rt_tick_t init_time = rt_tick_get() - g_system_start_time;
}

void cleanup_triple_screen_display(void)
{
    if (!g_screen_system_initialized) {
        return;
    }

    event_bus_unsubscribe(EVENT_ENCODER_ROTATED, screen_encoder_event_handler);
    event_bus_unsubscribe(EVENT_DATA_WEATHER_UPDATED, screen_data_event_handler);
    event_bus_unsubscribe(EVENT_DATA_STOCK_UPDATED, screen_data_event_handler);
    event_bus_unsubscribe(EVENT_DATA_SYSTEM_UPDATED, screen_data_event_handler);
    event_bus_unsubscribe(EVENT_DATA_SENSOR_UPDATED, screen_data_event_handler);
    
    screen_timer_manager_deinit();

    screen_context_deinit_all();

    screen_ui_manager_deinit();

    screen_core_deinit();
    
    g_screen_system_initialized = false;
}

int screen_switch_group(screen_group_t group)
{
    if (!g_screen_system_initialized || group >= SCREEN_GROUP_MAX) {
        return -RT_EINVAL;
    }
    
    return screen_core_post_switch_group(group, false);
}

screen_group_t screen_get_current_group(void)
{
    if (!g_screen_system_initialized) {
        return SCREEN_GROUP_1;
    }
    
    return screen_core_get_current_group();
}

void screen_next_group(void)
{
    if (!g_screen_system_initialized) {
        return;
    }
    
    screen_group_t current = screen_core_get_current_group();
    screen_group_t next = (current + 1) % SCREEN_GROUP_MAX;
    screen_core_post_switch_group(next, false);
}

void screen_process_switch_request(void)
{
    if (!g_screen_system_initialized) {
        return;
    }

    screen_core_process_messages();
}

/**********************
 *   数据更新API - 线程安全版本
 **********************/

/**
 * 更新天气数据
 */
int screen_update_weather(const weather_data_t *data)
{
    if (!data) {
        return -1;
    }
    
    int ret = data_manager_update_weather(data);
    if (ret == 0) {
        // 发布事件通知屏幕更新
        event_data_weather_t weather_event = { .weather = *data };
        event_bus_publish(EVENT_DATA_WEATHER_UPDATED, &weather_event, sizeof(weather_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
    }
    
    return ret;
}

int screen_update_stock(const stock_data_t *data)
{
    if (!data) return -1;
    
    // 通过data_manager更新
    int ret = data_manager_update_stock(data);
    if (ret == 0) {
        // 发布事件通知屏幕更新
        event_data_stock_t stock_event = { .stock = *data };
        event_bus_publish(EVENT_DATA_STOCK_UPDATED, &stock_event, sizeof(stock_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
    }
    
    return ret;
}

int screen_update_system_monitor(const system_monitor_data_t *data)
{
    if (!data) return -1;
    
    // 通过data_manager更新
    int ret = data_manager_update_system(data);
    if (ret == 0) {
        // 发布事件通知屏幕更新
        event_data_system_t system_event = { .system = *data };
        event_bus_publish(EVENT_DATA_SYSTEM_UPDATED, &system_event, sizeof(system_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
    }
    
    return ret;
}

int screen_update_sensor_data(void)
{
    if (!g_screen_system_initialized) {
        return -RT_ERROR;
    }
    
    /* 传感器数据更新作为天气更新的一部分 */
    return screen_core_post_update_weather(NULL);
}

int screen_update_cpu_usage(float usage)
{
    // 构造系统监控数据（只更新CPU部分）
    system_monitor_data_t sys_data = {0};
    
    // 尝试获取现有数据
    if (data_manager_get_system(&sys_data) != 0) {
        // 如果没有现有数据，创建新的
        memset(&sys_data, 0, sizeof(sys_data));
    }
    
    // 更新CPU使用率
    sys_data.cpu_usage = usage;
    sys_data.valid = true;
    
    // 更新时间戳
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                    "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    return screen_update_system_monitor(&sys_data);
}

int screen_update_cpu_temp(float temp)
{
    system_monitor_data_t sys_data = {0};
    if (data_manager_get_system(&sys_data) != 0) {
        memset(&sys_data, 0, sizeof(sys_data));
    }
    
    sys_data.cpu_temp = temp;
    sys_data.valid = true;
    
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                    "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    return screen_update_system_monitor(&sys_data);
}

int screen_update_gpu_usage(float usage)
{
    system_monitor_data_t sys_data = {0};
    if (data_manager_get_system(&sys_data) != 0) {
        memset(&sys_data, 0, sizeof(sys_data));
    }
    
    sys_data.gpu_usage = usage;
    sys_data.valid = true;
    
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                    "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    return screen_update_system_monitor(&sys_data);
}

int screen_update_gpu_temp(float temp)
{
    system_monitor_data_t sys_data = {0};
    if (data_manager_get_system(&sys_data) != 0) {
        memset(&sys_data, 0, sizeof(sys_data));
    }
    
    sys_data.gpu_temp = temp;
    sys_data.valid = true;
    
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                    "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    return screen_update_system_monitor(&sys_data);
}

int screen_update_ram_usage(float usage)
{
    system_monitor_data_t sys_data = {0};
    if (data_manager_get_system(&sys_data) != 0) {
        memset(&sys_data, 0, sizeof(sys_data));
    }
    
    sys_data.ram_usage = usage;
    sys_data.valid = true;
    
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                    "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    return screen_update_system_monitor(&sys_data);
}

int screen_update_net_speeds(float upload_mbps, float download_mbps)
{
    system_monitor_data_t sys_data = {0};
    if (data_manager_get_system(&sys_data) != 0) {
        memset(&sys_data, 0, sizeof(sys_data));
    }
    
    sys_data.net_upload_speed = upload_mbps;
    sys_data.net_download_speed = download_mbps;
    sys_data.valid = true;
    
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                    "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    return screen_update_system_monitor(&sys_data);
}

/**********************
 *   层级管理API
 **********************/

screen_level_t screen_get_current_level(void)
{
    if (!g_screen_system_initialized) {
        return SCREEN_LEVEL_1;
    }
    
    return screen_core_get_current_level();
}

int screen_enter_level2(screen_l2_group_t l2_group, screen_l2_page_t l2_page)
{
    if (!g_screen_system_initialized) {
        return -RT_ERROR;
    }
    
    return screen_core_post_enter_l2(l2_group, l2_page);
}

int screen_return_to_level1(void)
{
    if (!g_screen_system_initialized) {
        return -RT_ERROR;
    }
    
    return screen_core_post_return_l1();
}

int screen_enter_level2_auto(screen_group_t from_l1_group)
{
    /* 自动映射L1组到L2组 */
    screen_l2_group_t l2_group = SCREEN_L2_TIME_GROUP;
    screen_l2_page_t l2_page = SCREEN_L2_TIME_DETAIL;
    
    switch (from_l1_group) {
        case SCREEN_GROUP_1:
            l2_group = SCREEN_L2_TIME_GROUP;
            l2_page = SCREEN_L2_TIME_DETAIL;
            break;
            
        case SCREEN_GROUP_3:
            /* Group 3没有自动映射，需要通过按键选择 */
            return -RT_EINVAL;
            
        default:
            return -RT_EINVAL;
    }
    
    return screen_enter_level2(l2_group, l2_page);
}

