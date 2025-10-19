#ifndef SCREEN_H
#define SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "screen_types.h"

void create_triple_screen_display(void);

void cleanup_triple_screen_display(void);

int screen_switch_group(screen_group_t group);

screen_group_t screen_get_current_group(void);

void screen_next_group(void);

void screen_process_switch_request(void);

int screen_update_weather(const weather_data_t *data);

int screen_update_stock(const stock_data_t *data);

int screen_update_system_monitor(const system_monitor_data_t *data);

int screen_update_sensor_data(void);

int screen_update_cpu_usage(float usage);
int screen_update_cpu_temp(float temp);
int screen_update_gpu_usage(float usage);
int screen_update_gpu_temp(float temp);
int screen_update_ram_usage(float usage);
int screen_update_net_speeds(float upload_mbps, float download_mbps);

screen_level_t screen_get_current_level(void);

int screen_enter_level2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);

int screen_return_to_level1(void);

int screen_enter_level2_auto(screen_group_t from_l1_group);

int screen_handle_back_button(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* SCREEN_H */