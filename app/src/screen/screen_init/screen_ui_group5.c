/**
 * @file screen_ui_group5.c
 * @brief Group 5 UI模块实现 - 自定义面板 (Custom1/Custom2/Custom3)
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

#include "screen_ui_group5.h"
#include "screen_ui_common.h"
#include "screen_ui_resources.h"
#include "../screen_ui_manager.h"
#include "../../custom/custom_icon_loader.h"

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * Group 5 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - 自定义1面板
 * 优先从SD卡加载自定义图标，失败则回退到默认图标
 */
void group5_build_left_custom1_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    const lv_img_dsc_t *custom_dsc = custom_icon_get_dsc(CUSTOM_ICON_1);
    lv_obj_t *custom1_icon = NULL;
    
    if (custom_dsc != NULL) {
        /* 自定义图片使用独立缩放控制 */
        custom1_icon = create_custom_fullsize_icon(parent, custom_dsc);
    } else {
        /* 使用默认图标 */
        custom1_icon = create_fullsize_icon(parent, get_custom1_image());
    }
    
    if (custom1_icon) {
        g_ui_mgr.handles.group5_custom1.custom1_icon = custom1_icon;
    }
    
    /* 仅在使用默认图标时显示提示文字 */
    if (custom_dsc == NULL) {
        lv_obj_t *custom1_hint = lv_label_create(parent);
        lv_label_set_text(custom1_hint, "自定义1");
        lv_obj_add_style(custom1_hint, &g_ui_mgr.handles.style_small, 0);
        lv_obj_set_style_text_color(custom1_hint, lv_color_make(200, 200, 200), 0);
        lv_obj_align(custom1_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
        g_ui_mgr.handles.group5_custom1.custom1_hint = custom1_hint;
    }
}

/**
 * @brief 构建中屏 - 自定义2面板
 * 优先从SD卡加载自定义图标，失败则回退到默认图标
 */
void group5_build_middle_custom2_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    const lv_img_dsc_t *custom_dsc = custom_icon_get_dsc(CUSTOM_ICON_2);
    lv_obj_t *custom2_icon = NULL;
    
    if (custom_dsc != NULL) {
        /* 自定义图片使用独立缩放控制 */
        custom2_icon = create_custom_fullsize_icon(parent, custom_dsc);
    } else {
        /* 使用默认图标 */
        custom2_icon = create_fullsize_icon(parent, get_custom2_image());
    }
    
    if (custom2_icon) {
        g_ui_mgr.handles.group5_custom2.custom2_icon = custom2_icon;
    }
    
    /* 仅在使用默认图标时显示提示文字 */
    if (custom_dsc == NULL) {
        lv_obj_t *custom2_hint = lv_label_create(parent);
        lv_label_set_text(custom2_hint, "自定义2");
        lv_obj_add_style(custom2_hint, &g_ui_mgr.handles.style_small, 0);
        lv_obj_set_style_text_color(custom2_hint, lv_color_make(200, 200, 200), 0);
        lv_obj_align(custom2_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
        g_ui_mgr.handles.group5_custom2.custom2_hint = custom2_hint;
    }
}

/**
 * @brief 构建右屏 - 自定义3面板
 * 优先从SD卡加载自定义图标，失败则回退到默认图标
 */
void group5_build_right_custom3_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    const lv_img_dsc_t *custom_dsc = custom_icon_get_dsc(CUSTOM_ICON_3);
    lv_obj_t *custom3_icon = NULL;
    
    if (custom_dsc != NULL) {
        /* 自定义图片使用独立缩放控制 */
        custom3_icon = create_custom_fullsize_icon(parent, custom_dsc);
    } else {
        /* 使用默认图标 */
        custom3_icon = create_fullsize_icon(parent, get_custom3_image());
    }
    
    if (custom3_icon) {
        g_ui_mgr.handles.group5_custom3.custom3_icon = custom3_icon;
    }
    
    /* 仅在使用默认图标时显示提示文字 */
    if (custom_dsc == NULL) {
        lv_obj_t *custom3_hint = lv_label_create(parent);
        lv_label_set_text(custom3_hint, "自定义3");
        lv_obj_add_style(custom3_hint, &g_ui_mgr.handles.style_small, 0);
        lv_obj_set_style_text_color(custom3_hint, lv_color_make(200, 200, 200), 0);
        lv_obj_align(custom3_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
        g_ui_mgr.handles.group5_custom3.custom3_hint = custom3_hint;
    }
}
