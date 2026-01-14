/**
 * @file screen_ui_l2_stopwatch.c
 * @brief 秒表UI模块实现 - 计时器功能
 * 
 * 秒表功能:
 * - L1入口: 计时器图标入口
 * - L2界面: 计时显示/状态/控制
 * 
 * 页面布局:
 * - 左屏: 秒表标题 + 状态 + 开始/暂停提示
 * - 中屏: 计时显示(MM:SS.d) + 重置提示
 * - 右屏: 计时器图标
 */

#include "screen_ui_l2_stopwatch.h"
#include "../../../screen/screen_init/screen_ui_common.h"
#include "../../../screen/screen_init/screen_ui_resources.h"
#include "../../../screen/screen_ui_manager.h"
#include "../../../screen/screen_context.h"
#include <rtthread.h>
#include <stdio.h>

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * L1 入口面板构建
 ******************************************************************************/

/**
 * @brief 构建计时器入口面板
 */
void l2_stopwatch_build_entrance_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 计时器图片入口 - 上半部分居中 */
    g_ui_mgr.handles.group4_stopwatch.stopwatch_icon = create_entrance_icon(parent, get_calculagraph_image());
    if (g_ui_mgr.handles.group4_stopwatch.stopwatch_icon) {
        lv_obj_align(g_ui_mgr.handles.group4_stopwatch.stopwatch_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_stopwatch.stopwatch_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_stopwatch.stopwatch_hint, "计时器");
    lv_obj_add_style(g_ui_mgr.handles.group4_stopwatch.stopwatch_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_stopwatch.stopwatch_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_stopwatch.stopwatch_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/*******************************************************************************
 * L2 界面构建
 ******************************************************************************/

/**
 * @brief 构建L2秒表界面
 */
void l2_stopwatch_build_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 初始化秒表数据 */
    screen_context_init_stopwatch_data();
    
    /* 创建三面板 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* ========== 左屏: 状态和提示 ========== */
    lv_obj_t *title = lv_label_create(left);
    lv_label_set_text(title, "秒表");
    lv_obj_add_style(title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    g_ui_mgr.handles.l2_stopwatch_timer.state_label = lv_label_create(left);
    lv_label_set_text(g_ui_mgr.handles.l2_stopwatch_timer.state_label, "待机");
    lv_obj_add_style(g_ui_mgr.handles.l2_stopwatch_timer.state_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_stopwatch_timer.state_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.l2_stopwatch_timer.state_label, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t *start_hint = lv_label_create(left);
    lv_label_set_text(start_hint, "开始/暂停");
    lv_obj_add_style(start_hint, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(start_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(start_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    /* ========== 中屏: 计时显示 ========== */
    g_ui_mgr.handles.l2_stopwatch_timer.time_label = lv_label_create(middle);
    lv_label_set_text(g_ui_mgr.handles.l2_stopwatch_timer.time_label, "00:00.0");
    lv_obj_add_style(g_ui_mgr.handles.l2_stopwatch_timer.time_label, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_stopwatch_timer.time_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.l2_stopwatch_timer.time_label, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t *reset_hint = lv_label_create(middle);
    lv_label_set_text(reset_hint, "重置");
    lv_obj_add_style(reset_hint, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(reset_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(reset_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    /* ========== 右屏: 计时器图标 ========== */
    lv_obj_t *icon = create_entrance_icon(right, get_calculagraph_image());
    if (icon) {
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);
    }
}

/*******************************************************************************
 * 更新函数
 ******************************************************************************/

/**
 * @brief 更新秒表显示
 */
int l2_stopwatch_update_display(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    
    if (!g_ui_mgr.handles.l2_stopwatch_timer.time_label || 
        !lv_obj_is_valid(g_ui_mgr.handles.l2_stopwatch_timer.time_label)) {
        return 0;
    }
    
    /* 获取秒表数据 */
    stopwatch_data_t stopwatch_data;
    if (screen_context_get_stopwatch_data(&stopwatch_data) != 0) {
        return -RT_ERROR;
    }
    
    /* 计算时间显示 */
    uint32_t total_deciseconds = stopwatch_data.elapsed_deciseconds;
    uint32_t minutes = total_deciseconds / 600;
    uint32_t seconds = (total_deciseconds % 600) / 10;
    uint32_t decisecond = total_deciseconds % 10;
    
    /* 更新计时显示 */
    char time_str[16];
    rt_snprintf(time_str, sizeof(time_str), "%02u:%02u.%u", minutes, seconds, decisecond);
    lv_label_set_text(g_ui_mgr.handles.l2_stopwatch_timer.time_label, time_str);
    
    /* 更新状态标签 */
    if (g_ui_mgr.handles.l2_stopwatch_timer.state_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_stopwatch_timer.state_label)) {
        const char *state_text;
        if (stopwatch_data.is_running) {
            state_text = "运行中";
        } else if (stopwatch_data.elapsed_deciseconds > 0) {
            state_text = "已暂停";
        } else {
            state_text = "待机";
        }
        lv_label_set_text(g_ui_mgr.handles.l2_stopwatch_timer.state_label, state_text);
    }
    
    return 0;
}
