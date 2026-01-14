/**
 * @file screen_ui_l2_time.c
 * @brief L2数字时钟UI模块实现
 * 
 * L2数字时钟页面:
 * - 左屏: 小时显示 (十位/个位)
 * - 中屏: 分钟显示 (十位/个位)
 * - 右屏: 秒钟显示 (十位/个位)
 * 
 * 特性:
 * - 使用数字图片显示时间
 * - 支持实时更新
 * - 最大化图片显示
 */

#include "screen_ui_l2_time.h"
#include "../../../screen/screen_init/screen_ui_common.h"
#include "../../../screen/screen_init/screen_ui_resources.h"
#include "../../../screen/screen_ui_manager.h"
#include <time.h>

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * 静态函数声明
 ******************************************************************************/

static lv_obj_t* create_digit_image(lv_obj_t *parent, int digit, lv_coord_t x_offset, lv_coord_t y_offset);
static void update_digit_image(lv_obj_t *img_obj, int digit);

/*******************************************************************************
 * 静态函数实现
 ******************************************************************************/

/**
 * @brief 创建单个数字图片对象
 * @param parent 父容器
 * @param digit 要显示的数字（0-9）
 * @param x_offset X轴偏移量
 * @param y_offset Y轴偏移量
 * @return 创建的图片对象指针
 */
static lv_obj_t* create_digit_image(lv_obj_t *parent, int digit, lv_coord_t x_offset, lv_coord_t y_offset)
{
    if (!parent) {
        return NULL;
    }
    
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
        return NULL;
    }
    
    /* 设置图片资源 */
    lv_img_set_src(img, get_digit_image(digit));
    
    /* 最大化图片容器尺寸 - 占用几乎全部显示区域 */
    lv_coord_t img_width = (lv_coord_t)(SCREEN_WIDTH * 0.5f);    /* 64像素，占满一半板块 */
    lv_coord_t img_height = (lv_coord_t)(SCREEN_HEIGHT * 1.0f);  /* 128像素，占满整个高度 */
    lv_obj_set_size(img, img_width, img_height);
    
    /* 设置位置 - 无边距，完全贴边 */
    lv_obj_set_pos(img, x_offset, y_offset);
    
    /* 激进缩放 - 在原有基础上再放大50% */
    float max_scale = g_ui_mgr.scale_factor * 1.5f;
    
    /* 扩大缩放范围限制 */
    if (max_scale < 0.8f) max_scale = 0.8f;
    if (max_scale > 4.0f) max_scale = 4.0f;  /* 允许更大的缩放 */
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * max_scale));
    lv_img_set_antialias(img, true);                    /* 抗锯齿 */
    lv_img_set_pivot(img, img_width/2, img_height/2);   /* 居中缩放点 */
    
    /* 移除所有内边距和边框 */
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    lv_obj_set_style_outline_width(img, 0, 0);
    
    return img;
}

/**
 * @brief 更新单个数字图片显示
 * @param img_obj 图片对象
 * @param digit 新的数字值（0-9）
 */
static void update_digit_image(lv_obj_t *img_obj, int digit)
{
    if (!img_obj || !lv_obj_is_valid(img_obj)) {
        return;
    }
    
    lv_img_set_src(img_obj, get_digit_image(digit));
}

/*******************************************************************************
 * L2 数字时钟页面构建
 ******************************************************************************/

/**
 * @brief 构建L2数字时钟页面 - 纯图片数字时钟显示
 */
void l2_time_build_digital_clock_page(lv_obj_t *screen)
{
    if (!screen) return;
    
    /* 创建左屏面板 - 小时显示 */
    lv_obj_t *left = lv_obj_create(screen);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(left, LEFT_X, 0);
    lv_obj_set_style_bg_color(left, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    
    /* 创建中屏面板 - 分钟显示 */
    lv_obj_t *middle = lv_obj_create(screen);
    lv_obj_remove_style_all(middle);
    lv_obj_set_size(middle, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(middle, MID_X, 0);
    lv_obj_set_style_bg_color(middle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
    
    /* 创建右屏面板 - 秒钟显示 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(right, RIGHT_X, 0);
    lv_obj_set_style_bg_color(right, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, 0);
    
    /* 获取当前时间 */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    int hour = tm_info ? tm_info->tm_hour : 0;
    int min = tm_info ? tm_info->tm_min : 0;
    int sec = tm_info ? tm_info->tm_sec : 0;
    
    lv_coord_t no_spacing = 0;
    lv_coord_t no_offset = 0;
    
    /* ========== 左屏: 小时 ========== */
    g_ui_mgr.handles.l2_digital_clock.hour_tens = create_digit_image(
        left, 
        hour / 10,
        no_spacing,
        no_offset
    );
    
    g_ui_mgr.handles.l2_digital_clock.hour_units = create_digit_image(
        left,
        hour % 10,
        SCREEN_WIDTH/2,
        no_offset
    );
    
    /* ========== 中屏: 分钟 ========== */
    g_ui_mgr.handles.l2_digital_clock.min_tens = create_digit_image(
        middle,
        min / 10,
        no_spacing,
        no_offset
    );
    
    g_ui_mgr.handles.l2_digital_clock.min_units = create_digit_image(
        middle,
        min % 10,
        SCREEN_WIDTH/2,
        no_offset
    );
    
    /* ========== 右屏: 秒钟 ========== */
    g_ui_mgr.handles.l2_digital_clock.sec_tens = create_digit_image(
        right,
        sec / 10,
        no_spacing,
        no_offset
    );
    
    g_ui_mgr.handles.l2_digital_clock.sec_units = create_digit_image(
        right,
        sec % 10,
        SCREEN_WIDTH/2,
        no_offset
    );
}

/*******************************************************************************
 * L2 数字时钟更新
 ******************************************************************************/

/**
 * @brief 更新L2数字时钟显示
 */
int l2_time_update_digital_clock(void)
{
    /* 检查数字时钟UI对象是否存在，如果不存在说明不在时间详情L2页面 */
    if (!g_ui_mgr.handles.l2_digital_clock.hour_tens || 
        !lv_obj_is_valid(g_ui_mgr.handles.l2_digital_clock.hour_tens)) {
        return 0; /* 不是时间详情页面，不更新数字时钟 */
    }
    
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        return -1; /* 时间获取失败 */
    }
    
    struct tm *tm_info = localtime(&now);
    if (!tm_info) {
        return -1; /* 时间转换失败 */
    }
    
    /* 提取小时、分钟、秒钟 */
    int hour = tm_info->tm_hour;
    int min = tm_info->tm_min;
    int sec = tm_info->tm_sec;
    
    /* 更新小时显示 */
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.hour_tens, hour / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.hour_units, hour % 10);
    
    /* 更新分钟显示 */
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.min_tens, min / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.min_units, min % 10);
    
    /* 更新秒钟显示 */
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.sec_tens, sec / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.sec_units, sec % 10);
    
    /* 调试用：检测秒数变化 */
    static int last_sec = -1;
    if (sec != last_sec) {
        last_sec = sec;
    }
    
    return 0;
}
