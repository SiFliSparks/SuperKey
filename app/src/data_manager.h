#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <rtthread.h>
#include "screen.h"

#define DATA_TIMEOUT_MS         (60000)
#define CLEANUP_INTERVAL_MS     (30000)

int data_manager_init(void);

int data_manager_deinit(void);

int data_manager_get_weather(weather_data_t *data);
int data_manager_get_stock(stock_data_t *data);  
int data_manager_get_system(system_monitor_data_t *data);

int data_manager_update_weather(const weather_data_t *data);
int data_manager_update_stock(const stock_data_t *data);
int data_manager_update_system(const system_monitor_data_t *data);

rt_tick_t data_manager_get_last_update(const char *type);

int data_manager_cleanup_expired_data(void);
int data_manager_reset_all_data(void);
int data_manager_get_data_status(char *status_buf, size_t buf_size);
bool data_manager_is_data_fresh(const char *type);

#endif