#ifndef SCREEN_UI_MANAGER_H
#define SCREEN_UI_MANAGER_H

#include "lvgl.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *screen_group1;
    lv_obj_t *screen_group2;
    lv_obj_t *screen_group3;
    lv_obj_t *screen_group4;
    lv_obj_t *screen_group5;
    
    lv_obj_t *screen_l2_time_detail;
    lv_obj_t *screen_l2_weather_forecast;
    lv_obj_t *screen_l2_muyu;
    lv_obj_t *screen_l2_tomato;
    lv_obj_t *screen_l2_stopwatch;
    lv_obj_t *screen_l2_media;
    lv_obj_t *screen_l2_web;
    lv_obj_t *screen_l2_shortcut;
    
    lv_obj_t *g1_left_panel;
    lv_obj_t *g1_middle_panel;
    lv_obj_t *g1_right_panel;
    
    struct {
        lv_obj_t *time_label;
        lv_obj_t *date_label;
        lv_obj_t *weekday_label;
        lv_obj_t *year_label;
    } group1_time;
    
    struct {
        lv_obj_t *city_label;
        lv_obj_t *temperature_label;
        lv_obj_t *weather_label;
        lv_obj_t *humidity_label;
        lv_obj_t *pressure_label;
        lv_obj_t *sensor_label;
        lv_obj_t *weather_icon;
    } group1_weather;
    
    struct {
        lv_obj_t *name_label;
        lv_obj_t *price_label;
        lv_obj_t *change_label;
        lv_obj_t *update_time_label;
    } group1_stock;
    
    lv_obj_t *g2_left_panel;
    lv_obj_t *g2_middle_panel;
    lv_obj_t *g2_right_panel;
    
    struct {
        lv_obj_t *cpu_title;
        lv_obj_t *cpu_usage;
        lv_obj_t *cpu_temp;
        lv_obj_t *cpu_chart;
        lv_obj_t *gpu_title;
        lv_obj_t *gpu_usage;
        lv_obj_t *gpu_temp;
        lv_obj_t *gpu_chart;
    } group2_cpu_gpu;
    
    struct {
        lv_obj_t *ram_title;
        lv_obj_t *ram_usage;
        lv_obj_t *ram_chart;
    } group2_memory;
    
    struct {
        lv_obj_t *network_title;
        lv_obj_t *net_upload;
        lv_obj_t *net_download;
        lv_obj_t *net_status;
    } group2_network;
    
    lv_obj_t *g3_left_panel;
    lv_obj_t *g3_middle_panel;
    lv_obj_t *g3_right_panel;
    
    lv_obj_t *g4_left_panel;
    lv_obj_t *g4_middle_panel;
    lv_obj_t *g4_right_panel;
    
    lv_obj_t *g5_left_panel;
    lv_obj_t *g5_middle_panel;
    lv_obj_t *g5_right_panel;
    
    struct {
        lv_obj_t *muyu_title;
        lv_obj_t *muyu_icon;
        lv_obj_t *muyu_hint;
    } group4_muyu;
    
    struct {
        lv_obj_t *tomato_title;
        lv_obj_t *tomato_icon;
        lv_obj_t *tomato_hint;
    } group4_tomato;
    
    struct {
        lv_obj_t *stopwatch_title;
        lv_obj_t *stopwatch_icon;
        lv_obj_t *stopwatch_hint;
    } group4_stopwatch;
    
    struct {
        lv_obj_t *custom1_icon;
        lv_obj_t *custom1_hint;
    } group5_custom1;
    
    struct {
        lv_obj_t *custom2_icon;
        lv_obj_t *custom2_hint;
    } group5_custom2;
    
    struct {
        lv_obj_t *custom3_icon;
        lv_obj_t *custom3_hint;
    } group5_custom3;
    
    struct {
        lv_obj_t *hour_tens;
        lv_obj_t *hour_units;
        lv_obj_t *min_tens;
        lv_obj_t *min_units;
        lv_obj_t *sec_tens;
        lv_obj_t *sec_units;
    } l2_digital_clock;
    
    struct {
        lv_obj_t *muyu_image;
        lv_obj_t *counter_label;
        lv_obj_t *total_label;
        lv_obj_t *merit_label;
        lv_obj_t *reset_hint;
    } l2_muyu_main;
    
    struct {
        lv_obj_t *day0_title;
        lv_obj_t *day0_weather;
        lv_obj_t *day0_temp_max;
        lv_obj_t *day0_temp_min;
        lv_obj_t *day0_wind_dir;
        lv_obj_t *day0_wind_scale;
        
        lv_obj_t *day1_title;
        lv_obj_t *day1_weather;
        lv_obj_t *day1_temp_max;
        lv_obj_t *day1_temp_min;
        lv_obj_t *day1_wind_dir;
        lv_obj_t *day1_wind_scale;
        
        lv_obj_t *day2_title;
        lv_obj_t *day2_weather;
        lv_obj_t *day2_temp_max;
        lv_obj_t *day2_temp_min;
        lv_obj_t *day2_wind_dir;
        lv_obj_t *day2_wind_scale;
        
        lv_obj_t *update_time;
    } l2_weather_forecast;
    
    struct {
        lv_obj_t *mode_label;
        lv_obj_t *timer_label;
        lv_obj_t *progress_bar;
        lv_obj_t *state_label;
        lv_obj_t *stats_label;
        lv_obj_t *round_label;
    } l2_tomato_timer;
    
    struct {
        lv_obj_t *time_label;
        lv_obj_t *state_label;
        lv_obj_t *hint_label;
    } l2_stopwatch_timer;
    
    lv_font_t *font_xsmall;
    lv_font_t *font_small;
    lv_font_t *font_medium;
    lv_font_t *font_large;
    lv_font_t *font_xlarge;
    lv_font_t *font_xxlarge;
    
    lv_style_t style_xsmall;
    lv_style_t style_small;
    lv_style_t style_medium;
    lv_style_t style_large;
    lv_style_t style_xlarge;
    lv_style_t style_xxlarge;
    
} screen_ui_handles_t;

typedef struct {
    screen_ui_handles_t handles;
    screen_group_t current_group;
    screen_level_t current_level;
    screen_l2_group_t current_l2_group;
    bool initialized;
    float scale_factor;
    
    bool group1_built;
    bool group2_built;
    bool group3_built;
    bool group4_built;
    bool group5_built;
    
    bool l2_time_built;
    bool l2_weather_built;
    bool l2_muyu_built;
    bool l2_tomato_built;
    bool l2_stopwatch_built;
    bool l2_media_built;
    bool l2_web_built;
    bool l2_shortcut_built;

    muyu_data_t muyu_data;
} screen_ui_manager_t;

int screen_ui_manager_init(void);
int screen_ui_manager_deinit(void);

int screen_ui_build_group1(void);
int screen_ui_build_group2(void); 
int screen_ui_build_group3(void);
int screen_ui_build_group5(void);
int screen_ui_build_group4(void);

int screen_ui_build_l2_weather(void);
int screen_ui_build_l2_time(void);
int screen_ui_build_l2_media(void);
int screen_ui_build_l2_web(void);
int screen_ui_build_l2_shortcut(void);
int screen_ui_build_l2_muyu(void);
int screen_ui_build_l2_tomato(void);
int screen_ui_build_l2_stopwatch(void);

int screen_ui_switch_to_group(screen_group_t target_group);
int screen_ui_switch_to_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);
int screen_ui_return_to_l1(screen_group_t l1_group);

int screen_ui_update_time_display(void);
int screen_ui_update_tomato_display(void);
int screen_ui_update_weather_display(const weather_data_t *data);
int screen_ui_update_l2_weather_forecast(const weather_forecast_data_t *forecast);
int screen_ui_update_stock_display(const stock_data_t *data);
int screen_ui_update_system_display(const system_monitor_data_t *data);
int screen_ui_update_sensor_display(void);
int screen_ui_update_muyu_display(void);
int screen_ui_update_stopwatch_display(void);

int screen_ui_cleanup_current_group(void);
int screen_ui_cleanup_all(void);

screen_group_t screen_ui_get_current_group(void);
bool screen_ui_is_initialized(void);

const muyu_data_t* screen_ui_get_muyu_data(void);
int screen_ui_reset_muyu_counter(void);

int screen_ui_start_tomato_background_display(void);
int screen_ui_stop_tomato_background_display(void);

#ifdef __cplusplus
}
#endif

#endif