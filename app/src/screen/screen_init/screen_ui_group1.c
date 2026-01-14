/**
 * @file screen_ui_group1.c
 * @brief Group 1 UI模块实现 - 日期时间、天气、小工具
 * 
 * Group 1 包含三个面板:
 * - 左屏: 日期时间显示 (年、月日、星期、时间)
 * - 中屏: 天气信息显示 (城市、天气、温度、湿度、气压、传感器)
 * - 右屏: 小工具面板 (调用widget_ui模块)
 * 
 * L2页面:
 * - 天气预报详情页 (已移至 screen_ui_l2_weather.c)
 */

#include "screen_ui_group1.h"
#include "screen_ui_common.h"
#include "screen_ui_resources.h"
#include "../../screen/screen_init/screen_l2/screen_ui_l2_weather.h"
#include "../screen_ui_manager.h"
#include "../../manager/data_manager.h"
#include "../../widget/widget_ui.h"
#include <rtthread.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * Group 1 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 日期时间面板
 */
void group1_build_left_datetime_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 获取当前时间用于初始显示 */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[32], year_str[16], weekday_str[16];
    
    /* 年份 - 上半部分顶部，左对齐，略小字号 */
    g_ui_mgr.handles.group1_time.year_label = lv_label_create(parent);
    if (tm_info) {
        rt_snprintf(year_str, sizeof(year_str), "%d年", tm_info->tm_year + 1900);
    } else {
        rt_snprintf(year_str, sizeof(year_str), "2025年");
    }
    lv_label_set_text(g_ui_mgr.handles.group1_time.year_label, year_str);
    lv_obj_add_style(g_ui_mgr.handles.group1_time.year_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.year_label, lv_color_make(180, 180, 180), 0);
    lv_obj_align(g_ui_mgr.handles.group1_time.year_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* 日期 - 年份下方，左对齐，最大字号 */
    g_ui_mgr.handles.group1_time.date_label = lv_label_create(parent);
    if (tm_info) {
        rt_snprintf(date_str, sizeof(date_str), "%s%d日", 
                   chinese_months[tm_info->tm_mon], tm_info->tm_mday);
    } else {
        rt_snprintf(date_str, sizeof(date_str), "十二月25日");
    }
    lv_label_set_text(g_ui_mgr.handles.group1_time.date_label, date_str);
    lv_obj_add_style(g_ui_mgr.handles.group1_time.date_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.date_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_time.date_label, 
                    g_ui_mgr.handles.group1_time.year_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 星期 - 日期下方，左对齐，比日期略小 */
    g_ui_mgr.handles.group1_time.weekday_label = lv_label_create(parent);
    if (tm_info) {
        rt_snprintf(weekday_str, sizeof(weekday_str), "%s", chinese_weekdays[tm_info->tm_wday]);
    } else {
        rt_snprintf(weekday_str, sizeof(weekday_str), "周一");
    }
    lv_label_set_text(g_ui_mgr.handles.group1_time.weekday_label, weekday_str);
    lv_obj_add_style(g_ui_mgr.handles.group1_time.weekday_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.weekday_label, lv_color_make(200, 200, 200), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_time.weekday_label, 
                    g_ui_mgr.handles.group1_time.date_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 时间 - 下半部分居中，略大字号 */
    g_ui_mgr.handles.group1_time.time_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_time.time_label, "00:00");
    lv_obj_add_style(g_ui_mgr.handles.group1_time.time_label, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.time_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group1_time.time_label, LV_ALIGN_CENTER, 0, 40);
}

/**
 * @brief 构建中屏 - 天气信息面板
 */
void group1_build_middle_weather_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 城市名 - 顶部左对齐 */
    g_ui_mgr.handles.group1_weather.city_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.city_label, "未知");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.city_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.city_label, lv_color_make(100, 200, 255), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* 天气描述 - 移到左边，城市名下方，左对齐 */
    g_ui_mgr.handles.group1_weather.weather_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.weather_label, "未知");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.weather_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.weather_label, lv_color_make(255, 220, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.weather_label, 
                    g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 温度 - 移到右边，顶部右对齐 */
    g_ui_mgr.handles.group1_weather.temperature_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.temperature_label, "--°C");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.temperature_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.temperature_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.temperature_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* 天气图标 - 在温度下方，靠右边缘 */
    g_ui_mgr.handles.group1_weather.weather_icon = lv_img_create(parent);
    lv_img_set_src(g_ui_mgr.handles.group1_weather.weather_icon, &w999); /* 默认显示未知图标 */
    
    /* 设置图标大小和缩放 */
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 1.0f);  /* 图标占屏幕宽度100% */
    lv_obj_set_size(g_ui_mgr.handles.group1_weather.weather_icon, icon_size, icon_size);
    
    float scale = g_ui_mgr.scale_factor * 0.4f;  /* 适中的缩放比例 */
    
    lv_img_set_zoom(g_ui_mgr.handles.group1_weather.weather_icon, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(g_ui_mgr.handles.group1_weather.weather_icon, true);
    
    /* 移除所有边距和边框并允许溢出显示 */
    lv_obj_set_style_pad_all(g_ui_mgr.handles.group1_weather.weather_icon, 0, 0);
    lv_obj_set_style_border_width(g_ui_mgr.handles.group1_weather.weather_icon, 0, 0);
    lv_obj_add_flag(g_ui_mgr.handles.group1_weather.weather_icon, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    
    /* 位置：在温度下方，靠右边缘 */
    lv_obj_align(g_ui_mgr.handles.group1_weather.weather_icon, 
                LV_ALIGN_TOP_RIGHT, 35, 10);  

    /* 湿度 - 天气描述下方，左对齐 */
    g_ui_mgr.handles.group1_weather.humidity_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.humidity_label, "-%");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.humidity_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.humidity_label, lv_color_make(150, 200, 255), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.humidity_label, 
                    g_ui_mgr.handles.group1_weather.weather_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 气压 - 湿度下方，左对齐 */
    g_ui_mgr.handles.group1_weather.pressure_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.pressure_label, "----hPa");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.pressure_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.pressure_label, lv_color_make(150, 200, 255), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.pressure_label, 
                    g_ui_mgr.handles.group1_weather.humidity_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    
    /* SHT30传感器数据 - 底部居中 */
    g_ui_mgr.handles.group1_weather.sensor_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "当前: --°C --%");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.sensor_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.sensor_label, lv_color_make(100, 255, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.sensor_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/**
 * @brief 构建右屏 - 小工具面板
 */
void group1_build_right_widget_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 使用小工具UI模块构建面板 */
    widget_ui_build_panel(parent);
    
    /* 保存句柄用于后续更新 */
    g_ui_mgr.handles.g1_right_panel = parent;
}

/*******************************************************************************
 * Group 1 L2 页面构建
 * (天气预报实现已移至 screen_ui_l2_weather.c)
 ******************************************************************************/

/**
 * @brief 构建L2天气预报详情页
 * @note 包装函数，调用 screen_ui_l2_weather 模块
 */
void group1_build_l2_weather_forecast_page(lv_obj_t *screen)
{
    l2_weather_build_forecast_page(screen);
}

/*******************************************************************************
 * Group 1 数据更新
 ******************************************************************************/

/**
 * @brief 更新L2天气预报页面数据
 * @note 包装函数，调用 screen_ui_l2_weather 模块
 */
int group1_update_l2_weather_forecast(const weather_forecast_data_t *forecast)
{
    return l2_weather_update_forecast(forecast);
}
