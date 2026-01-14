/**
 * @file screen_ui_l2_muyu.c
 * @brief 木鱼UI模块实现 - 赛博木鱼功能
 * 
 * 木鱼功能:
 * - L1入口: 赛博木鱼图标入口
 * - L2主界面: 木鱼敲击/功德计数/等级显示
 * 
 * 页面布局:
 * - 左屏: 木鱼图片 + "按键1敲击"提示
 * - 中屏: 功德标题 + 本次计数
 * - 右屏: 总计 + 等级 + "按键2重置"提示
 */

#include "screen_ui_l2_muyu.h"
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
 * 静态函数声明
 ******************************************************************************/

static lv_obj_t* create_muyu_display_image(lv_obj_t *parent);

/*******************************************************************************
 * 静态函数实现
 ******************************************************************************/

/**
 * @brief 创建L2木鱼主界面的展示图片（非点击）
 */
static lv_obj_t* create_muyu_display_image(lv_obj_t *parent)
{
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
        return NULL;
    }
    
    /* 设置木鱼图片资源 */
    lv_img_set_src(img, get_muyu_image());
    
    /* 复用入口图标的处理方式 - 合适的尺寸和缩放 */
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 0.25f);
    lv_obj_set_size(img, icon_size, icon_size);
    
    /* 设置位置 - 居中显示 */
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    /* 适中的缩放比例 */
    float scale = g_ui_mgr.scale_factor * 0.4f;
    if (scale < 0.3f) scale = 0.3f;
    if (scale > 1.0f) scale = 1.0f;
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    /* 移除所有边距和边框 */
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    
    /* 设置为非点击 - 纯展示用途 */
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    
    return img;
}

/*******************************************************************************
 * L1 入口面板构建
 ******************************************************************************/

/**
 * @brief 构建赛博木鱼入口面板
 */
void l2_muyu_build_entrance_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 木鱼图片入口 - 上半部分居中 */
    g_ui_mgr.handles.group4_muyu.muyu_icon = create_entrance_icon(parent, get_muyu_image());
    if (g_ui_mgr.handles.group4_muyu.muyu_icon) {
        lv_obj_align(g_ui_mgr.handles.group4_muyu.muyu_icon, LV_ALIGN_CENTER, 0, -10);
    }

    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_muyu.muyu_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_muyu.muyu_hint, "赛博木鱼");
    lv_obj_add_style(g_ui_mgr.handles.group4_muyu.muyu_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_muyu.muyu_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_muyu.muyu_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/*******************************************************************************
 * L2 主界面构建
 ******************************************************************************/

/**
 * @brief 构建L2木鱼主界面
 */
void l2_muyu_build_main_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 创建左屏面板 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    /* 创建中屏面板 */
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    /* 创建右屏面板 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* ========== 左屏: 木鱼图片 ========== */
    g_ui_mgr.handles.l2_muyu_main.muyu_image = create_muyu_display_image(left);
    
    lv_obj_t *key_hint = lv_label_create(left);
    lv_label_set_text(key_hint, "按键1敲击");
    lv_obj_add_style(key_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(key_hint, lv_color_make(255, 215, 0), 0);
    lv_obj_align(key_hint, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    /* ========== 中屏: 功德计数 ========== */
    lv_obj_t *counter_title = lv_label_create(middle);
    lv_label_set_text(counter_title, "功德");
    lv_obj_add_style(counter_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(counter_title, lv_color_make(255, 215, 0), 0);
    lv_obj_align(counter_title, LV_ALIGN_TOP_MID, 0, 15);
    
    g_ui_mgr.handles.l2_muyu_main.counter_label = lv_label_create(middle);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.counter_label, "0");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.counter_label, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.counter_label, lv_color_make(255, 215, 0), 0);
    lv_obj_align(g_ui_mgr.handles.l2_muyu_main.counter_label, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *session_hint = lv_label_create(middle);
    lv_label_set_text(session_hint, "本次");
    lv_obj_add_style(session_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(session_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(session_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* ========== 右屏: 总计/等级 ========== */
    lv_obj_t *total_title = lv_label_create(right);
    lv_label_set_text(total_title, "总计");
    lv_obj_add_style(total_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(total_title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(total_title, LV_ALIGN_TOP_MID, 0, 10);
    
    g_ui_mgr.handles.l2_muyu_main.total_label = lv_label_create(right);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.total_label, "--");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.total_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.total_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.l2_muyu_main.total_label, total_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    g_ui_mgr.handles.l2_muyu_main.merit_label = lv_label_create(right);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.merit_label, "lv1");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.merit_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.merit_label, lv_color_make(144, 238, 144), 0);
    lv_obj_align(g_ui_mgr.handles.l2_muyu_main.merit_label, LV_ALIGN_CENTER, 0, 10);
    
    g_ui_mgr.handles.l2_muyu_main.reset_hint = lv_label_create(right);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.reset_hint, "按键2重置");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.reset_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.reset_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(g_ui_mgr.handles.l2_muyu_main.reset_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    /* 初始化木鱼数据 */
    if (g_ui_mgr.muyu_data.sound_enabled == 0) {
        g_ui_mgr.muyu_data.sound_enabled = true;
        g_ui_mgr.muyu_data.auto_save = true;
        g_ui_mgr.muyu_data.tap_effect_level = 1;
    }
}

/*******************************************************************************
 * 更新与控制
 ******************************************************************************/

/**
 * @brief 更新木鱼显示
 */
int l2_muyu_update_display(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    
    if (!g_ui_mgr.handles.l2_muyu_main.counter_label || 
        !lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.counter_label)) {
        return 0;
    }
    
    /* 获取木鱼计数 */
    uint32_t current_count = 0;
    uint32_t total_count = 0;
    screen_context_get_muyu_count(&current_count, &total_count);
    
    /* 更新内部数据 */
    g_ui_mgr.muyu_data.tap_count = current_count;
    g_ui_mgr.muyu_data.total_taps = total_count;
    
    /* 更新本次计数显示 */
    char counter_str[16];
    rt_snprintf(counter_str, sizeof(counter_str), "%u", current_count);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.counter_label, counter_str);
    
    /* 更新总计显示 */
    if (g_ui_mgr.handles.l2_muyu_main.total_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.total_label)) {
        char total_str[16];
        rt_snprintf(total_str, sizeof(total_str), "%u", total_count);
        lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.total_label, total_str);
    }
    
    /* 更新功德等级显示 */
    if (g_ui_mgr.handles.l2_muyu_main.merit_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.merit_label)) {
        const char *merit_text;
        if (total_count < 100) {
            merit_text = "lv1";
        } else if (total_count < 1000) {
            merit_text = "lv2";
        } else {
            merit_text = "lv3";
        }
        lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.merit_label, merit_text);
    }
    
    return 0;
}

/**
 * @brief 重置木鱼计数器
 */
int l2_muyu_reset_counter(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    screen_context_handle_muyu_reset();
    l2_muyu_update_display();
    
    return 0;
}
