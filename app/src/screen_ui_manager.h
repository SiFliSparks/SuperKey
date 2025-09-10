#ifndef SCREEN_UI_MANAGER_H
#define SCREEN_UI_MANAGER_H

#include "lvgl.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UI组件句柄结构 */
typedef struct {
    /* 基础面板 */
    lv_obj_t *root;
    lv_obj_t *left_panel;
    lv_obj_t *middle_panel; 
    lv_obj_t *right_panel;
    
    /* Group 1 组件 */
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
    } group1_weather;
    
    struct {
        lv_obj_t *name_label;
        lv_obj_t *price_label;
        lv_obj_t *change_label;
        lv_obj_t *update_time_label;
    } group1_stock;
    
    /* Group 2 组件 */
    struct {
        lv_obj_t *cpu_title;
        lv_obj_t *cpu_usage;
        lv_obj_t *cpu_temp;
        lv_obj_t *gpu_title;
        lv_obj_t *gpu_usage;
        lv_obj_t *gpu_temp;
    } group2_cpu_gpu;
    
    struct {
        lv_obj_t *ram_title;
        lv_obj_t *ram_usage;
    } group2_memory;
    
    struct {
        lv_obj_t *network_title;
        lv_obj_t *net_upload;
        lv_obj_t *net_download;
        lv_obj_t *net_status;
    } group2_network;
    
    /* 字体和样式 */
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

/* UI管理器状态 */
typedef struct {
    screen_ui_handles_t handles;
    screen_group_t current_group;
    screen_level_t current_level;
    bool initialized;
    float scale_factor;
} screen_ui_manager_t;

/* 初始化/清理 */
int screen_ui_manager_init(void);
int screen_ui_manager_deinit(void);

/* UI构建 - 仅在GUI线程调用 */
int screen_ui_build_group1(void);
int screen_ui_build_group2(void); 
int screen_ui_build_group3(void);
int screen_ui_build_l2_time(void);
int screen_ui_build_l2_media(void);
int screen_ui_build_l2_web(void);
int screen_ui_build_l2_shortcut(void);

/* 安全的UI切换 - 仅在GUI线程调用 */
int screen_ui_switch_to_group(screen_group_t target_group);
int screen_ui_switch_to_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);
int screen_ui_return_to_l1(screen_group_t l1_group);

/* UI更新 - 仅在GUI线程调用 */
int screen_ui_update_time_display(void);
int screen_ui_update_weather_display(const weather_data_t *data);
int screen_ui_update_stock_display(const stock_data_t *data);
int screen_ui_update_system_display(const system_monitor_data_t *data);
int screen_ui_update_sensor_display(void);

/* UI清理 - 仅在GUI线程调用 */
int screen_ui_cleanup_current_group(void);
int screen_ui_cleanup_all(void);

/* 状态查询 */
screen_group_t screen_ui_get_current_group(void);
bool screen_ui_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_MANAGER_H */