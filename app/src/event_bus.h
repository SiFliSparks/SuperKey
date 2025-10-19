// event_bus.h - 更新版本

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EVENT_DATA_WEATHER_UPDATED = 0x1000,
    EVENT_DATA_STOCK_UPDATED,
    EVENT_DATA_SYSTEM_UPDATED,
    EVENT_DATA_SENSOR_UPDATED,
    
    EVENT_SCREEN_SWITCH_REQUEST = 0x2000,
    EVENT_SCREEN_REFRESH_REQUEST,
    EVENT_SCREEN_GROUP_CHANGED,
    EVENT_SCREEN_LEVEL_CHANGED,
    
    EVENT_HID_MODE_CHANGED = 0x3000,
    EVENT_HID_KEY_PRESSED,
    EVENT_HID_ERROR_OCCURRED,
    
    EVENT_ENCODER_ROTATED = 0x4000,
    EVENT_ENCODER_MODE_CHANGED,
    
    EVENT_SYSTEM_ERROR = 0x5000,
    EVENT_SYSTEM_WARNING,
    EVENT_SYSTEM_STATUS_CHANGED,
    EVENT_SYSTEM_CLEANUP_REQUEST,
    
    EVENT_COMM_DATA_RECEIVED = 0x6000,
    EVENT_COMM_CONNECTION_STATUS,
    EVENT_COMM_ERROR,
    
    EVENT_LED_FEEDBACK_REQUEST = 0x7000,
    
    EVENT_TYPE_MAX = 0x8000
} event_type_t;

typedef enum {
    EVENT_PRIORITY_LOW = 0,
    EVENT_PRIORITY_NORMAL = 1,
    EVENT_PRIORITY_HIGH = 2,     
    EVENT_PRIORITY_CRITICAL = 3
} event_priority_t;

typedef struct {
    uint32_t int_value;
    float float_value;
    char string_value[64];
    void *ptr_value;
    uint32_t extra_data[4];
} event_data_generic_t;

typedef struct {
    weather_data_t weather;
} event_data_weather_t;

typedef struct {
    stock_data_t stock;
} event_data_stock_t;

typedef struct {
    system_monitor_data_t system;
} event_data_system_t;

typedef struct {
    screen_group_t target_group;
    screen_group_t current_group;
    bool force_switch;
} event_data_screen_switch_t;

typedef struct {
    int32_t delta;
    int32_t total_count;
    void *user_data;
} event_data_encoder_t;

typedef struct {
    int error_code;
    char error_msg[128];
    const char *module_name;
} event_data_error_t;

typedef struct {
    int led_index;
    uint32_t color;
    uint32_t duration_ms;
} event_data_led_t;

typedef struct {
    event_type_t type;
    event_priority_t priority;
    rt_tick_t timestamp;
    uint32_t source_module_id;
    
    union {
        event_data_generic_t generic;
        event_data_weather_t weather;
        event_data_stock_t stock;
        event_data_system_t system;
        event_data_screen_switch_t screen_switch;
        event_data_encoder_t encoder;
        event_data_error_t error;
        event_data_led_t led;
    } data;
} event_t;

typedef int (*event_handler_t)(const event_t *event, void *user_data);

typedef struct {
    event_type_t event_type;
    event_handler_t handler;
    void *user_data;
    event_priority_t min_priority;
    bool enabled;
} event_subscription_t;

#define MODULE_ID_SCREEN        0x0001
#define MODULE_ID_DATA_MANAGER  0x0002
#define MODULE_ID_SERIAL_COMM   0x0003
#define MODULE_ID_HID_DEVICE    0x0004
#define MODULE_ID_ENCODER       0x0005
#define MODULE_ID_LED           0x0006
#define MODULE_ID_SENSOR        0x0007
#define MODULE_ID_SYSTEM        0x0008

// 核心函数
int event_bus_init(void);
int event_bus_deinit(void);
int event_bus_publish(event_type_t type, const void *event_data, size_t data_size, 
                     event_priority_t priority, uint32_t source_module_id);
int event_bus_publish_sync(event_type_t type, const void *event_data, size_t data_size,
                          event_priority_t priority, uint32_t source_module_id);
int event_bus_subscribe(event_type_t event_type, event_handler_t handler, 
                       void *user_data, event_priority_t min_priority);
int event_bus_unsubscribe(event_type_t event_type, event_handler_t handler);
int event_bus_enable_subscription(event_type_t event_type, event_handler_t handler, bool enable);

// 统计和监控函数
int event_bus_get_stats(uint32_t *published_count, uint32_t *processed_count, 
                       uint32_t *dropped_count, uint32_t *queue_size);
int event_bus_cleanup(void);

//  增强功能
int event_bus_enable_health_monitor(bool enable);
uint32_t event_bus_get_error_count(void);
int event_bus_reset_stats(void);

// 便捷函数
int event_bus_publish_data_update(event_type_t data_type, const void *data);
int event_bus_publish_screen_switch(screen_group_t target_group, bool force);
int event_bus_publish_error(int error_code, const char *error_msg, const char *module_name);
int event_bus_publish_led_feedback(int led_index, uint32_t color, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif
#endif