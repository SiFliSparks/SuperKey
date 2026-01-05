/**
 * @file widget_ui.h
 * @brief 小工具UI显示模块 - 头文件
 */

#ifndef WIDGET_UI_H
#define WIDGET_UI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 构建小工具面板UI
 * @param parent 父容器(Group1的第三个panel)
 * @return 0成功, <0失败
 */
int widget_ui_build_panel(lv_obj_t *parent);

/**
 * @brief 更新小工具面板显示
 * @return 0成功, <0失败
 */
int widget_ui_update_panel(void);

/**
 * @brief 构建L2小工具选择器页面
 * @param screen L2屏幕对象
 * @return 0成功, <0失败
 */
int widget_ui_build_selector(lv_obj_t *screen);

/**
 * @brief 更新选择器高亮显示
 * @return 0成功, <0失败
 */
int widget_ui_update_selector(void);

/**
 * @brief 清理小工具UI资源
 */
void widget_ui_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_UI_H */
