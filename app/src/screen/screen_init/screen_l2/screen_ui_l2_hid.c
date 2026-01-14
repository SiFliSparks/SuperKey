/**
 * @file screen_ui_l2_hid.c
 * @brief HID快捷键UI模块实现 - 键盘快捷键控制
 * 
 * HID快捷键功能:
 * - L1入口: 快捷键图标入口
 * - L2界面: 复制/粘贴/撤销快捷键控制
 * 
 * 页面布局:
 * - 左屏: 复制(Ctrl+C)
 * - 中屏: 粘贴(Ctrl+V)
 * - 右屏: 撤销(Ctrl+Z)
 */

#include "screen_ui_l2_hid.h"
#include "../../../screen/screen_init/screen_ui_common.h"
#include "../../../screen/screen_init/screen_ui_resources.h"
#include "../../../screen/screen_ui_manager.h"
#include <rtthread.h>

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * L1 入口面板构建
 ******************************************************************************/

/**
 * @brief 构建快捷键入口面板
 */
void l2_hid_build_entrance_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 快捷键图片入口 - 上半部分居中 */
    lv_obj_t *shortcut_icon = create_entrance_icon(parent, get_shortcut_image());
    if (shortcut_icon) {
        lv_obj_align(shortcut_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    /* 功能提示 - 底部小字 */
    lv_obj_t *shortcut_hint = lv_label_create(parent);
    lv_label_set_text(shortcut_hint, "快捷键");
    lv_obj_add_style(shortcut_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(shortcut_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(shortcut_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/*******************************************************************************
 * L2 界面构建
 ******************************************************************************/

/**
 * @brief 构建L2快捷键控制页面
 */
void l2_hid_build_shortcut_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 创建左屏面板 - 复制 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    /* 创建中屏面板 - 粘贴 */
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    /* 创建右屏面板 - 撤销 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* ========== 左屏 - 复制 (Ctrl+C) ========== */
    lv_obj_t *copy_icon = create_entrance_icon(left, get_ctrlc_image());
    if (copy_icon) {
        lv_obj_align(copy_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *copy_hint = lv_label_create(left);
    lv_label_set_text(copy_hint, "复制");
    lv_obj_add_style(copy_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(copy_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(copy_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* ========== 中屏 - 粘贴 (Ctrl+V) ========== */
    lv_obj_t *paste_icon = create_entrance_icon(middle, get_ctrlv_image());
    if (paste_icon) {
        lv_obj_align(paste_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *paste_hint = lv_label_create(middle);
    lv_label_set_text(paste_hint, "粘贴");
    lv_obj_add_style(paste_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(paste_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(paste_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* ========== 右屏 - 撤销 (Ctrl+Z) ========== */
    lv_obj_t *undo_icon = create_entrance_icon(right, get_ctrlz_image());
    if (undo_icon) {
        lv_obj_align(undo_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *undo_hint = lv_label_create(right);
    lv_label_set_text(undo_hint, "撤销");
    lv_obj_add_style(undo_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(undo_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(undo_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}
