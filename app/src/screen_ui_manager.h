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

/* UI管理器状态 */
typedef struct {
    screen_ui_handles_t handles;
    screen_group_t current_group;
    screen_level_t current_level;
    bool initialized;
    float scale_factor;
    
    /* 赛博木鱼状态数据 - 新增 */
    muyu_data_t muyu_data;
} screen_ui_manager_t;

/**********************
 * 初始化/清理
 **********************/
int screen_ui_manager_init(void);
int screen_ui_manager_deinit(void);

/**********************
 * UI构建 - 仅在GUI线程调用
 **********************/

/**
 * @brief 构建Group 1 UI (时间/天气/股票)
 * @return 0成功，负数失败
 */
int screen_ui_build_group1(void);

/**
 * @brief 构建Group 2 UI (系统监控)
 * @return 0成功，负数失败
 */
int screen_ui_build_group2(void); 

/**
 * @brief 构建Group 3 UI (HID控制)
 * @return 0成功，负数失败
 */
int screen_ui_build_group3(void);

/**
 * @brief 构建Group 4 UI (实用工具：木鱼/番茄钟/全屏图片) - 新增
 * @return 0成功，负数失败
 */
int screen_ui_build_group4(void);

/**
 * @brief 构建L2时间详情页面 (数字图片时钟)
 * 
 * 功能说明：
 * - 左板块：显示小时 (十位+个位数字图片)
 * - 中板块：显示分钟 (十位+个位数字图片)
 * - 右板块：显示秒钟 (十位+个位数字图片)
 * - 使用t0~t9图片资源显示纯数字时钟
 * - 完全移除原有的"未开发"等文字提示
 * 
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_time(void);

/**
 * @brief 构建L2媒体控制页面
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_media(void);

/**
 * @brief 构建L2网页控制页面
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_web(void);

/**
 * @brief 构建L2快捷键控制页面
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_shortcut(void);

/**
 * @brief 构建L2赛博木鱼主界面 - 新增
 * 
 * 功能说明：
 * - 左板块：木鱼图片（可点击，使用muyu图片资源）
 * - 中板块：计数器显示（当前敲击次数）
 * - 右板块：功德/总计等信息显示
 * - 点击木鱼图片时触发计数器+1和LED特效
 * - 复用数字时钟的图片加载方法
 * 
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_muyu(void);

/**
 * @brief 构建L2番茄钟页面 - 新增（预留）
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_tomato(void);

/**
 * @brief 构建L2全屏图片页面 - 新增（预留）
 * @return 0成功，负数失败
 */
int screen_ui_build_l2_gallery(void);

/**********************
 * 安全的UI切换 - 仅在GUI线程调用
 **********************/

/**
 * @brief 切换到指定屏幕组
 * @param target_group 目标屏幕组
 * @return 0成功，负数失败
 */
int screen_ui_switch_to_group(screen_group_t target_group);

/**
 * @brief 切换到L2界面
 * @param l2_group L2层级组
 * @param l2_page L2层级页面
 * @return 0成功，负数失败
 */
int screen_ui_switch_to_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page);

/**
 * @brief 返回L1界面
 * @param l1_group 要返回的L1组
 * @return 0成功，负数失败
 */
int screen_ui_return_to_l1(screen_group_t l1_group);

/**********************
 * UI更新 - 仅在GUI线程调用
 **********************/

/**
 * @brief 更新时间显示
 * 
 * 智能更新机制：
 * - 如果当前在L2时间详情页面，更新数字图片时钟
 * - 如果在Group1页面，更新常规时间显示
 * - 其他情况不执行更新
 * 
 * @return 0成功，负数失败
 */
int screen_ui_update_time_display(void);

/**
 * @brief 更新天气数据显示
 * @param data 天气数据指针
 * @return 0成功，负数失败
 */
int screen_ui_update_weather_display(const weather_data_t *data);

/**
 * @brief 更新股票数据显示
 * @param data 股票数据指针
 * @return 0成功，负数失败
 */
int screen_ui_update_stock_display(const stock_data_t *data);

/**
 * @brief 更新系统监控数据显示
 * @param data 系统监控数据指针
 * @return 0成功，负数失败
 */
int screen_ui_update_system_display(const system_monitor_data_t *data);

/**
 * @brief 更新传感器数据显示
 * @return 0成功，负数失败
 */
int screen_ui_update_sensor_display(void);

/**
 * @brief 更新赛博木鱼计数器显示 - 新增
 * 
 * 功能说明：
 * - 更新当前敲击计数显示
 * - 更新总计数和功德显示
 * - 触发木鱼敲击特效（可选）
 * 
 * @return 0成功，负数失败
 */
int screen_ui_update_muyu_display(void);

/**
 * @brief 木鱼敲击事件处理 - 新增
 * 
 * 功能说明：
 * - 计数器+1
 * - 更新显示
 * - 触发LED特效
 * - 播放音效（如果启用）
 * 
 * @return 0成功，负数失败
 */
int screen_ui_muyu_tap_event(void);

/**********************
 * UI清理 - 仅在GUI线程调用
 **********************/

/**
 * @brief 清理当前组的UI对象
 * @return 0成功，负数失败
 */
int screen_ui_cleanup_current_group(void);

/**
 * @brief 清理所有UI对象
 * @return 0成功，负数失败
 */
int screen_ui_cleanup_all(void);

/**********************
 * 状态查询
 **********************/

/**
 * @brief 获取当前屏幕组
 * @return 当前屏幕组
 */
screen_group_t screen_ui_get_current_group(void);

/**
 * @brief 检查UI管理器是否已初始化
 * @return true已初始化，false未初始化
 */
bool screen_ui_is_initialized(void);

/**
 * @brief 获取木鱼数据 - 新增
 * @return 木鱼数据指针
 */
const muyu_data_t* screen_ui_get_muyu_data(void);

/**
 * @brief 重置木鱼计数器 - 新增
 * @return 0成功，负数失败
 */
int screen_ui_reset_muyu_counter(void);


#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_MANAGER_H */