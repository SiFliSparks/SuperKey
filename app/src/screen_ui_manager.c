/**
 * @file screen_ui_manager.c - 修复编译错误版本
 * @brief 线程安全的UI管理器实现 - 修复g_core访问问题
 */

#include "screen_ui_manager.h"
#include "screen_core.h"  // 添加这个头文件引用
#include "data_manager.h"
#include "sht30_controller.h"
#include "screen_context.h"
#include "lv_tiny_ttf.h"
#include <rtthread.h>
#include <time.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  128
#define TOTAL_WIDTH    (SCREEN_WIDTH * 3)

#define LEFT_X         0
#define MID_X          SCREEN_WIDTH  
#define RIGHT_X        (SCREEN_WIDTH * 2)

#define BASE_WIDTH     390
#define BASE_HEIGHT    450
#define SCALE_DPX(val) LV_DPX((val) * g_ui_mgr.scale_factor)

/* 中文月份和星期数组 */
static const char* chinese_months[] = {
    "一月", "二月", "三月", "四月", "五月", "六月",
    "七月", "八月", "九月", "十月", "十一月", "十二月"
};

static const char* chinese_weekdays[] = {
    "周日", "周一", "周二", "周三", "周四", "周五", "周六"
};

/* 外部字体数据声明 */
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;

/* 数字图片资源声明 */
extern const lv_image_dsc_t t0;  // 数字0图片
extern const lv_image_dsc_t t1;  // 数字1图片
extern const lv_image_dsc_t t2;  // 数字2图片
extern const lv_image_dsc_t t3;  // 数字3图片
extern const lv_image_dsc_t t4;  // 数字4图片
extern const lv_image_dsc_t t5;  // 数字5图片
extern const lv_image_dsc_t t6;  // 数字6图片
extern const lv_image_dsc_t t7;  // 数字7图片
extern const lv_image_dsc_t t8;  // 数字8图片
extern const lv_image_dsc_t t9;  // 数字9图片


/* 数字图片资源数组，便于通过索引访问 */
static const lv_image_dsc_t* digit_images[10] = {
    &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8, &t9
};

/* 木鱼图片资源声明 - 需要添加到文件顶部 */
extern const lv_image_dsc_t muyu;  // 木鱼图片资源

/*********************
 *  STATIC VARIABLES
 *********************/
static screen_ui_manager_t g_ui_mgr = {0};

/*********************
 *  STATIC PROTOTYPES
 *********************/
/* 字体和缩放 */
static float get_scale_factor(void);
static int create_fonts(void);
static void cleanup_fonts(void);

/* 基础UI */
static int create_base_ui(void);
static void cleanup_base_ui(void);

/* Group 1 UI构建 */
static void build_left_datetime_panel(lv_obj_t *parent);
static void build_middle_weather_panel(lv_obj_t *parent);
static void build_right_stock_panel(lv_obj_t *parent);

/* Group 2 UI构建 */
static void build_left_cpu_gpu_panel(lv_obj_t *parent);
static void build_middle_memory_panel(lv_obj_t *parent);
static void build_right_network_panel(lv_obj_t *parent);

/* Group 3 UI构建 */
static void build_left_media_panel(lv_obj_t *parent);
static void build_middle_web_panel(lv_obj_t *parent);
static void build_right_shortcut_panel(lv_obj_t *parent);

/* L2 UI构建 - 数字时钟相关 */
static const lv_image_dsc_t* get_digit_image(int digit);
static const lv_image_dsc_t* get_muyu_image(void);
static lv_obj_t* create_digit_image(lv_obj_t *parent, int digit, lv_coord_t x_offset, lv_coord_t y_offset);
static void update_digit_image(lv_obj_t *img_obj, int digit);
static void build_l2_time_detail_page(void);
static int screen_ui_update_l2_digital_clock(void);

static void build_l2_media_control_page(void);
static void build_l2_web_control_page(void);
static void build_l2_shortcut_control_page(void);

/* 安全清理 */
static void safe_cleanup_ui_objects(void);

/*********************
 *   HELPER FUNCTIONS
 *********************/

/**
 * 获取当前屏幕尺寸并计算缩放因子
 */
static float get_scale_factor(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t scr_width = lv_disp_get_hor_res(disp);
    lv_coord_t scr_height = lv_disp_get_ver_res(disp);

    float scale_x = (float)scr_width / BASE_WIDTH;
    float scale_y = (float)scr_height / BASE_HEIGHT;

    return (scale_x < scale_y) ? scale_x : scale_y;
}

/**
 * 创建动态字体
 */
static int create_fonts(void)
{
    const int base_font_xsmall = 20;
    const int base_font_small = 25;
    const int base_font_medium = 30;
    const int base_font_large = 35;
    const int base_font_xlarge = 43;
    const int base_font_xxlarge = 65;

    const int font_size_xsmall = (int)(base_font_xsmall * g_ui_mgr.scale_factor + 0.5f);
    const int font_size_small = (int)(base_font_small * g_ui_mgr.scale_factor + 0.5f);
    const int font_size_medium = (int)(base_font_medium * g_ui_mgr.scale_factor + 0.5f);
    const int font_size_large = (int)(base_font_large * g_ui_mgr.scale_factor + 0.5f);
    const int font_size_xlarge = (int)(base_font_xlarge * g_ui_mgr.scale_factor + 0.5f);
    const int font_size_xxlarge = (int)(base_font_xxlarge * g_ui_mgr.scale_factor + 0.5f);

    /* 创建字体 */
    g_ui_mgr.handles.font_xsmall = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_xsmall);
    g_ui_mgr.handles.font_small = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_small);
    g_ui_mgr.handles.font_medium = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_medium);
    g_ui_mgr.handles.font_large = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_large);
    g_ui_mgr.handles.font_xlarge = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_xlarge);
    g_ui_mgr.handles.font_xxlarge = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size_xxlarge);

    if (!g_ui_mgr.handles.font_small || !g_ui_mgr.handles.font_medium || 
        !g_ui_mgr.handles.font_large || !g_ui_mgr.handles.font_xlarge) {
        rt_kprintf("[UIManager] ERROR: Font creation failed\n");
        return -RT_ERROR;
    }

    /* 初始化样式 */
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

    rt_kprintf("[UIManager] Dynamic Chinese fonts created with scale factor: %.2f\n", g_ui_mgr.scale_factor);
    return 0;
}

/**
 * 清理字体资源
 */
static void cleanup_fonts(void)
{
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
}

/**
 * 创建基础UI结构
 */
static int create_base_ui(void)
{
    /* 获取当前屏幕 */
    lv_obj_t *scr = lv_scr_act();
    if (!scr) {
        rt_kprintf("[UIManager] ERROR: No active screen\n");
        return -RT_ERROR;
    }

    /* 设置背景为纯黑色 */
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* 创建根容器 */
    g_ui_mgr.handles.root = lv_obj_create(scr);
    if (!g_ui_mgr.handles.root) {
        rt_kprintf("[UIManager] ERROR: Failed to create root container\n");
        return -RT_ERROR;
    }
    lv_obj_remove_style_all(g_ui_mgr.handles.root);
    lv_obj_set_size(g_ui_mgr.handles.root, TOTAL_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(g_ui_mgr.handles.root, 0, 0);
    lv_obj_set_style_bg_color(g_ui_mgr.handles.root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_ui_mgr.handles.root, LV_OPA_COVER, 0);

    /* 创建三个面板 */
    g_ui_mgr.handles.left_panel = lv_obj_create(g_ui_mgr.handles.root);
    g_ui_mgr.handles.middle_panel = lv_obj_create(g_ui_mgr.handles.root);
    g_ui_mgr.handles.right_panel = lv_obj_create(g_ui_mgr.handles.root);

    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) {
        rt_kprintf("[UIManager] ERROR: Failed to create panels\n");
        return -RT_ERROR;
    }

    /* 配置左屏面板 */
    lv_obj_remove_style_all(g_ui_mgr.handles.left_panel);
    lv_obj_set_size(g_ui_mgr.handles.left_panel, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(g_ui_mgr.handles.left_panel, LEFT_X, 0);
    lv_obj_set_style_bg_color(g_ui_mgr.handles.left_panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_ui_mgr.handles.left_panel, LV_OPA_COVER, 0);

    /* 配置中屏面板 */
    lv_obj_remove_style_all(g_ui_mgr.handles.middle_panel);
    lv_obj_set_size(g_ui_mgr.handles.middle_panel, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(g_ui_mgr.handles.middle_panel, MID_X, 0);
    lv_obj_set_style_bg_color(g_ui_mgr.handles.middle_panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_ui_mgr.handles.middle_panel, LV_OPA_COVER, 0);

    /* 配置右屏面板 */
    lv_obj_remove_style_all(g_ui_mgr.handles.right_panel);
    lv_obj_set_size(g_ui_mgr.handles.right_panel, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(g_ui_mgr.handles.right_panel, RIGHT_X, 0);
    lv_obj_set_style_bg_color(g_ui_mgr.handles.right_panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_ui_mgr.handles.right_panel, LV_OPA_COVER, 0);

    rt_kprintf("[UIManager] Base UI structure created successfully\n");
    return 0;
}

/**
 * 清理基础UI
 */
static void cleanup_base_ui(void)
{
    if (g_ui_mgr.handles.root && lv_obj_is_valid(g_ui_mgr.handles.root)) {
        lv_obj_del(g_ui_mgr.handles.root);
    }
    
    /* 清空所有句柄 */
    memset(&g_ui_mgr.handles, 0, sizeof(screen_ui_handles_t));
}

/**
 * 安全清理UI对象
 */
static void safe_cleanup_ui_objects(void)
{
    rt_kprintf("[UIManager] Starting safe UI cleanup...\n");

    /* 清理面板内容，但保留面板本身 */
    if (g_ui_mgr.handles.left_panel && lv_obj_is_valid(g_ui_mgr.handles.left_panel)) {
        lv_obj_clean(g_ui_mgr.handles.left_panel);
    }
    if (g_ui_mgr.handles.middle_panel && lv_obj_is_valid(g_ui_mgr.handles.middle_panel)) {
        lv_obj_clean(g_ui_mgr.handles.middle_panel);
    }
    if (g_ui_mgr.handles.right_panel && lv_obj_is_valid(g_ui_mgr.handles.right_panel)) {
        lv_obj_clean(g_ui_mgr.handles.right_panel);
    }

    /* 清空组件句柄（但保留面板和根容器） */
    memset(&g_ui_mgr.handles.group1_time, 0, sizeof(g_ui_mgr.handles.group1_time));
    memset(&g_ui_mgr.handles.group1_weather, 0, sizeof(g_ui_mgr.handles.group1_weather));
    memset(&g_ui_mgr.handles.group1_stock, 0, sizeof(g_ui_mgr.handles.group1_stock));
    memset(&g_ui_mgr.handles.group2_cpu_gpu, 0, sizeof(g_ui_mgr.handles.group2_cpu_gpu));
    memset(&g_ui_mgr.handles.group2_memory, 0, sizeof(g_ui_mgr.handles.group2_memory));
    memset(&g_ui_mgr.handles.group2_network, 0, sizeof(g_ui_mgr.handles.group2_network));
    
    // 清空L2数字时钟句柄
    memset(&g_ui_mgr.handles.l2_digital_clock, 0, sizeof(g_ui_mgr.handles.l2_digital_clock));

    lv_timer_handler(); /* 处理清理操作 */
    rt_kprintf("[UIManager] Safe UI cleanup completed\n");
}

/*********************
 *   GROUP 1 UI BUILD
 *********************/

/**
 * 构建左屏 - 日期时间面板
 */
static void build_left_datetime_panel(lv_obj_t *parent)
{
    /* 获取当前时间用于初始显示 */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[32], year_str[16], weekday_str[16];
    
    /* 年份 - 上半部分顶部，左对齐，略小字号 */
    g_ui_mgr.handles.group1_time.year_label = lv_label_create(parent);
    if (tm_info) {
        rt_snprintf(year_str, sizeof(year_str), "%d年", tm_info->tm_year + 1900);
    } else {
        rt_snprintf(year_str, sizeof(year_str), "2025年");
    }
    lv_label_set_text(g_ui_mgr.handles.group1_time.year_label, year_str);
    lv_obj_add_style(g_ui_mgr.handles.group1_time.year_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.year_label, lv_color_make(180, 180, 180), 0);
    lv_obj_align(g_ui_mgr.handles.group1_time.year_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* 日期 - 年份下方，左对齐，最大字号 */
    g_ui_mgr.handles.group1_time.date_label = lv_label_create(parent);
    if (tm_info) {
        rt_snprintf(date_str, sizeof(date_str), "%s%d日", 
                   chinese_months[tm_info->tm_mon], tm_info->tm_mday);
    } else {
        rt_snprintf(date_str, sizeof(date_str), "十二月25日");
    }
    lv_label_set_text(g_ui_mgr.handles.group1_time.date_label, date_str);
    lv_obj_add_style(g_ui_mgr.handles.group1_time.date_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.date_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_time.date_label, g_ui_mgr.handles.group1_time.year_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 星期 - 日期下方，左对齐，比日期略小 */
    g_ui_mgr.handles.group1_time.weekday_label = lv_label_create(parent);
    if (tm_info) {
        rt_snprintf(weekday_str, sizeof(weekday_str), "%s", chinese_weekdays[tm_info->tm_wday]);
    } else {
        rt_snprintf(weekday_str, sizeof(weekday_str), "周一");
    }
    lv_label_set_text(g_ui_mgr.handles.group1_time.weekday_label, weekday_str);
    lv_obj_add_style(g_ui_mgr.handles.group1_time.weekday_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.weekday_label, lv_color_make(200, 200, 200), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_time.weekday_label, g_ui_mgr.handles.group1_time.date_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 时间 - 下半部分居中，略大字号 */
    g_ui_mgr.handles.group1_time.time_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_time.time_label, "00:00");
    lv_obj_add_style(g_ui_mgr.handles.group1_time.time_label, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_time.time_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group1_time.time_label, LV_ALIGN_CENTER, 0, 40);
}

/**
 * 构建中屏 - 天气信息面板
 */
static void build_middle_weather_panel(lv_obj_t *parent)
{
    /* 城市名 - 顶部左对齐 */
    g_ui_mgr.handles.group1_weather.city_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.city_label, "杭州");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.city_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.city_label, lv_color_make(100, 200, 255), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* 温度 - 城市名右侧，相同字号 */
    g_ui_mgr.handles.group1_weather.temperature_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.temperature_label, "0.0°C");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.temperature_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.temperature_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.temperature_label, g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    /* 天气描述 - 城市名下方，左对齐 */
    g_ui_mgr.handles.group1_weather.weather_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.weather_label, "晴");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.weather_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.weather_label, lv_color_make(255, 220, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.weather_label, g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 湿度 - 天气描述下方，左对齐 */
    g_ui_mgr.handles.group1_weather.humidity_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.humidity_label, "湿度: 0%");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.humidity_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.humidity_label, lv_color_make(150, 200, 255), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.humidity_label, g_ui_mgr.handles.group1_weather.weather_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 气压 - 湿度下方，左对齐 */
    g_ui_mgr.handles.group1_weather.pressure_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.pressure_label, "1013hPa");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.pressure_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.pressure_label, lv_color_make(150, 200, 255), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.pressure_label, g_ui_mgr.handles.group1_weather.humidity_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    
    /* SHT30传感器数据 - 底部居中 */
    g_ui_mgr.handles.group1_weather.sensor_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "传感器: --°C --%");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.sensor_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.sensor_label, lv_color_make(100, 255, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.sensor_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/**
 * 构建右屏 - 股票信息面板
 */
static void build_right_stock_panel(lv_obj_t *parent)
{
    /* 股票名称 - 顶部居中 */
    g_ui_mgr.handles.group1_stock.name_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.name_label, "上证指数");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.name_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.name_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group1_stock.name_label, LV_ALIGN_TOP_MID, 0, 0);
    
    /* 当前价格 - 标题下方，更大字号 */
    g_ui_mgr.handles.group1_stock.price_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.price_label, "3234.56");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.price_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.price_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_stock.price_label, g_ui_mgr.handles.group1_stock.name_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    /* 涨跌信息合并显示 */
    g_ui_mgr.handles.group1_stock.change_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.change_label, "+12.34\n+1.23%");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.change_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.change_label, lv_color_make(255, 80, 80), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_stock.change_label, g_ui_mgr.handles.group1_stock.price_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
    
    /* 更新时间 - 底部居中，小字 */
    g_ui_mgr.handles.group1_stock.update_time_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.update_time_label, "14:30:15");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.update_time_label, &g_ui_mgr.handles.style_xsmall, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.update_time_label, lv_color_make(120, 120, 120), 0);
    lv_obj_align(g_ui_mgr.handles.group1_stock.update_time_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/*********************
 *   GROUP 2 UI BUILD
 *********************/

/**
 * 构建左屏 - CPU/GPU监控面板
 */
static void build_left_cpu_gpu_panel(lv_obj_t *parent)
{
    /* CPU标题 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_title, "CPU");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* CPU使用率 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, "35.2%");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, lv_color_make(255, 165, 0), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, g_ui_mgr.handles.group2_cpu_gpu.cpu_title, LV_ALIGN_OUT_BOTTOM_MID, 10, 5);
    
    /* CPU温度 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_temp = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, "58.5°C");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, lv_color_make(255, 100, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    
    /* GPU标题 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_title, "GPU");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_title, lv_color_make(100, 255, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.gpu_title, g_ui_mgr.handles.group2_cpu_gpu.cpu_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 45);
    
    /* GPU使用率 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, "15.8%");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, lv_color_make(0, 255, 127), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, g_ui_mgr.handles.group2_cpu_gpu.gpu_title, LV_ALIGN_OUT_BOTTOM_MID, 10, 5);
    
    /* GPU温度 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_temp = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, "42.3°C");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, lv_color_make(100, 255, 150), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
}

/**
 * 构建中屏 - 内存监控面板
 */
static void build_middle_memory_panel(lv_obj_t *parent)
{
    /* 内存标题 */
    g_ui_mgr.handles.group2_memory.ram_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_title, "内存");
    lv_obj_add_style(g_ui_mgr.handles.group2_memory.ram_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_memory.ram_title, lv_color_make(255, 165, 0), 0);
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_title, LV_ALIGN_TOP_MID, 0, 20);
    
    /* 内存使用率 - 使用更大字体，居中显示 */
    g_ui_mgr.handles.group2_memory.ram_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_usage, "0.0%");
    lv_obj_add_style(g_ui_mgr.handles.group2_memory.ram_usage, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_memory.ram_usage, lv_color_make(255, 215, 0), 0);
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_usage, LV_ALIGN_CENTER, 0, 0);
    
    /* 内存状态提示 */
    lv_obj_t *mem_hint = lv_label_create(parent);
    lv_label_set_text(mem_hint, "内存使用率");
    lv_obj_add_style(mem_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(mem_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(mem_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * 构建右屏 - 网络监控面板
 */
static void build_right_network_panel(lv_obj_t *parent)
{
    /* 网络标题 */
    g_ui_mgr.handles.group2_network.network_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.network_title, "网络");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.network_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.network_title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(g_ui_mgr.handles.group2_network.network_title, LV_ALIGN_TOP_MID, 0, 8);
    
    /* 上传速度 */
    g_ui_mgr.handles.group2_network.net_upload = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_upload, "↑ 1.25MB/s");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_upload, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_upload, lv_color_make(255, 100, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_network.net_upload, g_ui_mgr.handles.group2_network.network_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    
    /* 下载速度 */
    g_ui_mgr.handles.group2_network.net_download = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_download, "↓ 8.67MB/s");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_download, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_download, lv_color_make(100, 255, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_network.net_download, g_ui_mgr.handles.group2_network.net_upload, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    
    /* 网络状态 */
    g_ui_mgr.handles.group2_network.net_status = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_status, "在线");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_status, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_status, lv_color_make(120, 120, 120), 0);
    lv_obj_align(g_ui_mgr.handles.group2_network.net_status, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/*********************
 *   GROUP 3 UI BUILD
 *********************/

/**
 * 构建左屏 - 媒体控制面板
 */
static void build_left_media_panel(lv_obj_t *parent)
{
    /* 喇叭emoji - 上半部分居中 */
    lv_obj_t *media_emoji = lv_label_create(parent);
    lv_label_set_text(media_emoji, "♪");
    lv_obj_add_style(media_emoji, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(media_emoji, lv_color_make(255, 165, 0), 0);
    lv_obj_align(media_emoji, LV_ALIGN_CENTER, 0, -25);
    
    /* 媒体控制文字 - 下半部分居中 */
    lv_obj_t *media_text = lv_label_create(parent);
    lv_label_set_text(media_text, "媒体控制");
    lv_obj_add_style(media_text, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(media_text, lv_color_white(), 0);
    lv_obj_align(media_text, LV_ALIGN_CENTER, 0, 15);
    
    /* 功能提示 - 底部小字 */
    lv_obj_t *media_hint = lv_label_create(parent);
    lv_label_set_text(media_hint, "音量·播放");
    lv_obj_add_style(media_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(media_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(media_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * 构建中屏 - 网页控制面板
 */
static void build_middle_web_panel(lv_obj_t *parent)
{
    /* 地球emoji - 上半部分居中 */
    lv_obj_t *web_emoji = lv_label_create(parent);
    lv_label_set_text(web_emoji, "◯ ");
    lv_obj_add_style(web_emoji, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(web_emoji, lv_color_make(100, 200, 255), 0);
    lv_obj_align(web_emoji, LV_ALIGN_CENTER, 0, -25);
    
    /* 网页控制文字 - 下半部分居中 */
    lv_obj_t *web_text = lv_label_create(parent);
    lv_label_set_text(web_text, "网页控制");
    lv_obj_add_style(web_text, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(web_text, lv_color_white(), 0);
    lv_obj_align(web_text, LV_ALIGN_CENTER, 0, 15);
    
    /* 功能提示 - 底部小字 */
    lv_obj_t *web_hint = lv_label_create(parent);
    lv_label_set_text(web_hint, "翻页·刷新");
    lv_obj_add_style(web_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(web_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(web_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * 构建右屏 - 快捷键面板
 */
static void build_right_shortcut_panel(lv_obj_t *parent)
{
    /* 灯泡emoji - 上半部分居中 */
    lv_obj_t *shortcut_emoji = lv_label_create(parent);
    lv_label_set_text(shortcut_emoji, "※");
    lv_obj_add_style(shortcut_emoji, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(shortcut_emoji, lv_color_make(255, 255, 100), 0);
    lv_obj_align(shortcut_emoji, LV_ALIGN_CENTER, 0, -25);
    
    /* 快捷键文字 - 下半部分居中 */
    lv_obj_t *shortcut_text = lv_label_create(parent);
    lv_label_set_text(shortcut_text, "快捷键");
    lv_obj_add_style(shortcut_text, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(shortcut_text, lv_color_white(), 0);
    lv_obj_align(shortcut_text, LV_ALIGN_CENTER, 0, 15);
    
    /* 功能提示 - 底部小字 */
    lv_obj_t *shortcut_hint = lv_label_create(parent);
    lv_label_set_text(shortcut_hint, "复制·粘贴");
    lv_obj_add_style(shortcut_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(shortcut_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(shortcut_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/*********************
 *   L2 DIGITAL CLOCK
 *********************/

/**
 * 根据数字值获取对应的图片资源
 * @param digit 数字值（0-9）
 * @return 对应的图片描述符指针，无效输入返回&t0
 */
static const lv_image_dsc_t* get_digit_image(int digit)
{
    if (digit >= 0 && digit <= 9) {
        return digit_images[digit];
    }
    return &t0; // 默认返回数字0
}

/**
 * 创建单个数字图片对象
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
        rt_kprintf("[UIManager] Failed to create digit image\n");
        return NULL;
    }
    
    // 设置图片资源
    lv_img_set_src(img, get_digit_image(digit));
    
    /* 极致优化1: 最大化图片容器尺寸 - 占用几乎全部显示区域 */
    lv_coord_t img_width = (lv_coord_t)(SCREEN_WIDTH * 0.5f);    // 64像素，占满一半板块
    lv_coord_t img_height = (lv_coord_t)(SCREEN_HEIGHT * 1.0f);  // 128像素，占满整个高度
    lv_obj_set_size(img, img_width, img_height);
    
    // 设置位置 - 无边距，完全贴边
    lv_obj_set_pos(img, x_offset, y_offset);
    
    /* 极致优化2: 激进缩放 - 在原有基础上再放大50% */
    float max_scale = g_ui_mgr.scale_factor * 1.5f;  // 激进放大50%
    
    // 扩大缩放范围限制
    if (max_scale < 0.8f) max_scale = 0.8f;
    if (max_scale > 4.0f) max_scale = 4.0f;  // 允许更大的缩放
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * max_scale));
    
    /* 极致优化3: 最佳显示效果设置 */
    lv_img_set_antialias(img, true);                    // 抗锯齿
    lv_img_set_pivot(img, img_width/2, img_height/2);   // 居中缩放点
    
    // 移除所有内边距和边框
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    lv_obj_set_style_outline_width(img, 0, 0);
    
    rt_kprintf("[UIManager] Created MAXIMIZED digit %d: size=%dx%d, scale=%.2f\n", 
              digit, img_width, img_height, max_scale);
    
    return img;
}

/**
 * 更新单个数字图片显示
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

/**
 * 构建L2数字时钟页面 - 纯图片数字时钟显示
 */
static void build_l2_time_detail_page(void)
{
    rt_kprintf("[UIManager] Building MAXIMIZED L2 Digital Clock (extreme optimization)\n");
    
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) {
        rt_kprintf("[UIManager] Panel objects not available\n");
        return;
    }
    
    // 获取当前时间
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    int hour = tm_info ? tm_info->tm_hour : 0;
    int min = tm_info ? tm_info->tm_min : 0;
    int sec = tm_info ? tm_info->tm_sec : 0;
    
    /* 极致优化：零间距布局 - 数字完全贴边显示 */
    lv_coord_t no_spacing = 0;        // 完全无间距
    lv_coord_t no_offset = 0;         // 完全无偏移
    
    // === 左板块：小时显示 - 完全占满 ===
    g_ui_mgr.handles.l2_digital_clock.hour_tens = create_digit_image(
        g_ui_mgr.handles.left_panel, 
        hour / 10,
        no_spacing,                   // X偏移：完全贴左边
        no_offset                     // Y偏移：完全贴顶部
    );
    
    g_ui_mgr.handles.l2_digital_clock.hour_units = create_digit_image(
        g_ui_mgr.handles.left_panel,
        hour % 10,
        SCREEN_WIDTH/2,              // X偏移：右半部分，无间距
        no_offset                    // Y偏移：完全贴顶部
    );
    
    // === 中板块：分钟显示 - 完全占满 ===
    g_ui_mgr.handles.l2_digital_clock.min_tens = create_digit_image(
        g_ui_mgr.handles.middle_panel,
        min / 10,
        no_spacing,                  // X偏移：完全贴左边
        no_offset                    // Y偏移：完全贴顶部
    );
    
    g_ui_mgr.handles.l2_digital_clock.min_units = create_digit_image(
        g_ui_mgr.handles.middle_panel,
        min % 10,
        SCREEN_WIDTH/2,              // X偏移：右半部分，无间距
        no_offset                    // Y偏移：完全贴顶部
    );
    
    // === 右板块：秒钟显示 - 完全占满 ===
    g_ui_mgr.handles.l2_digital_clock.sec_tens = create_digit_image(
        g_ui_mgr.handles.right_panel,
        sec / 10,
        no_spacing,                  // X偏移：完全贴左边
        no_offset                    // Y偏移：完全贴顶部
    );
    
    g_ui_mgr.handles.l2_digital_clock.sec_units = create_digit_image(
        g_ui_mgr.handles.right_panel,
        sec % 10,
        SCREEN_WIDTH/2,              // X偏移：右半部分，无间距
        no_offset                    // Y偏移：完全贴顶部
    );
}


/**
 * 更新L2数字时钟显示 - 修复版本，添加时间变量声明
 */
static int screen_ui_update_l2_digital_clock(void)
{
    // 检查数字时钟UI对象是否存在，如果不存在说明不在时间详情L2页面
    if (!g_ui_mgr.handles.l2_digital_clock.hour_tens || 
        !lv_obj_is_valid(g_ui_mgr.handles.l2_digital_clock.hour_tens)) {
        return 0; // 不是时间详情页面，不更新数字时钟
    }
    
    // 获取当前时间 - 添加这部分缺失的代码
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        return -1; // 时间获取失败
    }
    
    struct tm *tm_info = localtime(&now);
    if (!tm_info) {
        return -1; // 时间转换失败
    }
    
    // 提取小时、分钟、秒钟
    int hour = tm_info->tm_hour;
    int min = tm_info->tm_min;
    int sec = tm_info->tm_sec;
    
    // 更新小时显示
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.hour_tens, hour / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.hour_units, hour % 10);
    
    // 更新分钟显示
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.min_tens, min / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.min_units, min % 10);
    
    // 更新秒钟显示 - 关键：确保秒钟能实时更新
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.sec_tens, sec / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.sec_units, sec % 10);
    
    // 添加调试日志，确认更新正在进行
    static int last_sec = -1;
    if (sec != last_sec) {
        rt_kprintf("[UIManager] Digital clock updated: %02d:%02d:%02d\n", hour, min, sec);
        last_sec = sec;
    }
    
    return 0;
}

/*********************
 *   OTHER L2 UI BUILD
 *********************/

/**
 * 构建媒体控制L2页面
 */
static void build_l2_media_control_page(void)
{
    rt_kprintf("[UIManager] Building L2 Media Control page\n");
    
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) return;
    
    /* 左屏：音量+ */
    lv_obj_t *left_title = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(left_title, "音量+");
    lv_obj_add_style(left_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(left_title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(left_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *vol_up_icon = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(vol_up_icon, "♪+");
    lv_obj_add_style(vol_up_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(vol_up_icon, lv_color_make(100, 255, 100), 0);
    lv_obj_align(vol_up_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *vol_up_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(vol_up_hint, "按键1增大音量");
    lv_obj_add_style(vol_up_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(vol_up_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(vol_up_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏：音量- */
    lv_obj_t *middle_title = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(middle_title, "音量-");
    lv_obj_add_style(middle_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(middle_title, lv_color_make(255, 165, 0), 0);
    lv_obj_align(middle_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *vol_down_icon = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(vol_down_icon, "♪-");
    lv_obj_add_style(vol_down_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(vol_down_icon, lv_color_make(255, 100, 100), 0);
    lv_obj_align(vol_down_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *vol_down_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(vol_down_hint, "按键2减小音量");
    lv_obj_add_style(vol_down_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(vol_down_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(vol_down_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏：播放/暂停 */
    lv_obj_t *right_title = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(right_title, "播放/暂停");
    lv_obj_add_style(right_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(right_title, lv_color_make(255, 100, 255), 0);
    lv_obj_align(right_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *play_pause_icon = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(play_pause_icon, "▶");
    lv_obj_add_style(play_pause_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(play_pause_icon, lv_color_make(255, 255, 100), 0);
    lv_obj_align(play_pause_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *play_pause_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(play_pause_hint, "按键3播放暂停");
    lv_obj_add_style(play_pause_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(play_pause_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(play_pause_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * 构建网页控制L2页面
 */
static void build_l2_web_control_page(void)
{
    rt_kprintf("[UIManager] Building L2 Web Control page\n");
    
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) return;
    
    /* 左屏：上翻页 */
    lv_obj_t *left_title = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(left_title, "上翻页");
    lv_obj_add_style(left_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(left_title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(left_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *page_up_icon = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(page_up_icon, "↑");
    lv_obj_add_style(page_up_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(page_up_icon, lv_color_make(100, 255, 255), 0);
    lv_obj_align(page_up_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *page_up_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(page_up_hint, "按键1向上翻页");
    lv_obj_add_style(page_up_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(page_up_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(page_up_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏：下翻页 */
    lv_obj_t *middle_title = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(middle_title, "下翻页");
    lv_obj_add_style(middle_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(middle_title, lv_color_make(255, 165, 0), 0);
    lv_obj_align(middle_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *page_down_icon = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(page_down_icon, "↓");
    lv_obj_add_style(page_down_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(page_down_icon, lv_color_make(100, 100, 255), 0);
    lv_obj_align(page_down_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *page_down_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(page_down_hint, "按键2向下翻页");
    lv_obj_add_style(page_down_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(page_down_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(page_down_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏：刷新 */
    lv_obj_t *right_title = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(right_title, "刷新页面");
    lv_obj_add_style(right_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(right_title, lv_color_make(255, 100, 255), 0);
    lv_obj_align(right_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *refresh_icon = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(refresh_icon, "▲");
    lv_obj_add_style(refresh_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(refresh_icon, lv_color_make(255, 100, 255), 0);
    lv_obj_align(refresh_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *refresh_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(refresh_hint, "按键3刷新F5");
    lv_obj_add_style(refresh_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(refresh_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(refresh_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * 构建快捷键控制L2页面
 */
static void build_l2_shortcut_control_page(void)
{
    rt_kprintf("[UIManager] Building L2 Shortcut Control page\n");
    
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) return;
    
    /* 左屏：复制 */
    lv_obj_t *left_title = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(left_title, "复制");
    lv_obj_add_style(left_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(left_title, lv_color_make(100, 255, 100), 0);
    lv_obj_align(left_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *copy_icon = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(copy_icon, "C");
    lv_obj_add_style(copy_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(copy_icon, lv_color_make(100, 255, 100), 0);
    lv_obj_align(copy_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *copy_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(copy_hint, "按键1复制Ctrl+C");
    lv_obj_add_style(copy_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(copy_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(copy_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏：粘贴 */
    lv_obj_t *middle_title = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(middle_title, "粘贴");
    lv_obj_add_style(middle_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(middle_title, lv_color_make(255, 255, 100), 0);
    lv_obj_align(middle_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *paste_icon = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(paste_icon, "V");
    lv_obj_add_style(paste_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(paste_icon, lv_color_make(255, 255, 100), 0);
    lv_obj_align(paste_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *paste_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(paste_hint, "按键2粘贴Ctrl+V");
    lv_obj_add_style(paste_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(paste_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(paste_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏：撤销 */
    lv_obj_t *right_title = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(right_title, "撤销");
    lv_obj_add_style(right_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(right_title, lv_color_make(255, 100, 100), 0);
    lv_obj_align(right_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *undo_icon = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(undo_icon, "↶");
    lv_obj_add_style(undo_icon, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(undo_icon, lv_color_make(255, 100, 100), 0);
    lv_obj_align(undo_icon, LV_ALIGN_CENTER, 0, -10);
    
    lv_obj_t *undo_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(undo_hint, "按键3撤销Ctrl+Z");
    lv_obj_add_style(undo_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(undo_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(undo_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/*********************
 *   PUBLIC API
 *********************/

int screen_ui_manager_init(void)
{
    if (g_ui_mgr.initialized) {
        rt_kprintf("[UIManager] Already initialized\n");
        return 0;
    }

    /* 计算缩放因子 */
    g_ui_mgr.scale_factor = get_scale_factor();
    rt_kprintf("[UIManager] Screen scale factor: %.2f\n", g_ui_mgr.scale_factor);

    /* 创建动态字体 */
    if (create_fonts() != 0) {
        rt_kprintf("[UIManager] ERROR: Failed to create fonts\n");
        return -RT_ERROR;
    }

    /* 创建基础UI结构 */
    if (create_base_ui() != 0) {
        rt_kprintf("[UIManager] ERROR: Failed to create base UI\n");
        cleanup_fonts();
        return -RT_ERROR;
    }

    g_ui_mgr.current_group = SCREEN_GROUP_1;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    g_ui_mgr.initialized = true;

    rt_kprintf("[UIManager] UI manager initialized successfully\n");
    return 0;
}

int screen_ui_manager_deinit(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }

    cleanup_base_ui();
    cleanup_fonts();
    
    memset(&g_ui_mgr, 0, sizeof(screen_ui_manager_t));
    
    rt_kprintf("[UIManager] UI manager deinitialized\n");
    return 0;
}

int screen_ui_build_group1(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_kprintf("[UIManager] Building Group 1 UI (Time/Weather/Stock)\n");
    
    safe_cleanup_ui_objects();
    
    build_left_datetime_panel(g_ui_mgr.handles.left_panel);
    build_middle_weather_panel(g_ui_mgr.handles.middle_panel);
    build_right_stock_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_1;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_1);
    
    lv_obj_invalidate(lv_scr_act());
    rt_kprintf("[UIManager] Group 1 UI built successfully\n");
    
    return 0;
}

int screen_ui_build_group2(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_kprintf("[UIManager] Building Group 2 UI (System Monitor)\n");
    
    safe_cleanup_ui_objects();
    
    build_left_cpu_gpu_panel(g_ui_mgr.handles.left_panel);
    build_middle_memory_panel(g_ui_mgr.handles.middle_panel);
    build_right_network_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_2;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_2);
    
    lv_obj_invalidate(lv_scr_act());
    rt_kprintf("[UIManager] Group 2 UI built successfully\n");
    
    return 0;
}

int screen_ui_build_group3(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_kprintf("[UIManager] Building Group 3 UI (HID Control)\n");
    
    safe_cleanup_ui_objects();
    
    build_left_media_panel(g_ui_mgr.handles.left_panel);
    build_middle_web_panel(g_ui_mgr.handles.middle_panel);
    build_right_shortcut_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_3;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_3);
    
    lv_obj_invalidate(lv_scr_act());
    rt_kprintf("[UIManager] Group 3 UI built successfully\n");
    
    return 0;
}

int screen_ui_build_l2_time(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_kprintf("[UIManager] Building L2 Digital Time Display\n");
    
    safe_cleanup_ui_objects();
    build_l2_time_detail_page();  // 使用新的数字时钟页面
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_TIME_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
    rt_kprintf("[UIManager] L2 Digital Time Display built successfully\n");
    return 0;
}

int screen_ui_build_l2_media(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    safe_cleanup_ui_objects();
    build_l2_media_control_page();
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_MEDIA_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
    return 0;
}

int screen_ui_build_l2_web(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    safe_cleanup_ui_objects();
    build_l2_web_control_page();
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_WEB_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
    return 0;
}

int screen_ui_build_l2_shortcut(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    safe_cleanup_ui_objects();
    build_l2_shortcut_control_page();
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_SHORTCUT_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
    return 0;
}

int screen_ui_switch_to_group(screen_group_t target_group)
{
        rt_kprintf("[UIManager] DEBUG: Switching to group %d (MAX=%d)\n", 
               target_group, SCREEN_GROUP_MAX);
    if (!g_ui_mgr.initialized || target_group >= SCREEN_GROUP_MAX) {
        return -RT_EINVAL;
    }

    switch (target_group) {
        case SCREEN_GROUP_1:
            return screen_ui_build_group1();
        case SCREEN_GROUP_2:
            return screen_ui_build_group2();
        case SCREEN_GROUP_3:
            return screen_ui_build_group3();
        case SCREEN_GROUP_4:
            return screen_ui_build_group4();
        default:
            rt_kprintf("[UIManager] Invalid target group: %d\n", target_group);
            return -RT_EINVAL;
    }
}

int screen_ui_switch_to_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    switch (l2_group) {
        case SCREEN_L2_TIME_GROUP:
            return screen_ui_build_l2_time();
        case SCREEN_L2_MEDIA_GROUP:
            return screen_ui_build_l2_media();
        case SCREEN_L2_WEB_GROUP:
            return screen_ui_build_l2_web();
        case SCREEN_L2_SHORTCUT_GROUP:
            return screen_ui_build_l2_shortcut();
        case SCREEN_L2_MUYU_GROUP:
            return screen_ui_build_l2_muyu();
        case SCREEN_L2_TOMATO_GROUP:
            return screen_ui_build_l2_tomato();
        case SCREEN_L2_GALLERY_GROUP:
            return screen_ui_build_l2_gallery();
        default:
            rt_kprintf("[UIManager] Invalid L2 group: %d\n", l2_group);
            return -RT_EINVAL;
    }
}

int screen_ui_return_to_l1(screen_group_t l1_group)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    return screen_ui_switch_to_group(l1_group);
}

/*********************
 *   UI UPDATE API
 *********************/

int screen_ui_update_time_display(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    
    // 优先检查是否在L2时间详情页面
    // 简化检查：通过UI对象是否存在来判断
    if (g_ui_mgr.handles.l2_digital_clock.hour_tens && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_digital_clock.hour_tens)) {
        // 数字时钟UI存在，更新数字时钟
        return screen_ui_update_l2_digital_clock();
    }
    
    // L1层级的常规时间显示更新
    if (g_ui_mgr.current_group != SCREEN_GROUP_1) {
        return 0;
    }
    
    time_t now = time(NULL);
    if (now == (time_t)-1) return -1;
    
    struct tm *tm_info = localtime(&now);
    if (!tm_info) return -1;

    /* 更新年份 */
    if (g_ui_mgr.handles.group1_time.year_label && lv_obj_is_valid(g_ui_mgr.handles.group1_time.year_label)) {
        char year_str[16];
        rt_snprintf(year_str, sizeof(year_str), "%d年", tm_info->tm_year + 1900);
        lv_label_set_text(g_ui_mgr.handles.group1_time.year_label, year_str);
    }

    /* 更新时间 */
    if (g_ui_mgr.handles.group1_time.time_label && lv_obj_is_valid(g_ui_mgr.handles.group1_time.time_label)) {
        char time_str[16];
        rt_snprintf(time_str, sizeof(time_str), "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
        lv_label_set_text(g_ui_mgr.handles.group1_time.time_label, time_str);
    }

    /* 更新中文日期 */
    if (g_ui_mgr.handles.group1_time.date_label && lv_obj_is_valid(g_ui_mgr.handles.group1_time.date_label)) {
        char date_str[32];
        rt_snprintf(date_str, sizeof(date_str), "%s%d日", 
                   chinese_months[tm_info->tm_mon], tm_info->tm_mday);
        lv_label_set_text(g_ui_mgr.handles.group1_time.date_label, date_str);
    }

    /* 更新中文星期 */
    if (g_ui_mgr.handles.group1_time.weekday_label && lv_obj_is_valid(g_ui_mgr.handles.group1_time.weekday_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_time.weekday_label, chinese_weekdays[tm_info->tm_wday]);
    }

    return 0;
}

int screen_ui_update_weather_display(const weather_data_t *data)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.current_group != SCREEN_GROUP_1 || !data || !data->valid) {
        return 0;
    }

    /* 更新城市名 */
    if (g_ui_mgr.handles.group1_weather.city_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.city_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.city_label, data->city);
    }

    /* 更新温度 */
    if (g_ui_mgr.handles.group1_weather.temperature_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.temperature_label)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->temperature);
        lv_label_set_text(g_ui_mgr.handles.group1_weather.temperature_label, temp_str);
    }

    /* 更新天气描述 */
    if (g_ui_mgr.handles.group1_weather.weather_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.weather_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.weather_label, data->weather);
    }

    /* 更新湿度 */
    if (g_ui_mgr.handles.group1_weather.humidity_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.humidity_label)) {
        char humidity_str[16];
        rt_snprintf(humidity_str, sizeof(humidity_str), "湿度: %.0f%%", data->humidity);
        lv_label_set_text(g_ui_mgr.handles.group1_weather.humidity_label, humidity_str);
    }

    /* 更新气压 */
    if (g_ui_mgr.handles.group1_weather.pressure_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.pressure_label)) {
        char pressure_str[16];
        rt_snprintf(pressure_str, sizeof(pressure_str), "%dhPa", data->pressure);
        lv_label_set_text(g_ui_mgr.handles.group1_weather.pressure_label, pressure_str);
    }

    return 0;
}

int screen_ui_update_stock_display(const stock_data_t *data)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.current_group != SCREEN_GROUP_1 || !data || !data->valid) {
        return 0;
    }

    /* 更新股票名称 */
    if (g_ui_mgr.handles.group1_stock.name_label && lv_obj_is_valid(g_ui_mgr.handles.group1_stock.name_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_stock.name_label, data->name);
    }

    /* 更新价格 */
    if (g_ui_mgr.handles.group1_stock.price_label && lv_obj_is_valid(g_ui_mgr.handles.group1_stock.price_label)) {
        char price_str[16];
        rt_snprintf(price_str, sizeof(price_str), "%.2f", data->current_price);
        lv_label_set_text(g_ui_mgr.handles.group1_stock.price_label, price_str);
    }

    /* 更新涨跌幅 */
    if (g_ui_mgr.handles.group1_stock.change_label && lv_obj_is_valid(g_ui_mgr.handles.group1_stock.change_label)) {
        char change_str[32];
        rt_snprintf(change_str, sizeof(change_str), "%+.2f\n%+.2f%%", 
                   data->change_value, data->change_percent);
        lv_label_set_text(g_ui_mgr.handles.group1_stock.change_label, change_str);
        
        /* 根据涨跌设置颜色 */
        lv_color_t color = (data->change_value >= 0) ? 
                          lv_color_make(255, 80, 80) :   /* 红色上涨 */
                          lv_color_make(80, 255, 80);    /* 绿色下跌 */
        lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.change_label, color, 0);
    }

    /* 更新时间 */
    if (g_ui_mgr.handles.group1_stock.update_time_label && lv_obj_is_valid(g_ui_mgr.handles.group1_stock.update_time_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_stock.update_time_label, data->update_time);
    }

    return 0;
}

int screen_ui_update_system_display(const system_monitor_data_t *data)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.current_group != SCREEN_GROUP_2 || !data || !data->valid) {
        return 0;
    }

    /* 更新CPU使用率 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage)) {
        char usage_str[16];
        rt_snprintf(usage_str, sizeof(usage_str), "%.1f%%", data->cpu_usage);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, usage_str);
    }

    /* 更新CPU温度 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_temp && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->cpu_temp);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, temp_str);
    }

    /* 更新GPU使用率 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage)) {
        char usage_str[16];
        rt_snprintf(usage_str, sizeof(usage_str), "%.1f%%", data->gpu_usage);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, usage_str);
    }

    /* 更新GPU温度 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_temp && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->gpu_temp);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, temp_str);
    }

    /* 更新内存信息 */
    if (g_ui_mgr.handles.group2_memory.ram_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_memory.ram_usage)) {
        char usage_str[16];
        rt_snprintf(usage_str, sizeof(usage_str), "%.1f%%", data->ram_usage);
        lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_usage, usage_str);
    }

    /* 更新网络信息 */
    if (g_ui_mgr.handles.group2_network.net_upload && lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_upload)) {
        char speed_str[32];
        rt_snprintf(speed_str, sizeof(speed_str), "↑ %.2fMB/s", data->net_upload_speed);
        lv_label_set_text(g_ui_mgr.handles.group2_network.net_upload, speed_str);
    }

    if (g_ui_mgr.handles.group2_network.net_download && lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_download)) {
        char speed_str[32];
        rt_snprintf(speed_str, sizeof(speed_str), "↓ %.2fMB/s", data->net_download_speed);
        lv_label_set_text(g_ui_mgr.handles.group2_network.net_download, speed_str);
    }

    /* 更新网络状态 */
    if (g_ui_mgr.handles.group2_network.net_status && lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_status)) {
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                char time_str[32];
                rt_snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
                           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
                lv_label_set_text(g_ui_mgr.handles.group2_network.net_status, time_str);
            }
        }
    }

    return 0;
}

int screen_ui_update_sensor_display(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.current_group != SCREEN_GROUP_1) {
        return 0;
    }

    if (!g_ui_mgr.handles.group1_weather.sensor_label || !lv_obj_is_valid(g_ui_mgr.handles.group1_weather.sensor_label)) {
        return 0;
    }

    sht30_data_t data = {0};
    if (sht30_controller_get_latest(&data) == RT_EOK && data.valid) {
        rt_tick_t now = rt_tick_get();
        if ((now - data.timestamp) <= rt_tick_from_millisecond(20000)) {
            char sensor_str[32];
            rt_snprintf(sensor_str, sizeof(sensor_str), "传感器: %.1f°C %.0f%%",
                      data.temperature_c, data.humidity_rh);
            lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, sensor_str);
        } else {
            lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "传感器: --°C --%");
        }
    } else {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "传感器: --°C --%");
    }

    return 0;
}

int screen_ui_cleanup_current_group(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }

    safe_cleanup_ui_objects();
    return 0;
}

int screen_ui_cleanup_all(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }

    cleanup_base_ui();
    return 0;
}

screen_group_t screen_ui_get_current_group(void)
{
    return g_ui_mgr.current_group;
}

bool screen_ui_is_initialized(void)
{
    return g_ui_mgr.initialized;
}


/* 需要添加到screen_ui_manager.c文件中的新代码部分 */

/* 木鱼图片资源声明 - 需要添加到文件顶部 */
extern const lv_image_dsc_t muyu;  // 木鱼图片资源

/*********************
 *   GROUP 4 UI BUILD - 新增实用工具页面
 *********************/

/**
 * 创建缩小的木鱼图片用于入口显示
 */
static lv_obj_t* create_muyu_entrance_icon(lv_obj_t *parent)
{
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
        rt_kprintf("[UIManager] Failed to create muyu entrance icon\n");
        return NULL;
    }
    
    // 设置木鱼图片资源
    lv_image_set_src(img, get_muyu_image());
    
    /* 入口图标使用较小尺寸 - 缩小50% */
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 0.25f);  // 从50%减小到25%
    lv_obj_set_size(img, icon_size, icon_size);
    
    // 适中的缩放比例 - 缩小50%
    float scale = g_ui_mgr.scale_factor * 0.4f;  // 从0.8f减小到0.4f
    if (scale < 0.3f) scale = 0.3f;
    if (scale > 1.0f) scale = 1.0f;  // 最大缩放限制
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    // 移除所有边距和边框
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    
    rt_kprintf("[UIManager] Created muyu entrance icon: size=%dx%d, scale=%.2f (reduced 50%%)\n", 
              icon_size, icon_size, scale);
    
    return img;
}

/**
 * 构建左屏 - 赛博木鱼面板
 */
static void build_left_muyu_panel(lv_obj_t *parent)
{
    /* 木鱼图片入口 - 上半部分居中 */
    g_ui_mgr.handles.group4_muyu.muyu_icon = create_muyu_entrance_icon(parent);
    if (g_ui_mgr.handles.group4_muyu.muyu_icon) {
        lv_obj_align(g_ui_mgr.handles.group4_muyu.muyu_icon, LV_ALIGN_CENTER, 0, -20);
    }
    
    /* 赛博木鱼文字 - 下半部分居中 */
    g_ui_mgr.handles.group4_muyu.muyu_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_muyu.muyu_title, "赛博木鱼");
    lv_obj_add_style(g_ui_mgr.handles.group4_muyu.muyu_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_muyu.muyu_title, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group4_muyu.muyu_title, LV_ALIGN_CENTER, 0, 25);
    
    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_muyu.muyu_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_muyu.muyu_hint, "积累·功德");
    lv_obj_add_style(g_ui_mgr.handles.group4_muyu.muyu_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_muyu.muyu_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_muyu.muyu_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * 构建中屏 - 番茄钟面板
 */
static void build_middle_tomato_panel(lv_obj_t *parent)
{
    /* 番茄emoji - 上半部分居中 */
    g_ui_mgr.handles.group4_tomato.tomato_icon = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_tomato.tomato_icon, "🍅");
    lv_obj_add_style(g_ui_mgr.handles.group4_tomato.tomato_icon, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_tomato.tomato_icon, lv_color_make(255, 99, 71), 0);
    lv_obj_align(g_ui_mgr.handles.group4_tomato.tomato_icon, LV_ALIGN_CENTER, 0, -25);
    
    /* 番茄钟文字 - 下半部分居中 */
    g_ui_mgr.handles.group4_tomato.tomato_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_tomato.tomato_title, "番茄钟");
    lv_obj_add_style(g_ui_mgr.handles.group4_tomato.tomato_title, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_tomato.tomato_title, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group4_tomato.tomato_title, LV_ALIGN_CENTER, 0, 15);
    
    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_tomato.tomato_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_tomato.tomato_hint, "专注·高效");
    lv_obj_add_style(g_ui_mgr.handles.group4_tomato.tomato_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_tomato.tomato_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_tomato.tomato_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * 构建右屏 - 全屏图片面板
 */
static void build_right_gallery_panel(lv_obj_t *parent)
{
    /* 图片emoji - 上半部分居中 */
    g_ui_mgr.handles.group4_gallery.gallery_icon = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_gallery.gallery_icon, "🖼️");
    lv_obj_add_style(g_ui_mgr.handles.group4_gallery.gallery_icon, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_gallery.gallery_icon, lv_color_make(144, 238, 144), 0);
    lv_obj_align(g_ui_mgr.handles.group4_gallery.gallery_icon, LV_ALIGN_CENTER, 0, -25);
    
    /* 全屏图片文字 - 下半部分居中 */
    g_ui_mgr.handles.group4_gallery.gallery_title = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_gallery.gallery_title, "全屏图片");
    lv_obj_add_style(g_ui_mgr.handles.group4_gallery.gallery_title, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_gallery.gallery_title, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group4_gallery.gallery_title, LV_ALIGN_CENTER, 0, 15);
    
    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_gallery.gallery_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_gallery.gallery_hint, "欣赏·展示");
    lv_obj_add_style(g_ui_mgr.handles.group4_gallery.gallery_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_gallery.gallery_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_gallery.gallery_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * 获取木鱼图片资源
 */
static const lv_image_dsc_t* get_muyu_image(void)
{
    return &muyu;
}



/**
 * 创建L2木鱼主界面的展示图片（非点击）
 */
static lv_obj_t* create_muyu_display_image(lv_obj_t *parent)
{
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
        rt_kprintf("[UIManager] Failed to create muyu display image\n");
        return NULL;
    }
    
    // 设置木鱼图片资源
    lv_img_set_src(img, get_muyu_image());
    
    /* 复用入口图标的处理方式 - 合适的尺寸和缩放 */
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 0.25f);  // 使用和入口一样的25%
    lv_obj_set_size(img, icon_size, icon_size);
    
    // 设置位置 - 居中显示
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    // 适中的缩放比例 - 和入口图标一样
    float scale = g_ui_mgr.scale_factor * 0.4f;  // 使用和入口一样的0.4f
    if (scale < 0.3f) scale = 0.3f;
    if (scale > 1.0f) scale = 1.0f;  // 最大缩放限制
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    // 移除所有边距和边框
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    
    // 设置为非点击 - 纯展示用途
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);  // 透明背景
    
    rt_kprintf("[UIManager] Created muyu display image using entrance icon style: size=%dx%d, scale=%.2f\n", 
              icon_size, icon_size, scale);
    
    return img;
}

/**
 * 构建L2赛博木鱼主界面
 */
static void build_l2_muyu_main_page(void)
{
    rt_kprintf("[UIManager] Building L2 Cyber Muyu Main Page (key-based interaction)\n");
    
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) {
        rt_kprintf("[UIManager] Panel objects not available\n");
        return;
    }
    
    // === 左板块：木鱼图片（纯展示，KEY1触发） ===
    g_ui_mgr.handles.l2_muyu_main.muyu_image = create_muyu_display_image(g_ui_mgr.handles.left_panel);
    
    // 添加按键提示文字
    lv_obj_t *key_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(key_hint, "按键1敲击");
    lv_obj_add_style(key_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(key_hint, lv_color_make(255, 215, 0), 0);
    lv_obj_align(key_hint, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // === 中板块：计数器显示 ===
    // 当前计数标题
    lv_obj_t *counter_title = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(counter_title, "功德");
    lv_obj_add_style(counter_title, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(counter_title, lv_color_make(255, 215, 0), 0);
    lv_obj_align(counter_title, LV_ALIGN_TOP_MID, 0, 15);
    
    // 当前计数显示
    g_ui_mgr.handles.l2_muyu_main.counter_label = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.counter_label, "0");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.counter_label, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.counter_label, lv_color_make(255, 215, 0), 0);
    lv_obj_align(g_ui_mgr.handles.l2_muyu_main.counter_label, LV_ALIGN_CENTER, 0, -10);
    
    // 当前会话提示
    lv_obj_t *session_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(session_hint, "本次");
    lv_obj_add_style(session_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(session_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(session_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // === 右板块：总计数和功德信息 ===
    // 总计数标题
    lv_obj_t *total_title = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(total_title, "总计");
    lv_obj_add_style(total_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(total_title, lv_color_make(100, 200, 255), 0);
    lv_obj_align(total_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 总计数显示
    g_ui_mgr.handles.l2_muyu_main.total_label = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.total_label, "0");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.total_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.total_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.l2_muyu_main.total_label, total_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    // 功德等级显示
    g_ui_mgr.handles.l2_muyu_main.merit_label = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.merit_label, "初心功德");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.merit_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.merit_label, lv_color_make(144, 238, 144), 0);
    lv_obj_align(g_ui_mgr.handles.l2_muyu_main.merit_label, LV_ALIGN_CENTER, 0, 10);
    
    // 重置提示
    g_ui_mgr.handles.l2_muyu_main.reset_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.reset_hint, "按键2重置");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.reset_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.reset_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(g_ui_mgr.handles.l2_muyu_main.reset_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // 初始化木鱼数据
    memset(&g_ui_mgr.muyu_data, 0, sizeof(muyu_data_t));
    g_ui_mgr.muyu_data.sound_enabled = true;
    g_ui_mgr.muyu_data.auto_save = true;
    g_ui_mgr.muyu_data.tap_effect_level = 1;
}

/* 需要添加到公共API部分的函数 */

int screen_ui_build_group4(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_kprintf("[UIManager] Building Group 4 UI (Utility Tools)\n");
    
    safe_cleanup_ui_objects();
    
    build_left_muyu_panel(g_ui_mgr.handles.left_panel);
    build_middle_tomato_panel(g_ui_mgr.handles.middle_panel);
    build_right_gallery_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_4;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_4);
    
    lv_obj_invalidate(lv_scr_act());
    rt_kprintf("[UIManager] Group 4 UI built successfully\n");
    
    return 0;
}

int screen_ui_build_l2_muyu(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    rt_kprintf("[UIManager] Building L2 Cyber Muyu Main Page\n");
    
    safe_cleanup_ui_objects();
    build_l2_muyu_main_page();
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_MUYU_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
    rt_kprintf("[UIManager] L2 Cyber Muyu built successfully\n");
    return 0;
}

int screen_ui_build_l2_tomato(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    // 预留番茄钟实现
    rt_kprintf("[UIManager] L2 Tomato Timer - Coming Soon\n");
    
    safe_cleanup_ui_objects();
    
    // 临时显示"开发中"界面
    lv_obj_t *temp_label = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(temp_label, "番茄钟\n开发中...");
    lv_obj_add_style(temp_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 0);
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    lv_obj_invalidate(lv_scr_act());
    return 0;
}

int screen_ui_build_l2_gallery(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    // 预留全屏图片实现
    rt_kprintf("[UIManager] L2 Gallery View - Coming Soon\n");
    
    safe_cleanup_ui_objects();
    
    // 临时显示"开发中"界面
    lv_obj_t *temp_label = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(temp_label, "全屏图片\n开发中...");
    lv_obj_add_style(temp_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 0);
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    lv_obj_invalidate(lv_scr_act());
    return 0;
}

int screen_ui_update_muyu_display(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    
    // 检查是否在木鱼L2页面
    if (!g_ui_mgr.handles.l2_muyu_main.counter_label || 
        !lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.counter_label)) {
        return 0; // 不在木鱼页面，不更新
    }
    
    // 更新当前计数显示
    char counter_str[16];
    rt_snprintf(counter_str, sizeof(counter_str), "%u", g_ui_mgr.muyu_data.tap_count);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.counter_label, counter_str);
    
    // 更新总计数显示
    if (g_ui_mgr.handles.l2_muyu_main.total_label && lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.total_label)) {
        char total_str[16];
        rt_snprintf(total_str, sizeof(total_str), "%u", g_ui_mgr.muyu_data.total_taps);
        lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.total_label, total_str);
    }
    
    // 更新功德等级显示
    if (g_ui_mgr.handles.l2_muyu_main.merit_label && lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.merit_label)) {
        const char *merit_text;
        if (g_ui_mgr.muyu_data.total_taps < 100) {
            merit_text = "初心功德";
        } else if (g_ui_mgr.muyu_data.total_taps < 1000) {
            merit_text = "精进功德";
        } else {
            merit_text = "圆满功德";
        }
        lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.merit_label, merit_text);
    }
    
    return 0;
}

int screen_ui_muyu_tap_event(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    // 计数器+1
    g_ui_mgr.muyu_data.tap_count++;
    g_ui_mgr.muyu_data.total_taps++;
    g_ui_mgr.muyu_data.session_taps++;
    
    // 更新最后敲击时间
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            rt_snprintf(g_ui_mgr.muyu_data.last_tap_time, 
                       sizeof(g_ui_mgr.muyu_data.last_tap_time),
                       "%02d:%02d:%02d", 
                       tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
    }
    
    // 更新显示
    screen_ui_update_muyu_display();
    
    rt_kprintf("[UIManager] Muyu tap event: count=%u, total=%u\n", 
              g_ui_mgr.muyu_data.tap_count, g_ui_mgr.muyu_data.total_taps);
    
    return 0;
}

const muyu_data_t* screen_ui_get_muyu_data(void)
{
    return &g_ui_mgr.muyu_data;
}

int screen_ui_reset_muyu_counter(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    // 重置当前计数器，但保留总计数
    g_ui_mgr.muyu_data.tap_count = 0;
    
    // 更新显示
    screen_ui_update_muyu_display();
    
    rt_kprintf("[UIManager] Muyu counter reset. Total preserved: %u\n", 
              g_ui_mgr.muyu_data.total_taps);
    
    return 0;
}

