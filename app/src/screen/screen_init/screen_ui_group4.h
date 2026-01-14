/**
 * @file screen_ui_group4.h
 * @brief Group 4 UI模块 - 实用工具 (番茄钟/秒表)
 * 
 * Group 4 包含三个面板:
 * - 左屏: 赛博木鱼入口 (实现已移至 screen_ui_l2_muyu.c)
 * - 中屏: 番茄钟入口
 * - 右屏: 计时器/秒表入口
 * 
 * L2页面:
 * - 木鱼主界面 (已移至 screen_ui_l2_muyu.c)
 * - 番茄钟界面 (倒计时/模式/统计)
 * - 秒表界面 (计时/状态)
 */

#ifndef SCREEN_UI_GROUP4_H
#define SCREEN_UI_GROUP4_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Group 4 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 赛博木鱼入口面板
 * @param parent 父容器
 * @note 包装函数，实际实现在 screen_ui_l2_muyu.c
 */
void group4_build_left_muyu_panel(lv_obj_t *parent);

/**
 * @brief 构建中屏 - 番茄钟入口面板
 * @param parent 父容器
 */
void group4_build_middle_tomato_panel(lv_obj_t *parent);

/**
 * @brief 构建右屏 - 计时器入口面板
 * @param parent 父容器
 */
void group4_build_right_stopwatch_panel(lv_obj_t *parent);

/*******************************************************************************
 * Group 4 L2 页面构建
 ******************************************************************************/

/**
 * @brief 构建L2木鱼主界面
 * @param screen 屏幕对象
 * @note 包装函数，实际实现在 screen_ui_l2_muyu.c
 */
void group4_build_l2_muyu_main_page(lv_obj_t *screen);

/**
 * @brief 构建L2番茄钟界面
 * @param screen 屏幕对象
 * 
 * 三屏布局:
 * - 左屏: 模式/状态/轮次
 * - 中屏: 倒计时/进度条
 * - 右屏: 今日统计
 */
void group4_build_l2_tomato_page(lv_obj_t *screen);

/**
 * @brief 构建L2秒表界面
 * @param screen 屏幕对象
 * 
 * 三屏布局:
 * - 左屏: 状态/操作提示
 * - 中屏: 计时显示
 * - 右屏: 计时器图标
 */
void group4_build_l2_stopwatch_page(lv_obj_t *screen);

/*******************************************************************************
 * Group 4 数据更新
 ******************************************************************************/

/**
 * @brief 更新木鱼显示
 * @return 0成功，其他失败
 * @note 包装函数，实际实现在 screen_ui_l2_muyu.c
 */
int group4_update_muyu_display(void);

/**
 * @brief 更新番茄钟显示
 * @return 0成功，其他失败
 */
int group4_update_tomato_display(void);

/**
 * @brief 更新秒表显示
 * @return 0成功，其他失败
 */
int group4_update_stopwatch_display(void);

/**
 * @brief 重置木鱼计数器
 * @return 0成功，其他失败
 * @note 包装函数，实际实现在 screen_ui_l2_muyu.c
 */
int group4_reset_muyu_counter(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_GROUP4_H */
