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
        lv_obj_t *weather_icon;
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
    
    /* Group 4 实用工具组件 - 新增 */
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
        lv_obj_t *gallery_title;
        lv_obj_t *gallery_icon;
        lv_obj_t *gallery_hint;
    } group4_gallery;
    
    /* L2数字时钟组件 */
    struct {
        lv_obj_t *hour_tens;    // 小时十位数字图片
        lv_obj_t *hour_units;   // 小时个位数字图片
        lv_obj_t *min_tens;     // 分钟十位数字图片
        lv_obj_t *min_units;    // 分钟个位数字图片
        lv_obj_t *sec_tens;     // 秒钟十位数字图片
        lv_obj_t *sec_units;    // 秒钟个位数字图片
    } l2_digital_clock;
    
    /* L2赛博木鱼组件 - 新增 */
    struct {
        lv_obj_t *muyu_image;       // 木鱼图片（可点击）
        lv_obj_t *counter_label;    // 计数器显示
        lv_obj_t *total_label;      // 总计数显示
        lv_obj_t *merit_label;      // 功德提示
        lv_obj_t *reset_hint;       // 重置提示
    } l2_muyu_main;
    
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

typedef struct {
    screen_ui_handles_t handles;
    screen_group_t current_group;
    screen_level_t current_level;
    bool initialized;
    float scale_factor;

    muyu_data_t muyu_data;
} screen_ui_manager_t;

int screen_ui_manager_init(void);
int screen_ui_manager_deinit(void);

int screen_ui_build_group1(void);

int screen_ui_build_group2(void); 

int screen_ui_build_group3(void);

int screen_ui_build_group4(void);

int screen_ui_build_l2_time(void);

int screen_ui_build_l2_media(void);

int screen_ui_build_l2_web(void);

int screen_ui_build_l2_shortcut(void);

int screen_ui_build_l2_muyu(void);

int screen_ui_build_l2_tomato(void);

int screen_ui_build_l2_gallery(void);

int screen_ui_switch_to_group(screen_group_t target_group);

int screen_ui_switch_to_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);

int screen_ui_return_to_l1(screen_group_t l1_group);

int screen_ui_update_time_display(void);

int screen_ui_update_weather_display(const weather_data_t *data);

int screen_ui_update_stock_display(const stock_data_t *data);

int screen_ui_update_system_display(const system_monitor_data_t *data);

int screen_ui_update_sensor_display(void);

int screen_ui_update_muyu_display(void);

int screen_ui_cleanup_current_group(void);

int screen_ui_cleanup_all(void);

screen_group_t screen_ui_get_current_group(void);

bool screen_ui_is_initialized(void);

const muyu_data_t* screen_ui_get_muyu_data(void);

int screen_ui_reset_muyu_counter(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_MANAGER_H */