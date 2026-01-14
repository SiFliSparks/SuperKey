/**
 * @file screen_ui_group5.h
 * @brief Group 5 UI模块 - 自定义面板 (Custom1/Custom2/Custom3)
 * 
 * Group 5 包含三个自定义面板:
 * - 左屏: 自定义1面板
 * - 中屏: 自定义2面板
 * - 右屏: 自定义3面板
 * 
 * 特性:
 * - 优先从SD卡加载自定义图标
 * - 失败时回退到默认图标
 * - 自定义图片使用独立缩放控制
 */

#ifndef SCREEN_UI_GROUP5_H
#define SCREEN_UI_GROUP5_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Group 5 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 自定义1面板
 * @param parent 父容器
 * 
 * 优先从SD卡加载自定义图标 (CUSTOM_ICON_1)
 * 失败则回退到默认图标，并显示"自定义1"提示
 */
void group5_build_left_custom1_panel(lv_obj_t *parent);

/**
 * @brief 构建中屏 - 自定义2面板
 * @param parent 父容器
 * 
 * 优先从SD卡加载自定义图标 (CUSTOM_ICON_2)
 * 失败则回退到默认图标，并显示"自定义2"提示
 */
void group5_build_middle_custom2_panel(lv_obj_t *parent);

/**
 * @brief 构建右屏 - 自定义3面板
 * @param parent 父容器
 * 
 * 优先从SD卡加载自定义图标 (CUSTOM_ICON_3)
 * 失败则回退到默认图标，并显示"自定义3"提示
 */
void group5_build_right_custom3_panel(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_GROUP5_H */
