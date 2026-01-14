/**
 * @file screen_ui_l2_weather.h
 * @brief 天气预报UI模块 - L2天气预报详情页
 * 
 * 天气预报功能:
 * - L2界面: 三天天气预报显示
 * 
 * 页面布局:
 * - 左屏: 今天天气 (天气/最高温/最低温/风向风力)
 * - 中屏: 明天天气 (天气/最高温/最低温/风向风力)
 * - 右屏: 后天天气 (天气/最高温/最低温/风向风力)
 */

#ifndef SCREEN_UI_L2_WEATHER_H
#define SCREEN_UI_L2_WEATHER_H

#include "lvgl.h"
#include "../../../screen/screen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * L2 界面构建
 ******************************************************************************/

/**
 * @brief 构建L2天气预报详情页
 * @param screen 屏幕对象
 * 
 * 三屏布局:
 * - 左屏: 今天天气预报
 * - 中屏: 明天天气预报
 * - 右屏: 后天天气预报
 * 
 * 每屏内容:
 * - 日期标题
 * - 天气描述
 * - 最高/最低温度
 * - 风向风力
 */
void l2_weather_build_forecast_page(lv_obj_t *screen);

/*******************************************************************************
 * 更新函数
 ******************************************************************************/

/**
 * @brief 更新L2天气预报页面数据
 * @param forecast 天气预报数据
 * @return 0成功，非0失败
 * 
 * 更新内容:
 * - 三天的天气描述
 * - 三天的最高/最低温度
 * - 三天的风向风力
 */
int l2_weather_update_forecast(const weather_forecast_data_t *forecast);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_L2_WEATHER_H */
