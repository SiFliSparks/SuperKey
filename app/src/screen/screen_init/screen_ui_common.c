/**
 * @file screen_ui_common.c
 * @brief 屏幕UI公共模块实现
 * 
 * 方案A实现：直接操作g_ui_mgr.handles中的字体和样式，
 * 保持与现有代码（如MP3模块）的兼容性
 */

#include "screen_ui_common.h"
#include "../screen_ui_manager.h"
#include "lv_tiny_ttf.h"
#include <rtthread.h>

/*********************
 *   EXTERNAL DATA
 *********************/
/* 全局UI管理器 - 在screen_ui_manager.c中定义 */
extern screen_ui_manager_t g_ui_mgr;

/* 外部字体数据 */
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;

/*********************
 *   STATIC VARIABLES
 *********************/
/* 缩放因子 - 本地缓存，同时会设置到g_ui_mgr.scale_factor */
static float s_scale_factor = 0.0f;
static bool s_fonts_initialized = false;

/*********************
 *   EXPORTED DATA
 *********************/
/* 中文月份数组 */
const char* chinese_months[12] = {
    "一月", "二月", "三月", "四月", "五月", "六月",
    "七月", "八月", "九月", "十月", "十一月", "十二月"
};

/* 中文星期数组 */
const char* chinese_weekdays[7] = {
    "周日", "周一", "周二", "周三", "周四", "周五", "周六"
};

/*********************
 *   SCALE FACTOR
 *********************/
float calc_scale_factor(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t scr_width = lv_disp_get_hor_res(disp);
    lv_coord_t scr_height = lv_disp_get_ver_res(disp);

    float scale_x = (float)scr_width / BASE_WIDTH;
    float scale_y = (float)scr_height / BASE_HEIGHT;

    s_scale_factor = (scale_x < scale_y) ? scale_x : scale_y;
    
    /* 同步到g_ui_mgr */
    g_ui_mgr.scale_factor = s_scale_factor;
    
    return s_scale_factor;
}

float get_scale_factor(void)
{
    if (s_scale_factor == 0.0f) {
        return calc_scale_factor();
    }
    return s_scale_factor;
}

void set_scale_factor(float factor)
{
    s_scale_factor = factor;
    g_ui_mgr.scale_factor = factor;
}

/*********************
 *   FONT MANAGEMENT
 *********************/
int create_fonts(void)
{
    if (s_fonts_initialized) {
        return 0;
    }
    
    float scale = get_scale_factor();
    
    /* 基准字体尺寸 */
    const int base_font_xsmall = 20;
    const int base_font_small = 25;
    const int base_font_medium = 30;
    const int base_font_large = 35;
    const int base_font_xlarge = 43;
    const int base_font_xxlarge = 65;

    /* 计算实际字体尺寸 */
    const int font_size_xsmall = (int)(base_font_xsmall * scale + 0.5f);
    const int font_size_small = (int)(base_font_small * scale + 0.5f);
    const int font_size_medium = (int)(base_font_medium * scale + 0.5f);
    const int font_size_large = (int)(base_font_large * scale + 0.5f);
    const int font_size_xlarge = (int)(base_font_xlarge * scale + 0.5f);
    const int font_size_xxlarge = (int)(base_font_xxlarge * scale + 0.5f);

    /* 创建字体 - 直接设置到g_ui_mgr.handles */
    g_ui_mgr.handles.font_xsmall = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_xsmall);
    g_ui_mgr.handles.font_small = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_small);
    g_ui_mgr.handles.font_medium = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_medium);
    g_ui_mgr.handles.font_large = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_large);
    g_ui_mgr.handles.font_xlarge = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_xlarge);
    g_ui_mgr.handles.font_xxlarge = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_xxlarge);

    /* 检查字体创建是否成功 */
    if (!g_ui_mgr.handles.font_small || !g_ui_mgr.handles.font_medium || 
        !g_ui_mgr.handles.font_large || !g_ui_mgr.handles.font_xlarge) {
        return -RT_ERROR;
    }

    /* 初始化样式 - 直接设置到g_ui_mgr.handles */
    lv_style_init(&g_ui_mgr.handles.style_xsmall);
    lv_style_set_text_font(&g_ui_mgr.handles.style_xsmall, g_ui_mgr.handles.font_xsmall);
    lv_style_set_text_align(&g_ui_mgr.handles.style_xsmall, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&g_ui_mgr.handles.style_xsmall, lv_color_hex(0xFFFFFF));

    lv_style_init(&g_ui_mgr.handles.style_small);
    lv_style_set_text_font(&g_ui_mgr.handles.style_small, g_ui_mgr.handles.font_small);
    lv_style_set_text_align(&g_ui_mgr.handles.style_small, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&g_ui_mgr.handles.style_small, lv_color_hex(0xFFFFFF));

    lv_style_init(&g_ui_mgr.handles.style_medium);
    lv_style_set_text_font(&g_ui_mgr.handles.style_medium, g_ui_mgr.handles.font_medium);
    lv_style_set_text_align(&g_ui_mgr.handles.style_medium, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&g_ui_mgr.handles.style_medium, lv_color_hex(0xFFFFFF));

    lv_style_init(&g_ui_mgr.handles.style_large);
    lv_style_set_text_font(&g_ui_mgr.handles.style_large, g_ui_mgr.handles.font_large);
    lv_style_set_text_align(&g_ui_mgr.handles.style_large, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&g_ui_mgr.handles.style_large, lv_color_hex(0xFFFFFF));

    lv_style_init(&g_ui_mgr.handles.style_xlarge);
    lv_style_set_text_font(&g_ui_mgr.handles.style_xlarge, g_ui_mgr.handles.font_xlarge);
    lv_style_set_text_align(&g_ui_mgr.handles.style_xlarge, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&g_ui_mgr.handles.style_xlarge, lv_color_hex(0xFFFFFF));

    lv_style_init(&g_ui_mgr.handles.style_xxlarge);
    lv_style_set_text_font(&g_ui_mgr.handles.style_xxlarge, g_ui_mgr.handles.font_xxlarge);
    lv_style_set_text_align(&g_ui_mgr.handles.style_xxlarge, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&g_ui_mgr.handles.style_xxlarge, lv_color_hex(0xFFFFFF));

    s_fonts_initialized = true;
    return 0;
}

void cleanup_fonts(void)
{
    if (!s_fonts_initialized) {
        return;
    }
    
    /* 清理字体 - 直接操作g_ui_mgr.handles */
    if (g_ui_mgr.handles.font_xsmall) {
        lv_tiny_ttf_destroy(g_ui_mgr.handles.font_xsmall);
        g_ui_mgr.handles.font_xsmall = NULL;
    }
    if (g_ui_mgr.handles.font_small) {
        lv_tiny_ttf_destroy(g_ui_mgr.handles.font_small);
        g_ui_mgr.handles.font_small = NULL;
    }
    if (g_ui_mgr.handles.font_medium) {
        lv_tiny_ttf_destroy(g_ui_mgr.handles.font_medium);
        g_ui_mgr.handles.font_medium = NULL;
    }
    if (g_ui_mgr.handles.font_large) {
        lv_tiny_ttf_destroy(g_ui_mgr.handles.font_large);
        g_ui_mgr.handles.font_large = NULL;
    }
    if (g_ui_mgr.handles.font_xlarge) {
        lv_tiny_ttf_destroy(g_ui_mgr.handles.font_xlarge);
        g_ui_mgr.handles.font_xlarge = NULL;
    }
    if (g_ui_mgr.handles.font_xxlarge) {
        lv_tiny_ttf_destroy(g_ui_mgr.handles.font_xxlarge);
        g_ui_mgr.handles.font_xxlarge = NULL;
    }
    
    s_fonts_initialized = false;
}

/*********************
 *   STYLE GETTERS
 *********************/
lv_style_t* get_style_xsmall(void)
{
    return &g_ui_mgr.handles.style_xsmall;
}

lv_style_t* get_style_small(void)
{
    return &g_ui_mgr.handles.style_small;
}

lv_style_t* get_style_medium(void)
{
    return &g_ui_mgr.handles.style_medium;
}

lv_style_t* get_style_large(void)
{
    return &g_ui_mgr.handles.style_large;
}

lv_style_t* get_style_xlarge(void)
{
    return &g_ui_mgr.handles.style_xlarge;
}

lv_style_t* get_style_xxlarge(void)
{
    return &g_ui_mgr.handles.style_xxlarge;
}

/*********************
 *   FONT GETTERS
 *********************/
lv_font_t* get_font_xsmall(void)
{
    return g_ui_mgr.handles.font_xsmall;
}

lv_font_t* get_font_small(void)
{
    return g_ui_mgr.handles.font_small;
}

lv_font_t* get_font_medium(void)
{
    return g_ui_mgr.handles.font_medium;
}

lv_font_t* get_font_large(void)
{
    return g_ui_mgr.handles.font_large;
}

lv_font_t* get_font_xlarge(void)
{
    return g_ui_mgr.handles.font_xlarge;
}

lv_font_t* get_font_xxlarge(void)
{
    return g_ui_mgr.handles.font_xxlarge;
}

/*********************
 *   SCREEN/PANEL
 *********************/
void setup_screen_base_style(lv_obj_t *screen)
{
    if (!screen) return;
    
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* create_panel(lv_obj_t *parent, lv_coord_t x_pos)
{
    if (!parent) return NULL;
    
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(panel, x_pos, 0);
    lv_obj_set_style_bg_color(panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    return panel;
}

void load_screen_with_anim(lv_obj_t *screen, lv_scr_load_anim_t anim_type, uint32_t time)
{
    if (!screen) return;
    
#if USE_SCREEN_ANIMATIONS
    lv_scr_load_anim(screen, anim_type, time, 0, false);
#else
    lv_scr_load(screen);
#endif
}

/*********************
 *   ICON CREATION
 *********************/
lv_obj_t* create_entrance_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src)
{
    if (!parent || !img_src) return NULL;
    
    lv_obj_t *img = lv_img_create(parent);
    if (!img) return NULL;
    
    lv_image_set_src(img, img_src);
    
    /* 入口图标使用较小尺寸 */
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 0.1f);
    lv_obj_set_size(img, icon_size, icon_size);
    
    /* 缩放比例 - 缩小50% */
    float scale = get_scale_factor() * 0.50f;
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    /* 移除所有边距和边框 */
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    
    return img;
}

lv_obj_t* create_fullsize_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src)
{
    if (!parent || !img_src) return NULL;
    
    lv_obj_t *img = lv_img_create(parent);
    if (!img) return NULL;
    
    lv_img_set_src(img, img_src);
    
    /* 全板块尺寸 - 占满整个面板 */
    lv_coord_t icon_size = SCREEN_WIDTH;
    lv_obj_set_size(img, icon_size, icon_size);
    
    /* 居中显示 */
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    /* 适中的缩放比例 */
    float scale = get_scale_factor() * 0.6f;
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    /* 移除所有边距和边框 */
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    lv_obj_add_flag(img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    
    return img;
}

lv_obj_t* create_custom_fullsize_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src)
{
    if (!parent || !img_src) return NULL;
    
    lv_obj_t *img = lv_img_create(parent);
    if (!img) return NULL;
    
    lv_img_set_src(img, img_src);
    
    /* 全板块尺寸 */
    lv_coord_t icon_size = SCREEN_WIDTH;
    lv_obj_set_size(img, icon_size, icon_size);
    
    /* 居中显示 */
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    /* 自定义图片专用缩放比例 */
    float scale = get_scale_factor() * 0.75f; 
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    /* 移除所有边距和边框 */
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    lv_obj_add_flag(img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    
    return img;
}

/*********************
 *   GAUGES & CHARTS
 *********************/
lv_obj_t* create_usage_arc(lv_obj_t *parent, lv_color_t color, lv_obj_t **label_out)
{
    if (!parent) return NULL;
    
    /* 仪表盘尺寸 */
    lv_coord_t arc_size = 100;
    
    /* 创建Arc对象 */
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, arc_size, arc_size);
    
    /* 设置Arc角度范围 */
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    
    /* 移除旋钮 */
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    
    /* 背景弧线样式 */
    lv_obj_set_style_arc_color(arc, lv_color_make(40, 40, 40), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    
    /* 前景弧线样式 */
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    
    /* 移除背景 */
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(arc, 0, 0);
    lv_obj_set_style_pad_all(arc, 0, 0);
    
    /* 在Arc中心创建百分比标签 */
    lv_obj_t *label = lv_label_create(arc);
    lv_label_set_text(label, "--%");
    lv_obj_add_style(label, get_style_medium(), 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_center(label);
    
    /* 返回标签指针 */
    if (label_out) {
        *label_out = label;
    }
    
    return arc;
}

lv_obj_t* create_memory_chart(lv_obj_t *parent, lv_color_t color)
{
    if (!parent) return NULL;
    
    lv_obj_t *container = lv_obj_create(parent);
    /* 宽度约为屏幕的一半，高度与CPU/GPU图表一致 */
    lv_obj_set_size(container, (SCREEN_WIDTH / 2) - 5, 50);
    
    /* 容器样式 */
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    
    lv_coord_t bar_width = 8;
    lv_coord_t bar_gap = 3;
    lv_coord_t start_x = 3;
    lv_coord_t max_bar_height = 46;
    
    /* 创建5个bar（每个包含背景条+前景条） */
    for (int i = 0; i < 5; i++) {
        /* 先创建灰色背景条 */
        lv_obj_t *bg_bar = lv_obj_create(container);
        lv_obj_set_size(bg_bar, bar_width, max_bar_height);
        lv_obj_set_pos(bg_bar, start_x + i * (bar_width + bar_gap), 2);
        
        /* 背景条样式 - 深灰色 */
        lv_obj_set_style_bg_color(bg_bar, lv_color_make(50, 50, 50), 0);
        lv_obj_set_style_bg_opa(bg_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bg_bar, 0, 0);
        lv_obj_set_style_radius(bg_bar, 0, 0);
        lv_obj_set_style_pad_all(bg_bar, 0, 0);
        
        /* 再创建前景数据条 */
        lv_obj_t *bar = lv_obj_create(container);
        lv_obj_set_size(bar, bar_width, 2);
        lv_obj_set_pos(bar, start_x + i * (bar_width + bar_gap), 48);
        
        /* bar样式 */
        lv_obj_set_style_bg_color(bar, color, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
    }
    
    return container;
}
