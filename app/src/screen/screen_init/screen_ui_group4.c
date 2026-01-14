/**
 * @file screen_ui_group4.c
 * @brief Group 4 UI模块实现 - 实用工具 (番茄钟/秒表)
 * 
 * Group 4 包含三个面板:
 * - 左屏: 赛博木鱼入口
 * - 中屏: 番茄钟入口
 * - 右屏: 计时器/秒表入口
 * 
 * L2页面:
 * - 木鱼界面 
 * - 番茄钟界面
 * - 秒表界面
 */

#include "screen_ui_group4.h"
#include "screen_ui_common.h"
#include "screen_ui_resources.h"
#include "../../screen/screen_init/screen_l2/screen_ui_l2_muyu.h"
#include "../../screen/screen_init/screen_l2/screen_ui_l2_tomato.h"
#include "../../screen/screen_init/screen_l2/screen_ui_l2_stopwatch.h"
#include "../screen_ui_manager.h"
#include "../screen_context.h"
#include <rtthread.h>
#include <stdio.h>

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * Group 4 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 赛博木鱼入口面板
 * @note 包装函数，调用 screen_ui_l2_muyu 模块
 */
void group4_build_left_muyu_panel(lv_obj_t *parent)
{
    l2_muyu_build_entrance_panel(parent);
}

/**
 * @brief 构建中屏 - 番茄钟入口面板
 * @note 包装函数，调用 screen_ui_l2_tomato 模块
 */
void group4_build_middle_tomato_panel(lv_obj_t *parent)
{
    l2_tomato_build_entrance_panel(parent);
}

/**
 * @brief 构建右屏 - 计时器入口面板
 * @note 包装函数，调用 screen_ui_l2_stopwatch 模块
 */
void group4_build_right_stopwatch_panel(lv_obj_t *parent)
{
    l2_stopwatch_build_entrance_panel(parent);
}

/*******************************************************************************
 * Group 4 L2 页面构建
 ******************************************************************************/

/**
 * @brief 构建L2木鱼主界面
 * @note 包装函数，调用 screen_ui_l2_muyu 模块
 */
void group4_build_l2_muyu_main_page(lv_obj_t *screen)
{
    l2_muyu_build_main_page(screen);
}

/**
 * @brief 构建L2番茄钟界面
 * @note 包装函数，调用 screen_ui_l2_tomato 模块
 */
void group4_build_l2_tomato_page(lv_obj_t *screen)
{
    l2_tomato_build_page(screen);
}

/**
 * @brief 构建L2秒表界面
 * @note 包装函数，调用 screen_ui_l2_stopwatch 模块
 */
void group4_build_l2_stopwatch_page(lv_obj_t *screen)
{
    l2_stopwatch_build_page(screen);
}

/*******************************************************************************
 * Group 4 数据更新
 ******************************************************************************/

/**
 * @brief 更新木鱼显示
 * @note 包装函数，调用 screen_ui_l2_muyu 模块
 */
int group4_update_muyu_display(void)
{
    return l2_muyu_update_display();
}

/**
 * @brief 更新番茄钟显示
 * @note 包装函数，调用 screen_ui_l2_tomato 模块
 */
int group4_update_tomato_display(void)
{
    return l2_tomato_update_display();
}

/**
 * @brief 更新秒表显示
 * @note 包装函数，调用 screen_ui_l2_stopwatch 模块
 */
int group4_update_stopwatch_display(void)
{
    return l2_stopwatch_update_display();
}

/**
 * @brief 重置木鱼计数器
 * @note 包装函数，调用 screen_ui_l2_muyu 模块
 */
int group4_reset_muyu_counter(void)
{
    return l2_muyu_reset_counter();
}
