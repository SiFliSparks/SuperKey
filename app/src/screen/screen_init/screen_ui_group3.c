/**
 * @file screen_ui_group3.c
 * @brief Group 3 UI模块实现 - 媒体控制、网页控制、快捷键
 * 
 * Group 3 包含三个面板:
 * - 左屏: 媒体控制入口
 * - 中屏: 网页控制入口
 * - 右屏: 快捷键入口
 * 
 * L2页面:
 * - 媒体控制页 (上一曲/下一曲/播放暂停)
 * - 网页控制页 (上翻页/下翻页/刷新)
 * - 快捷键页 (已移至 screen_ui_l2_hid.c)
 */

#include "screen_ui_group3.h"
#include "screen_ui_common.h"
#include "screen_ui_resources.h"
#include "../../screen/screen_init/screen_l2/screen_ui_l2_hid.h"
#include "../screen_ui_manager.h"

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * Group 3 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 媒体控制入口面板
 */
void group3_build_left_media_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 媒体控制图片入口 - 上半部分居中 */
    lv_obj_t *media_icon = create_entrance_icon(parent, get_media_image());
    if (media_icon) {
        lv_obj_align(media_icon, LV_ALIGN_CENTER, 0, -10);
    }   
    
    /* 功能提示 - 底部小字 */
    lv_obj_t *media_hint = lv_label_create(parent);
    lv_label_set_text(media_hint, "媒体控制");
    lv_obj_add_style(media_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(media_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(media_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * @brief 构建中屏 - 网页控制入口面板
 */
void group3_build_middle_web_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 网页控制图片入口 - 上半部分居中 */
    lv_obj_t *web_icon = create_entrance_icon(parent, get_web_image());
    if (web_icon) {
        lv_obj_align(web_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    /* 功能提示 - 底部小字 */
    lv_obj_t *web_hint = lv_label_create(parent);
    lv_label_set_text(web_hint, "网页控制");
    lv_obj_add_style(web_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(web_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(web_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * @brief 构建右屏 - 快捷键入口面板
 */
/**
 * @brief 构建右屏 - 快捷键入口面板
 * @note 包装函数，调用 screen_ui_l2_hid 模块
 */
void group3_build_right_shortcut_panel(lv_obj_t *parent)
{
    l2_hid_build_entrance_panel(parent);
}

/*******************************************************************************
 * Group 3 L2 页面构建
 ******************************************************************************/

/**
 * @brief 构建L2媒体控制页面
 */
void group3_build_l2_media_control_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 创建左屏面板 - 上一曲 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    /* 创建中屏面板 - 下一曲 */
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    /* 创建右屏面板 - 播放/暂停 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* 左屏 - 上一曲 */
    lv_obj_t *pre_song_icon = create_entrance_icon(left, get_pre_song_image());
    if (pre_song_icon) {
        lv_obj_align(pre_song_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *pre_song_hint = lv_label_create(left);
    lv_label_set_text(pre_song_hint, "上一曲");
    lv_obj_add_style(pre_song_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(pre_song_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(pre_song_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏 - 下一曲 */
    lv_obj_t *next_song_icon = create_entrance_icon(middle, get_next_song_image());
    if (next_song_icon) {
        lv_obj_align(next_song_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *next_song_hint = lv_label_create(middle);
    lv_label_set_text(next_song_hint, "下一曲");
    lv_obj_add_style(next_song_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(next_song_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(next_song_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏 - 播放/暂停 */
    lv_obj_t *play_pause_icon = create_entrance_icon(right, get_play_image());
    if (play_pause_icon) {
        lv_obj_align(play_pause_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *play_pause_hint = lv_label_create(right);
    lv_label_set_text(play_pause_hint, "播放/暂停");
    lv_obj_add_style(play_pause_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(play_pause_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(play_pause_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * @brief 构建L2网页控制页面
 */
void group3_build_l2_web_control_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 创建左屏面板 - 上翻页 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    /* 创建中屏面板 - 下翻页 */
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    /* 创建右屏面板 - 刷新 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* 左屏 - 上翻页 */
    lv_obj_t *page_up_icon = create_entrance_icon(left, get_up_image());
    if (page_up_icon) {
        lv_obj_align(page_up_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *page_up_hint = lv_label_create(left);
    lv_label_set_text(page_up_hint, "上翻页");
    lv_obj_add_style(page_up_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(page_up_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(page_up_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏 - 下翻页 */
    lv_obj_t *page_down_icon = create_entrance_icon(middle, get_down_image());
    if (page_down_icon) {
        lv_obj_align(page_down_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *page_down_hint = lv_label_create(middle);
    lv_label_set_text(page_down_hint, "下翻页");
    lv_obj_add_style(page_down_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(page_down_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(page_down_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏 - 刷新 */
    lv_obj_t *refresh_icon = create_entrance_icon(right, get_fresh_image());
    if (refresh_icon) {
        lv_obj_align(refresh_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *refresh_hint = lv_label_create(right);
    lv_label_set_text(refresh_hint, "刷新F5");
    lv_obj_add_style(refresh_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(refresh_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(refresh_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * @brief 构建L2快捷键控制页面
 * @note 包装函数，调用 screen_ui_l2_hid 模块
 */
void group3_build_l2_shortcut_control_page(lv_obj_t *screen)
{
    l2_hid_build_shortcut_page(screen);
}
