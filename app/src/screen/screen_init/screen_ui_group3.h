/**
 * @file screen_ui_group3.h
 * @brief Group 3 UI模块 - 媒体控制、网页控制、快捷键
 * 
 * Group 3 包含三个面板:
 * - 左屏: 媒体控制入口
 * - 中屏: 网页控制入口
 * - 右屏: 快捷键入口
 * 
 * L2页面:
 * - 媒体控制页 (上一曲/下一曲/播放暂停)
 * - 网页控制页 (上翻页/下翻页/刷新)
 * - 快捷键页 (复制/粘贴/撤销)
 */

#ifndef SCREEN_UI_GROUP3_H
#define SCREEN_UI_GROUP3_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Group 3 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 媒体控制入口面板
 * @param parent 父容器
 */
void group3_build_left_media_panel(lv_obj_t *parent);

/**
 * @brief 构建中屏 - 网页控制入口面板
 * @param parent 父容器
 */
void group3_build_middle_web_panel(lv_obj_t *parent);

/**
 * @brief 构建右屏 - 快捷键入口面板
 * @param parent 父容器
 */
void group3_build_right_shortcut_panel(lv_obj_t *parent);

/*******************************************************************************
 * Group 3 L2 页面构建
 ******************************************************************************/

/**
 * @brief 构建L2媒体控制页面
 * @param screen 屏幕对象
 * 
 * 三屏布局:
 * - 左屏: 上一曲
 * - 中屏: 下一曲
 * - 右屏: 播放/暂停
 */
void group3_build_l2_media_control_page(lv_obj_t *screen);

/**
 * @brief 构建L2网页控制页面
 * @param screen 屏幕对象
 * 
 * 三屏布局:
 * - 左屏: 上翻页
 * - 中屏: 下翻页
 * - 右屏: 刷新 (F5)
 */
void group3_build_l2_web_control_page(lv_obj_t *screen);

/**
 * @brief 构建L2快捷键控制页面
 * @param screen 屏幕对象
 * 
 * 三屏布局:
 * - 左屏: 复制 (Ctrl+C)
 * - 中屏: 粘贴 (Ctrl+V)
 * - 右屏: 撤销 (Ctrl+Z)
 */
void group3_build_l2_shortcut_control_page(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_GROUP3_H */
