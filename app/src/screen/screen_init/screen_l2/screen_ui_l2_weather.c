/**
 * @file screen_ui_l2_weather.c
 * @brief 天气预报UI模块实现 - L2天气预报详情页
 * 
 * 天气预报功能:
 * - L2界面: 三天天气预报显示
 * 
 * 页面布局:
 * - 左屏: 今天天气 (天气/最高温/最低温/风向风力)
 * - 中屏: 明天天气 (天气/最高温/最低温/风向风力)
 * - 右屏: 后天天气 (天气/最高温/最低温/风向风力)
 */

#include "screen_ui_l2_weather.h"
#include "../../../screen/screen_init/screen_ui_common.h"
#include "../../../screen/screen_init/screen_ui_resources.h"
#include "../../../screen/screen_ui_manager.h"
#include "../../../manager/data_manager.h"
#include <rtthread.h>
#include <stdio.h>

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * 静态函数声明
 ******************************************************************************/

static void build_forecast_day_panel(lv_obj_t *parent, const char *day_title,
                                     lv_obj_t **title, lv_obj_t **weather,
                                     lv_obj_t **temp_max, lv_obj_t **temp_min,
                                     lv_obj_t **wind_dir, lv_obj_t **wind_scale);

/*******************************************************************************
 * 静态函数实现
 ******************************************************************************/

/**
 * @brief 构建天气预报某一天面板
 */
static void build_forecast_day_panel(lv_obj_t *parent, const char *day_title,
                                     lv_obj_t **title, lv_obj_t **weather,
                                     lv_obj_t **temp_max, lv_obj_t **temp_min,
                                     lv_obj_t **wind_dir, lv_obj_t **wind_scale)
{
    if (!parent) return;
    
    /* 标题 */
    *title = lv_label_create(parent);
    lv_label_set_text(*title, day_title);
    lv_obj_add_style(*title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(*title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(*title, LV_ALIGN_TOP_MID, 0, 0);
    
    /* 天气描述 */
    *weather = lv_label_create(parent);
    lv_label_set_text(*weather, "---");
    lv_obj_add_style(*weather, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(*weather, lv_color_white(), 0);
    lv_obj_align(*weather, LV_ALIGN_CENTER, 0, -20);
    
    /* 最高温度 */
    *temp_max = lv_label_create(parent);
    lv_label_set_text(*temp_max, "最高--°C");
    lv_obj_add_style(*temp_max, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(*temp_max, lv_color_make(255, 100, 100), 0);  /* 红色 */
    lv_obj_align(*temp_max, LV_ALIGN_CENTER, 0, 10);
    
    /* 最低温度 */
    *temp_min = lv_label_create(parent);
    lv_label_set_text(*temp_min, "最低--°C");
    lv_obj_add_style(*temp_min, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(*temp_min, lv_color_make(100, 150, 255), 0);  /* 蓝色 */
    lv_obj_align(*temp_min, LV_ALIGN_CENTER, 0, 30);
    
    /* 风向风力 */
    *wind_dir = lv_label_create(parent);
    lv_label_set_text(*wind_dir, "--- ---");  /* 占位文本 */
    lv_obj_add_style(*wind_dir, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(*wind_dir, lv_color_make(180, 180, 180), 0);
    lv_obj_align(*wind_dir, LV_ALIGN_BOTTOM_MID, 0, 0);

    *wind_scale = *wind_dir;  /* 风力与风向共用一个标签 */
}

/*******************************************************************************
 * L2 界面构建
 ******************************************************************************/

/**
 * @brief 构建L2天气预报详情页（三屏显示三天天气）
 */
void l2_weather_build_forecast_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 创建左屏面板 - 今天 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    /* 创建中屏面板 - 明天 */
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    /* 创建右屏面板 - 后天 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* 构建今天面板 */
    build_forecast_day_panel(
        left, "今天",
        &g_ui_mgr.handles.l2_weather_forecast.day0_title,
        &g_ui_mgr.handles.l2_weather_forecast.day0_weather,
        &g_ui_mgr.handles.l2_weather_forecast.day0_temp_max,
        &g_ui_mgr.handles.l2_weather_forecast.day0_temp_min,
        &g_ui_mgr.handles.l2_weather_forecast.day0_wind_dir,
        &g_ui_mgr.handles.l2_weather_forecast.day0_wind_scale
    );
    
    /* 构建明天面板 */
    build_forecast_day_panel(
        middle, "明天",
        &g_ui_mgr.handles.l2_weather_forecast.day1_title,
        &g_ui_mgr.handles.l2_weather_forecast.day1_weather,
        &g_ui_mgr.handles.l2_weather_forecast.day1_temp_max,
        &g_ui_mgr.handles.l2_weather_forecast.day1_temp_min,
        &g_ui_mgr.handles.l2_weather_forecast.day1_wind_dir,
        &g_ui_mgr.handles.l2_weather_forecast.day1_wind_scale
    );
    
    /* 构建后天面板 */
    build_forecast_day_panel(
        right, "后天",
        &g_ui_mgr.handles.l2_weather_forecast.day2_title,
        &g_ui_mgr.handles.l2_weather_forecast.day2_weather,
        &g_ui_mgr.handles.l2_weather_forecast.day2_temp_max,
        &g_ui_mgr.handles.l2_weather_forecast.day2_temp_min,
        &g_ui_mgr.handles.l2_weather_forecast.day2_wind_dir,
        &g_ui_mgr.handles.l2_weather_forecast.day2_wind_scale
    );

    /* 尝试加载已有的天气预报数据 */
    weather_forecast_data_t forecast;
    if (data_manager_get_forecast(&forecast) == 0 && forecast.valid) {
        l2_weather_update_forecast(&forecast);
    }
}

/*******************************************************************************
 * 更新函数
 ******************************************************************************/

/**
 * @brief 更新L2天气预报页面数据
 */
int l2_weather_update_forecast(const weather_forecast_data_t *forecast)
{
    if (!forecast || !forecast->valid || !g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    /* 检查是否在天气预报页面 */
    if (!g_ui_mgr.handles.l2_weather_forecast.day0_weather ||
        !lv_obj_is_valid(g_ui_mgr.handles.l2_weather_forecast.day0_weather)) {
        return 0;
    }
    
    char buf[64];
    
    /* 更新今天 */
    if (forecast->day0.valid) {
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day0_weather, forecast->day0.text);
        
        snprintf(buf, sizeof(buf), "最高%d°C", forecast->day0.temp_max);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day0_temp_max, buf);
        
        snprintf(buf, sizeof(buf), "最低%d°C", forecast->day0.temp_min);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day0_temp_min, buf);
        
        snprintf(buf, sizeof(buf), "%s %s级", forecast->day0.wind_dir, forecast->day0.wind_scale);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day0_wind_dir, buf);
    }
    
    /* 更新明天 */
    if (forecast->day1.valid) {
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day1_weather, forecast->day1.text);
        
        snprintf(buf, sizeof(buf), "最高%d°C", forecast->day1.temp_max);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day1_temp_max, buf);
        
        snprintf(buf, sizeof(buf), "最低%d°C", forecast->day1.temp_min);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day1_temp_min, buf);
        
        snprintf(buf, sizeof(buf), "%s %s级", forecast->day1.wind_dir, forecast->day1.wind_scale);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day1_wind_dir, buf);
    }
    
    /* 更新后天 */
    if (forecast->day2.valid) {
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day2_weather, forecast->day2.text);
        
        snprintf(buf, sizeof(buf), "最高%d°C", forecast->day2.temp_max);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day2_temp_max, buf);
        
        snprintf(buf, sizeof(buf), "最低%d°C", forecast->day2.temp_min);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day2_temp_min, buf);
        
        snprintf(buf, sizeof(buf), "%s %s级", forecast->day2.wind_dir, forecast->day2.wind_scale);
        lv_label_set_text(g_ui_mgr.handles.l2_weather_forecast.day2_wind_dir, buf);
    }
    
    return 0;
}
