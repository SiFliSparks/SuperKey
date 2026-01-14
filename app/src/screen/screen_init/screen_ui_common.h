/**
 * @file screen_ui_common.h
 * @brief 屏幕UI公共模块头文件 - 字体、样式、面板等基础功能
 */

#ifndef SCREEN_UI_COMMON_H
#define SCREEN_UI_COMMON_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 屏幕尺寸和布局宏定义
 ******************************************************************************/

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  128
#define TOTAL_WIDTH    (SCREEN_WIDTH * 3)

#define LEFT_X         0
#define MID_X          SCREEN_WIDTH  
#define RIGHT_X        (SCREEN_WIDTH * 2)

#define BASE_WIDTH     390
#define BASE_HEIGHT    450

#define USE_SCREEN_ANIMATIONS  1

/* 缩放宏 - 需要 get_scale_factor() 支持 */
#define SCALE_DPX(val) LV_DPX((val) * get_scale_factor())

/*******************************************************************************
 * 中文月份和星期数组
 ******************************************************************************/

extern const char* chinese_months[];
extern const char* chinese_weekdays[];

/*******************************************************************************
 * 缩放因子函数
 ******************************************************************************/

/**
 * @brief 计算屏幕缩放因子
 * @return 缩放因子值
 */
float calc_scale_factor(void);

/**
 * @brief 获取当前缩放因子
 * @return 当前缩放因子值
 */
float get_scale_factor(void);

/**
 * @brief 设置缩放因子
 * @param scale 缩放因子值
 */
void set_scale_factor(float scale);

/*******************************************************************************
 * 字体管理函数
 ******************************************************************************/

/**
 * @brief 创建所有字体和样式
 * @return 0成功，其他失败
 * @note 字体和样式直接存储在 g_ui_mgr.handles 中
 */
int create_fonts(void);

/**
 * @brief 清理所有字体资源
 */
void cleanup_fonts(void);

/*******************************************************************************
 * 字体获取函数
 ******************************************************************************/

lv_font_t* get_font_xsmall(void);
lv_font_t* get_font_small(void);
lv_font_t* get_font_medium(void);
lv_font_t* get_font_large(void);
lv_font_t* get_font_xlarge(void);
lv_font_t* get_font_xxlarge(void);

/*******************************************************************************
 * 样式获取函数
 ******************************************************************************/

lv_style_t* get_style_xsmall(void);
lv_style_t* get_style_small(void);
lv_style_t* get_style_medium(void);
lv_style_t* get_style_large(void);
lv_style_t* get_style_xlarge(void);
lv_style_t* get_style_xxlarge(void);

/*******************************************************************************
 * 屏幕和面板函数
 ******************************************************************************/

/**
 * @brief 设置屏幕基础样式
 * @param screen 屏幕对象
 */
void setup_screen_base_style(lv_obj_t *screen);

/**
 * @brief 创建面板
 * @param parent 父对象
 * @param x_pos X坐标位置
 * @return 创建的面板对象
 */
lv_obj_t* create_panel(lv_obj_t *parent, lv_coord_t x_pos);

/**
 * @brief 带动画加载屏幕
 * @param screen 目标屏幕
 * @param anim_type 动画类型
 * @param time 动画时间(ms)
 */
void load_screen_with_anim(lv_obj_t *screen, lv_scr_load_anim_t anim_type, uint32_t time);

/*******************************************************************************
 * 图标创建函数
 ******************************************************************************/

/**
 * @brief 创建入口图标（小尺寸）
 * @param parent 父对象
 * @param img_src 图片资源
 * @return 创建的图标对象
 */
lv_obj_t* create_entrance_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src);

/**
 * @brief 创建全尺寸图标
 * @param parent 父对象
 * @param img_src 图片资源
 * @return 创建的图标对象
 */
lv_obj_t* create_fullsize_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src);

/**
 * @brief 创建自定义全尺寸图标（更大缩放比例）
 * @param parent 父对象
 * @param img_src 图片资源
 * @return 创建的图标对象
 */
lv_obj_t* create_custom_fullsize_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src);

/*******************************************************************************
 * 仪表盘和图表函数
 ******************************************************************************/

/**
 * @brief 创建占用率圆弧仪表盘
 * @param parent 父对象
 * @param color 仪表颜色
 * @param label_out 输出参数，返回中心百分比标签
 * @return 创建的Arc对象
 */
lv_obj_t* create_usage_arc(lv_obj_t *parent, lv_color_t color, lv_obj_t **label_out);

/**
 * @brief 创建内存使用率图表（5个柱形）
 * @param parent 父对象
 * @param color 柱子颜色
 * @return 创建的图表容器对象
 */
lv_obj_t* create_memory_chart(lv_obj_t *parent, lv_color_t color);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_COMMON_H */
