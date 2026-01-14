/**
 * @file screen_ui_l2_tomato.c
 * @brief 番茄钟UI模块实现 - 番茄工作法计时器
 * 
 * 番茄钟功能:
 * - L1入口: 番茄钟图标入口
 * - L2界面: 倒计时/模式/状态/统计显示
 * 
 * 页面布局:
 * - 左屏: 模式(专注/短休息/长休息) + 状态 + 轮次
 * - 中屏: 倒计时显示 + 进度条
 * - 右屏: 今日完成数 + 连续数
 */

#include "screen_ui_l2_tomato.h"
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
 * @brief 构建番茄钟入口面板
 */
void l2_tomato_build_entrance_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 番茄钟图片入口 - 上半部分居中 */
    g_ui_mgr.handles.group4_tomato.tomato_icon = create_entrance_icon(parent, get_tomato_image());
    if (g_ui_mgr.handles.group4_tomato.tomato_icon) {
        lv_obj_align(g_ui_mgr.handles.group4_tomato.tomato_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_tomato.tomato_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_tomato.tomato_hint, "番茄钟");
    lv_obj_add_style(g_ui_mgr.handles.group4_tomato.tomato_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_tomato.tomato_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_tomato.tomato_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/*******************************************************************************
 * L2 界面构建
 ******************************************************************************/

/**
 * @brief 构建L2番茄钟界面
 */
void l2_tomato_build_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 初始化番茄钟数据 */
    screen_context_init_tomato_data();
    
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
    
    /* ========== 左屏: 模式和状态显示 ========== */
    
    /* 模式标题 */
    g_ui_mgr.handles.l2_tomato_timer.mode_label = lv_label_create(left);
    lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.mode_label, "专注");
    lv_obj_add_style(g_ui_mgr.handles.l2_tomato_timer.mode_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_tomato_timer.mode_label, lv_color_make(255, 99, 71), 0);
    lv_obj_align(g_ui_mgr.handles.l2_tomato_timer.mode_label, LV_ALIGN_TOP_MID, 0, SCALE_DPX(10));
    
    /* 状态提示 */
    g_ui_mgr.handles.l2_tomato_timer.state_label = lv_label_create(left);
    lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.state_label, "待机");
    lv_obj_add_style(g_ui_mgr.handles.l2_tomato_timer.state_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_tomato_timer.state_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.l2_tomato_timer.state_label, LV_ALIGN_CENTER, 0, 0);
    
    /* 轮次显示 */
    g_ui_mgr.handles.l2_tomato_timer.round_label = lv_label_create(left);
    lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.round_label, "1/4");
    lv_obj_add_style(g_ui_mgr.handles.l2_tomato_timer.round_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_tomato_timer.round_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.l2_tomato_timer.round_label, LV_ALIGN_BOTTOM_MID, 0, -SCALE_DPX(10));
    
    /* ========== 中屏: 倒计时和进度条 ========== */
    
    /* 倒计时显示 */
    g_ui_mgr.handles.l2_tomato_timer.timer_label = lv_label_create(middle);
    lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.timer_label, "25:00");
    lv_obj_add_style(g_ui_mgr.handles.l2_tomato_timer.timer_label, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_tomato_timer.timer_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.l2_tomato_timer.timer_label, LV_ALIGN_CENTER, 0, -SCALE_DPX(10));
    
    /* 进度条 */
    g_ui_mgr.handles.l2_tomato_timer.progress_bar = lv_bar_create(middle);
    lv_obj_set_size(g_ui_mgr.handles.l2_tomato_timer.progress_bar, SCALE_DPX(100), SCALE_DPX(8));
    lv_bar_set_range(g_ui_mgr.handles.l2_tomato_timer.progress_bar, 0, 100);
    lv_bar_set_value(g_ui_mgr.handles.l2_tomato_timer.progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_ui_mgr.handles.l2_tomato_timer.progress_bar, 
                              lv_color_make(255, 99, 71), LV_PART_INDICATOR);
    lv_obj_align(g_ui_mgr.handles.l2_tomato_timer.progress_bar, LV_ALIGN_BOTTOM_MID, 0, -SCALE_DPX(15));
    
    /* ========== 右屏: 统计信息 ========== */
    
    /* 统计标题 */
    lv_obj_t *stats_title = lv_label_create(right);
    lv_label_set_text(stats_title, "今日");
    lv_obj_add_style(stats_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(stats_title, lv_color_white(), 0);
    lv_obj_align(stats_title, LV_ALIGN_TOP_MID, 0, SCALE_DPX(10));
    
    /* 统计数据 */
    g_ui_mgr.handles.l2_tomato_timer.stats_label = lv_label_create(right);
    lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.stats_label, "0");
    lv_obj_add_style(g_ui_mgr.handles.l2_tomato_timer.stats_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_tomato_timer.stats_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.l2_tomato_timer.stats_label, LV_ALIGN_CENTER, 0, 0);
    
    /* 连续数显示 */
    lv_obj_t *continuous_label = lv_label_create(right);
    lv_label_set_text(continuous_label, "连续:0");
    lv_obj_add_style(continuous_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(continuous_label, lv_color_white(), 0);
    lv_obj_align(continuous_label, LV_ALIGN_BOTTOM_MID, 0, -SCALE_DPX(10));
}

/*******************************************************************************
 * 更新函数
 ******************************************************************************/

/**
 * @brief 更新番茄钟显示
 */
int l2_tomato_update_display(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    
    if (!g_ui_mgr.handles.l2_tomato_timer.timer_label || 
        !lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.timer_label)) {
        return 0;
    }
    
    tomato_data_t tomato_data;
    if (screen_context_get_tomato_data(&tomato_data) != 0) {
        return -RT_ERROR;
    }
    
    /* 更新模式标签 */
    if (g_ui_mgr.handles.l2_tomato_timer.mode_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.mode_label)) {
        const char *mode_text;
        lv_color_t mode_color;
        
        switch (tomato_data.current_mode) {
            case TOMATO_MODE_FOCUS:
                mode_text = "专注";
                mode_color = lv_color_make(255, 99, 71);  /* 番茄红 */
                break;
            case TOMATO_MODE_SHORT_BREAK:
                mode_text = "短休息";
                mode_color = lv_color_make(0, 255, 0);    /* 绿色 */
                break;
            case TOMATO_MODE_LONG_BREAK:
                mode_text = "长休息";
                mode_color = lv_color_make(0, 191, 255);  /* 深天蓝 */
                break;
            default:
                mode_text = "未知";
                mode_color = lv_color_white();
                break;
        }
        
        lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.mode_label, mode_text);
        lv_obj_set_style_text_color(g_ui_mgr.handles.l2_tomato_timer.mode_label, mode_color, 0);
    }
    
    /* 更新倒计时 */
    if (g_ui_mgr.handles.l2_tomato_timer.timer_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.timer_label)) {
        uint16_t minutes = tomato_data.remaining_seconds / 60;
        uint16_t seconds = tomato_data.remaining_seconds % 60;
        
        char timer_str[16];
        rt_snprintf(timer_str, sizeof(timer_str), "%02d:%02d", minutes, seconds);
        lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.timer_label, timer_str);
    }
    
    /* 更新进度条 */
    if (g_ui_mgr.handles.l2_tomato_timer.progress_bar && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.progress_bar)) {
        lv_bar_set_value(g_ui_mgr.handles.l2_tomato_timer.progress_bar, 
                        tomato_data.progress_percent, LV_ANIM_OFF);
    }
    
    /* 更新状态标签 */
    if (g_ui_mgr.handles.l2_tomato_timer.state_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.state_label)) {
        const char *state_text;
        
        switch (tomato_data.current_state) {
            case TOMATO_STATE_IDLE:
                state_text = "待机";
                break;
            case TOMATO_STATE_RUNNING:
                state_text = "进行中";
                break;
            case TOMATO_STATE_PAUSED:
                state_text = "已暂停";
                break;
            case TOMATO_STATE_COMPLETED:
                state_text = "已完成!";
                break;
            default:
                state_text = "未知";
                break;
        }
        
        lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.state_label, state_text);
    }
    
    /* 更新统计标签 */
    if (g_ui_mgr.handles.l2_tomato_timer.stats_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.stats_label)) {
        char stats_str[16];
        rt_snprintf(stats_str, sizeof(stats_str), "%d", tomato_data.today_completed);
        lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.stats_label, stats_str);
    }
    
    /* 更新轮次标签 */
    if (g_ui_mgr.handles.l2_tomato_timer.round_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_tomato_timer.round_label)) {
        char round_str[16];
        if (tomato_data.current_mode == TOMATO_MODE_FOCUS) {
            rt_snprintf(round_str, sizeof(round_str), "%d/%d", 
                       tomato_data.current_round + 1, 
                       tomato_data.long_break_interval);
        } else {
            rt_snprintf(round_str, sizeof(round_str), "休息中");
        }
        lv_label_set_text(g_ui_mgr.handles.l2_tomato_timer.round_label, round_str);
    }
    
    return 0;
}
