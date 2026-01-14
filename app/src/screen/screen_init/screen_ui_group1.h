/**
 * @file screen_ui_group1.h
 * @brief Group 1 UI模块 - 日期时间、天气、小工具
 * 
 * Group 1 包含三个面板:
 * - 左屏: 日期时间显示
 * - 中屏: 天气信息显示
 * - 右屏: 小工具面板 (调用widget_ui模块)
 * 
 * L2页面:
 * - 天气预报详情页 (三天预报)
 */

#ifndef SCREEN_UI_GROUP1_H
#define SCREEN_UI_GROUP1_H

#include "lvgl.h"
#include "../screen_ui_manager.h"
#include "../../manager/data_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Group 1 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 日期时间面板
 * @param parent 父容器
 */
void group1_build_left_datetime_panel(lv_obj_t *parent);

/**
 * @brief 构建中屏 - 天气信息面板
 * @param parent 父容器
 */
void group1_build_middle_weather_panel(lv_obj_t *parent);

/**
 * @brief 构建右屏 - 小工具面板
 * @param parent 父容器
 */
void group1_build_right_widget_panel(lv_obj_t *parent);

/*******************************************************************************
 * Group 1 L2 页面构建
 ******************************************************************************/

/**
 * @brief 构建L2天气预报详情页
 * @param screen 屏幕对象
 */
void group1_build_l2_weather_forecast_page(lv_obj_t *screen);

/*******************************************************************************
 * Group 1 数据更新
 ******************************************************************************/

/**
 * @brief 更新L2天气预报页面数据
 * @param forecast 天气预报数据指针
 * @return 0成功，其他失败
 */
int group1_update_l2_weather_forecast(const weather_forecast_data_t *forecast);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_GROUP1_H */
