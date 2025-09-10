#include "screen.h"
#include "screen_core.h"
#include "screen_ui_manager.h"
#include "screen_timer_manager.h"
#include "screen_context.h"
#include "data_manager.h"
#include "encoder_controller.h"
#include "event_bus.h"
#include "led_controller.h"
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
        
        rt_kprintf("[Screen] Encoder rotated: delta=%d, level=%d\n", 
                  encoder_data->delta, screen_core_get_current_level());
        
        if (encoder_data->delta > 0) {
            /* 顺时针：下一组/页面 */
            if (screen_core_get_current_level() == SCREEN_LEVEL_1) {
                screen_group_t current = screen_core_get_current_group();
                screen_group_t next = (current + 1) % SCREEN_GROUP_MAX;
                screen_core_post_switch_group(next, false);
            }
        } else if (encoder_data->delta < 0) {
            /* 逆时针：上一组/页面 */  
            if (screen_core_get_current_level() == SCREEN_LEVEL_1) {
                screen_group_t current = screen_core_get_current_group();
                screen_group_t prev = (current == 0) ? (SCREEN_GROUP_MAX - 1) : (current - 1);
                screen_core_post_switch_group(prev, false);
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
    if (g_screen_system_initialized) {
        rt_kprintf("[Screen] System already initialized\n");
        return;
    }
    
    rt_kprintf("[Screen] Initializing thread-safe screen system...\n");
    g_system_start_time = rt_tick_get();
    
      if (screen_core_init() != 0) {
        rt_kprintf("[Screen] ERROR: Failed to initialize screen core\n");
        return;
    }
    
    if (screen_ui_manager_init() != 0) {
        rt_kprintf("[Screen] ERROR: Failed to initialize UI manager\n");
        screen_core_deinit();
        return;
    }
    
    if (screen_timer_manager_init() != 0) {
        rt_kprintf("[Screen] ERROR: Failed to initialize timer manager\n");
        screen_ui_manager_deinit();
        screen_core_deinit();
        return;
    }
    
    if (screen_context_init_all() != 0) {
        rt_kprintf("[Screen] Warning: Failed to initialize screen contexts\n");
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
            rt_kprintf("[Screen] Encoder configured for screen switching\n");
        }
    }

    if (screen_ui_build_group1() != 0) {
        rt_kprintf("[Screen] ERROR: Failed to build initial UI\n");
        cleanup_triple_screen_display();
        return;
    }

    screen_timer_start_group1_timers();

    screen_context_activate_for_group(SCREEN_GROUP_1);
    
    g_screen_system_initialized = true;
    
    rt_tick_t init_time = rt_tick_get() - g_system_start_time;
    rt_kprintf("[Screen] ✅ Thread-safe screen system initialized in %ums\n", 
              (uint32_t)(init_time * 1000 / RT_TICK_PER_SECOND));
    rt_kprintf("[Screen] Key improvements:\n");
    rt_kprintf("[Screen]   - Timer callbacks only post messages (no direct LVGL access)\n");
    rt_kprintf("[Screen]   - All UI operations in GUI thread only\n");
    rt_kprintf("[Screen]   - Fixed data expiry logic (no more 0xFFFFFFFF bug)\n");
    rt_kprintf("[Screen]   - Message-driven architecture for thread safety\n");
    rt_kprintf("[Screen]   - Modular design for easier maintenance\n");
}

void cleanup_triple_screen_display(void)
{
    if (!g_screen_system_initialized) {
        return;
    }
    
    rt_kprintf("[Screen] Cleaning up thread-safe screen system...\n");

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
    rt_kprintf("[Screen] Thread-safe screen system cleaned up\n");
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
        rt_kprintf("[Screen] Invalid weather data\n");
        return -1;
    }
    
    rt_kprintf("[Screen] Weather data received: %s %.1f°C %s\n",
              data->city, data->temperature, data->weather);
    
    int ret = data_manager_update_weather(data);
    if (ret == 0) {
        // 发布事件通知屏幕更新
        event_data_weather_t weather_event = { .weather = *data };
        event_bus_publish(EVENT_DATA_WEATHER_UPDATED, &weather_event, sizeof(weather_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
        rt_kprintf("[Screen] Weather data updated via data manager\n");
    }
    
    return ret;
}

int screen_update_stock(const stock_data_t *data)
{
    if (!data) return -1;
    
    rt_kprintf("[Screen] Stock data received: %s %.2f %+.2f\n",
              data->name, data->current_price, data->change_value);
    
    // 通过data_manager更新
    int ret = data_manager_update_stock(data);
    if (ret == 0) {
        // 发布事件通知屏幕更新
        event_data_stock_t stock_event = { .stock = *data };
        event_bus_publish(EVENT_DATA_STOCK_UPDATED, &stock_event, sizeof(stock_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
        rt_kprintf("[Screen] Stock data updated via data manager\n");
    }
    
    return ret;
}

int screen_update_system_monitor(const system_monitor_data_t *data)
{
    if (!data) return -1;
    
    rt_kprintf("[Screen] System monitor data received: CPU=%.1f%% GPU=%.1f%% RAM=%.1f%%\n",
              data->cpu_usage, data->gpu_usage, data->ram_usage);
    
    // 通过data_manager更新
    int ret = data_manager_update_system(data);
    if (ret == 0) {
        // 发布事件通知屏幕更新
        event_data_system_t system_event = { .system = *data };
        event_bus_publish(EVENT_DATA_SYSTEM_UPDATED, &system_event, sizeof(system_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
        rt_kprintf("[Screen] System data updated via data manager\n");
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

/* 编码器集成 - 简化版本 */
void screen_register_encoder_callback(void)
{
    rt_kprintf("[Screen] Encoder callback registration handled by create_triple_screen_display()\n");
}

void screen_unregister_encoder_callback(void)
{
    rt_kprintf("[Screen] Encoder callback unregistration handled by cleanup_triple_screen_display()\n");
}

/**********************
 *   状态查询和调试
 **********************/

const screen_hierarchy_context_t* screen_get_hierarchy_context(void)
{
    /* 这个函数在新架构中需要重新实现，暂时返回NULL */
    return NULL;
}

