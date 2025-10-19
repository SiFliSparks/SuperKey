/**
 * @file screen_ui_manager.c - 修复编译错误版本
 * @brief 线程安全的UI管理器实现 - 修复g_core访问问题
 */

#include "screen_ui_manager.h"
#include "screen_core.h"
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
// 天气图标资源声明
extern const lv_image_dsc_t w100;  // 晴
extern const lv_image_dsc_t w101;  // 多云
extern const lv_image_dsc_t w102;  // 少云
extern const lv_image_dsc_t w103;  // 晴间多云
extern const lv_image_dsc_t w104;  // 阴
extern const lv_image_dsc_t w150;  // 晴（夜间）
extern const lv_image_dsc_t w151;  // 多云（夜间）
extern const lv_image_dsc_t w152;  // 少云（夜间）
extern const lv_image_dsc_t w153;  // 晴间多云（夜间）
extern const lv_image_dsc_t w300;  // 阵雨
extern const lv_image_dsc_t w301;  // 强阵雨
extern const lv_image_dsc_t w302;  // 雷阵雨
extern const lv_image_dsc_t w303;  // 强雷阵雨
extern const lv_image_dsc_t w304;  // 雷阵雨伴有冰雹
extern const lv_image_dsc_t w305;  // 小雨
extern const lv_image_dsc_t w306;  // 中雨
extern const lv_image_dsc_t w307;  // 大雨
extern const lv_image_dsc_t w308;  // 极端降雨
extern const lv_image_dsc_t w309;  // 毛毛雨/细雨
extern const lv_image_dsc_t w310;  // 暴雨
extern const lv_image_dsc_t w311;  // 大暴雨
extern const lv_image_dsc_t w312;  // 特大暴雨
extern const lv_image_dsc_t w313;  // 冻雨
extern const lv_image_dsc_t w314;  // 小到中雨
extern const lv_image_dsc_t w315;  // 中到大雨
extern const lv_image_dsc_t w316;  // 大到暴雨
extern const lv_image_dsc_t w317;  // 暴雨到大暴雨
extern const lv_image_dsc_t w318;  // 大暴雨到特大暴雨
extern const lv_image_dsc_t w350;  // 阵雨（夜间）
extern const lv_image_dsc_t w351;  // 强阵雨（夜间）
extern const lv_image_dsc_t w399;  // 雨
extern const lv_image_dsc_t w400;  // 小雪
extern const lv_image_dsc_t w401;  // 中雪
extern const lv_image_dsc_t w402;  // 大雪
extern const lv_image_dsc_t w403;  // 暴雪
extern const lv_image_dsc_t w404;  // 雨夹雪
extern const lv_image_dsc_t w405;  // 雨雪天气
extern const lv_image_dsc_t w406;  // 阵雨夹雪
extern const lv_image_dsc_t w407;  // 阵雪
extern const lv_image_dsc_t w408;  // 小到中雪
extern const lv_image_dsc_t w409;  // 中到大雪
extern const lv_image_dsc_t w410;  // 大到暴雪
extern const lv_image_dsc_t w456;  // 阵雨夹雪（夜间）
extern const lv_image_dsc_t w457;  // 阵雪（夜间）
extern const lv_image_dsc_t w499;  // 雪
extern const lv_image_dsc_t w500;  // 薄雾
extern const lv_image_dsc_t w501;  // 雾
extern const lv_image_dsc_t w502;  // 霾
extern const lv_image_dsc_t w503;  // 扬沙
extern const lv_image_dsc_t w504;  // 浮尘
extern const lv_image_dsc_t w507;  // 沙尘暴
extern const lv_image_dsc_t w508;  // 强沙尘暴
extern const lv_image_dsc_t w509;  // 浓雾
extern const lv_image_dsc_t w510;  // 强浓雾
extern const lv_image_dsc_t w511;  // 中度霾
extern const lv_image_dsc_t w512;  // 重度霾
extern const lv_image_dsc_t w513;  // 严重霾
extern const lv_image_dsc_t w514;  // 大雾
extern const lv_image_dsc_t w515;  // 特强浓雾
extern const lv_image_dsc_t w900;  // 热
extern const lv_image_dsc_t w901;  // 冷
extern const lv_image_dsc_t w999;  // 未知

/* 天气图标映射数组 - 根据weather_code获取对应图标 */
static const lv_image_dsc_t* weather_icon_map[] = {
    [100] = &w100, [101] = &w101, [102] = &w102, [103] = &w103, [104] = &w104,
    [150] = &w150, [151] = &w151, [152] = &w152, [153] = &w153,
    [300] = &w300, [301] = &w301, [302] = &w302, [303] = &w303, [304] = &w304,
    [305] = &w305, [306] = &w306, [307] = &w307, [308] = &w308, [309] = &w309,
    [310] = &w310, [311] = &w311, [312] = &w312, [313] = &w313, [314] = &w314,
    [315] = &w315, [316] = &w316, [317] = &w317, [318] = &w318,
    [350] = &w350, [351] = &w351, [399] = &w399,
    [400] = &w400, [401] = &w401, [402] = &w402, [403] = &w403, [404] = &w404,
    [405] = &w405, [406] = &w406, [407] = &w407, [408] = &w408, [409] = &w409,
    [410] = &w410, [456] = &w456, [457] = &w457, [499] = &w499,
    [500] = &w500, [501] = &w501, [502] = &w502, [503] = &w503, [504] = &w504,
    [507] = &w507, [508] = &w508, [509] = &w509, [510] = &w510, [511] = &w511,
    [512] = &w512, [513] = &w513, [514] = &w514, [515] = &w515,
    [900] = &w900, [901] = &w901, [999] = &w999
};
#define WEATHER_ICON_MAP_MAX 1000

extern const lv_image_dsc_t media;   // 媒体控制图片
extern const lv_image_dsc_t web;     // 网页控制图片  
extern const lv_image_dsc_t shortcut; // 快捷键图片
extern const lv_image_dsc_t muyu;  // 木鱼图片资源
extern const lv_image_dsc_t tomatolock;     // 番茄钟图片资源
extern const lv_image_dsc_t calculagraph;   // 计时器图片资源
extern const lv_image_dsc_t volup;      // 音量+图片
extern const lv_image_dsc_t voldown;    // 音量-图片
extern const lv_image_dsc_t play;       // 播放/暂停图片
extern const lv_image_dsc_t ctrlc;      // 复制图片
extern const lv_image_dsc_t ctrlv;      // 粘贴图片
extern const lv_image_dsc_t ctrlz;      // 撤销图片
extern const lv_image_dsc_t up;         // 上翻页图片
extern const lv_image_dsc_t down;       // 下翻页图片
extern const lv_image_dsc_t fresh;      // 刷新图片
// CPU 和 GPU 图标声明
extern const lv_image_dsc_t cpuicon;  // CPU图标
extern const lv_image_dsc_t gpuicon;  // GPU图标
extern const lv_image_dsc_t memicon;  // 内存图标
/*********************
 *  STATIC VARIABLES
 *********************/
static screen_ui_manager_t g_ui_mgr = {0};
static struct {
    lv_coord_t cpu_history[15];
    lv_coord_t gpu_history[15];
    lv_coord_t mem_history[5];
    uint8_t cpu_index;
    uint8_t gpu_index;
} chart_history = {0};
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

/* 图片资源获取函数 */
static const lv_image_dsc_t* get_muyu_image(void);
static const lv_image_dsc_t* get_tomato_image(void);
static const lv_image_dsc_t* get_calculagraph_image(void);
static const lv_image_dsc_t* get_media_image(void); 
static const lv_image_dsc_t* get_web_image(void);
static const lv_image_dsc_t* get_shortcut_image(void);
static const lv_image_dsc_t* get_volup_image(void);
static const lv_image_dsc_t* get_voldown_image(void);
static const lv_image_dsc_t* get_play_image(void);
static const lv_image_dsc_t* get_ctrlc_image(void);
static const lv_image_dsc_t* get_ctrlv_image(void);
static const lv_image_dsc_t* get_ctrlz_image(void);
static const lv_image_dsc_t* get_up_image(void);
static const lv_image_dsc_t* get_down_image(void);
static const lv_image_dsc_t* get_fresh_image(void);
static const lv_image_dsc_t* get_cpu_icon(void);
static const lv_image_dsc_t* get_gpu_icon(void);
static const lv_image_dsc_t* get_mem_icon(void);
/* 通用图标和全尺寸图标创建函数 */
static lv_obj_t* create_entrance_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src);
static lv_obj_t* create_fullsize_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src);
/* 图表创建函数 */
static lv_obj_t* create_usage_chart(lv_obj_t *parent, lv_color_t color);
static void update_usage_chart(lv_obj_t *chart, float value, lv_color_t color);
static lv_obj_t* create_memory_chart(lv_obj_t *parent, lv_color_t color); 
/* L2 UI构建 - 数字时钟相关 */
static const lv_image_dsc_t* get_digit_image(int digit);
static lv_obj_t* create_digit_image(lv_obj_t *parent, int digit, lv_coord_t x_offset, lv_coord_t y_offset);
static void update_digit_image(lv_obj_t *img_obj, int digit);
static void build_l2_time_detail_page(void);
static int screen_ui_update_l2_digital_clock(void);

static void build_l2_media_control_page(void);
static void build_l2_web_control_page(void);
static void build_l2_shortcut_control_page(void);

/* 安全清理 */
static void safe_cleanup_ui_objects(void);

/**
 * 根据天气代码获取对应的天气图标
 * @param weather_code 天气代码（0-999）
 * @return 对应的图标资源指针，无效代码返回未知图标
 */
static const lv_image_dsc_t* get_weather_icon_by_code(int weather_code)
{
    if (weather_code >= 0 && weather_code < WEATHER_ICON_MAP_MAX && 
        weather_icon_map[weather_code] != NULL) {
        return weather_icon_map[weather_code];
    }
    return &w999; // 默认返回未知天气图标
}

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
        return -RT_ERROR;
    }

    /* 设置背景为纯黑色 */
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* 创建根容器 */
    g_ui_mgr.handles.root = lv_obj_create(scr);
    if (!g_ui_mgr.handles.root) {
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
    lv_label_set_text(g_ui_mgr.handles.group1_weather.city_label, "未知");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.city_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.city_label, lv_color_make(100, 200, 255), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    /* ⭐ 天气描述 - 移到左边，城市名下方，左对齐 */
    g_ui_mgr.handles.group1_weather.weather_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.weather_label, "未知");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.weather_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.weather_label, lv_color_make(255, 220, 100), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.weather_label, g_ui_mgr.handles.group1_weather.city_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* ⭐ 温度 - 移到右边，顶部右对齐 */
    g_ui_mgr.handles.group1_weather.temperature_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.temperature_label, "--°C");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.temperature_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.temperature_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group1_weather.temperature_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* ⭐ 天气图标 - 在温度下方，靠右边缘 */
    g_ui_mgr.handles.group1_weather.weather_icon = lv_img_create(parent);
    lv_img_set_src(g_ui_mgr.handles.group1_weather.weather_icon, &w999); // 默认显示未知图标
    
    // 设置图标大小和缩放
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 1.0f);  // 图标占屏幕宽度100%
    lv_obj_set_size(g_ui_mgr.handles.group1_weather.weather_icon, icon_size, icon_size);
    
    float scale = g_ui_mgr.scale_factor * 0.4f;  // 适中的缩放比例
    
    lv_img_set_zoom(g_ui_mgr.handles.group1_weather.weather_icon, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(g_ui_mgr.handles.group1_weather.weather_icon, true);
    
    // 移除所有边距和边框并允许溢出显示
    lv_obj_set_style_pad_all(g_ui_mgr.handles.group1_weather.weather_icon, 0, 0);
    lv_obj_set_style_border_width(g_ui_mgr.handles.group1_weather.weather_icon, 0, 0);
    lv_obj_add_flag(g_ui_mgr.handles.group1_weather.weather_icon, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    // 位置：在温度下方，靠右边缘
    lv_obj_align(g_ui_mgr.handles.group1_weather.weather_icon, 
                LV_ALIGN_TOP_RIGHT, 
                35,
                10);  

    /* 湿度 - 天气描述下方，左对齐 */
    g_ui_mgr.handles.group1_weather.humidity_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.humidity_label, "-%");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.humidity_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.humidity_label, lv_color_make(150, 200, 255), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.humidity_label, g_ui_mgr.handles.group1_weather.weather_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    /* 气压 - 湿度下方，左对齐 */
    g_ui_mgr.handles.group1_weather.pressure_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.pressure_label, "----hPa");
    lv_obj_add_style(g_ui_mgr.handles.group1_weather.pressure_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_weather.pressure_label, lv_color_make(150, 200, 255), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_weather.pressure_label, g_ui_mgr.handles.group1_weather.humidity_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    
    /* SHT30传感器数据 - 底部居中 */
    g_ui_mgr.handles.group1_weather.sensor_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "当前: --°C --%");
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
    lv_label_set_text(g_ui_mgr.handles.group1_stock.name_label, "等待数据");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.name_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.name_label, lv_color_white(), 0);
    lv_obj_align(g_ui_mgr.handles.group1_stock.name_label, LV_ALIGN_TOP_MID, 0, 0);
    
    /* 当前价格 - 标题下方，更大字号 */
    g_ui_mgr.handles.group1_stock.price_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.price_label, "----------");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.price_label, &g_ui_mgr.handles.style_xlarge, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.price_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_stock.price_label, g_ui_mgr.handles.group1_stock.name_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    /* 涨跌信息合并显示 */
    g_ui_mgr.handles.group1_stock.change_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.change_label, "----.----\n---.---%");
    lv_obj_add_style(g_ui_mgr.handles.group1_stock.change_label, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group1_stock.change_label, lv_color_make(255, 80, 80), 0);
    lv_obj_align_to(g_ui_mgr.handles.group1_stock.change_label, g_ui_mgr.handles.group1_stock.price_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
    
    /* 更新时间 - 底部居中，小字 */
    g_ui_mgr.handles.group1_stock.update_time_label = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group1_stock.update_time_label, "--:--:--");
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
    /* CPU图标 - 居中显示，占满板块 */
    lv_obj_t *cpu_icon = create_fullsize_icon(parent, get_cpu_icon());
    
    /* ✅ CPU温度 - 改为左对齐 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_temp = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, "--.-°C");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, lv_color_make(255, 100, 100), 0);
    // 改为左对齐，紧靠左边
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    
    /* CPU占用率 - 底部上方显示 (原温度位置上方) */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, "--.-%");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, lv_color_make(255, 165, 0), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, LV_ALIGN_BOTTOM_MID, 0, -70);
    
    /* CPU柱状图图表 - 底部显示，15个数据点 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_chart = create_usage_chart(parent, lv_color_make(255, 165, 0));
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_chart, LV_ALIGN_BOTTOM_MID, 3, -3);
}

/**
 * 构建中屏 - 内存监控面板
 */
static void build_middle_memory_panel(lv_obj_t *parent)
{
    /* 内存图标 - 居中显示，占满板块 */
    lv_obj_t *mem_icon = create_fullsize_icon(parent, get_mem_icon());
    
    /* ========== 左侧：内存使用率 + 图表 ========== */
    
    /* 内存使用率 - 左上角 */
    g_ui_mgr.handles.group2_memory.ram_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_usage, "--.--%");
    lv_obj_add_style(g_ui_mgr.handles.group2_memory.ram_usage, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_memory.ram_usage, lv_color_make(255, 215, 0), 0);
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_usage, LV_ALIGN_LEFT_MID, 5, -15);
    
    /* ✅ 内存图表 - 左侧底部 */
    g_ui_mgr.handles.group2_memory.ram_chart = create_memory_chart(parent, lv_color_make(255, 215, 0));
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_chart, LV_ALIGN_BOTTOM_LEFT, 3, -3);
    
    /* ========== 右侧：网络上传/下载 ========== */
    
    /* 上传速度 - 右侧中上 */
    g_ui_mgr.handles.group2_network.net_upload = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_upload, "-.--MB/s");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_upload, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_upload, lv_color_make(255, 100, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group2_network.net_upload, LV_ALIGN_RIGHT_MID, -3, 7);
    
    /* 下载速度 - 上传速度下方，右对齐 */
    g_ui_mgr.handles.group2_network.net_download = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_download, "-.--MB/s");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_download, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_download, lv_color_make(100, 255, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group2_network.net_download, LV_ALIGN_RIGHT_MID, -3, 50);
}

/**
 * 构建右屏 - 网络监控面板
 */
static void build_right_network_panel(lv_obj_t *parent)
{
    /* GPU图标 - 居中显示，占满板块 */
    lv_obj_t *gpu_icon = create_fullsize_icon(parent, get_gpu_icon());
    
    /* ✅ GPU温度 - 改为左对齐 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_temp = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, "--.-°C");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, lv_color_make(100, 255, 150), 0);
    // 改为左对齐，紧靠左边
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    
    /* GPU占用率 - 底部上方显示 (原温度位置上方) */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, "--.-%");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, lv_color_make(0, 255, 127), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, LV_ALIGN_BOTTOM_MID, 0, -70);
    
    /* GPU柱状图图表 - 底部显示，15个数据点 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_chart = create_usage_chart(parent, lv_color_make(0, 255, 127));
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_chart, LV_ALIGN_BOTTOM_MID, 3, -3);
}

/**
 * 创建内存使用率图表（5个柱形）
 * @param parent 父容器
 * @param color 柱子颜色
 * @return 图表对象
 */
static lv_obj_t* create_memory_chart(lv_obj_t *parent, lv_color_t color)
{
    lv_obj_t *container = lv_obj_create(parent);
    // 宽度约为屏幕的一半，高度与CPU/GPU图表一致
    lv_obj_set_size(container, (SCREEN_WIDTH / 2) - 5, 50);
    
    /* 容器样式 */
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    
    lv_coord_t bar_width = 8;   // 稍宽一些，因为只有5个柱子
    lv_coord_t bar_gap = 3;
    lv_coord_t start_x = 3;
    
    /* 创建5个bar */
    for (int i = 0; i < 5; i++) {
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

/**
 * 创建LVGL柱状图图表
 * @param parent 父容器
 * @param color 柱子颜色
 * @return 图表对象
 */
static lv_obj_t* create_usage_chart(lv_obj_t *parent, lv_color_t color)
{
    lv_obj_t *container = lv_obj_create(parent);
    // ✅ 高度从40改为50
    lv_obj_set_size(container, SCREEN_WIDTH - 10, 50);
    
    /* 容器样式 */
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    
    lv_coord_t bar_width = 5;
    lv_coord_t bar_gap = 2;
    lv_coord_t start_x = 5;
    
    /* 创建15个bar */
    for (int i = 0; i < 15; i++) {
        lv_obj_t *bar = lv_obj_create(container);
        lv_obj_set_size(bar, bar_width, 2);
        // ✅ Y坐标从36改为48（容器高度50，底部留2像素）
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

/**
 * 更新图表数据 - 添加新数据点并滚动
 * @param chart 图表对象
 * @param value 新的数值 (0-100)
 * @param color 柱子颜色
 */
static void update_usage_chart(lv_obj_t *container, float value, lv_color_t color)
{
    if (!container || !lv_obj_is_valid(container)) {
        return;
    }
    
    /* 限制数值范围 */
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    
    /* 计算高度 (0-100 映射到 2-35像素) */
    lv_coord_t bar_height = (lv_coord_t)((value * 33.0f / 100.0f) + 2.0f);
    if (bar_height < 2) bar_height = 2;
    if (bar_height > 35) bar_height = 35;
    
    /* 判断是CPU还是GPU图表 */
    bool is_cpu = (color.red == 255 && color.green == 165 && color.blue == 0);
    lv_coord_t *history = is_cpu ? chart_history.cpu_history : chart_history.gpu_history;
    
    /* ✅ 关键：所有历史数据向左移动一格（丢弃最左边的旧数据） */
    for (int i = 0; i < 14; i++) {
        history[i] = history[i + 1];  // 把右边的数据移到左边
    }
    
    /* ✅ 新数据放在最右边（索引14） */
    history[14] = bar_height;
    
    /* ✅ 更新所有柱子的显示 */
    for (int i = 0; i < 15; i++) {
        lv_obj_t *bar = lv_obj_get_child(container, i);
        if (bar && lv_obj_is_valid(bar)) {
            lv_coord_t h = history[i];
            if (h < 2) h = 2;
            
            /* 更新柱子高度和位置 */
            lv_obj_set_height(bar, h);
            lv_obj_set_y(bar, 38 - h);  // 底部对齐
        }
    }
}

/*********************
 *   GROUP 3 UI BUILD
 *********************/

/**
 * 构建左屏 - 媒体控制面板
 */
static void build_left_media_panel(lv_obj_t *parent)
{
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
 * 构建中屏 - 网页控制面板
 */
static void build_middle_web_panel(lv_obj_t *parent)
{
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
 * 构建右屏 - 快捷键面板
 */
static void build_right_shortcut_panel(lv_obj_t *parent)
{
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

    lv_img_set_antialias(img, true);                    // 抗锯齿
    lv_img_set_pivot(img, img_width/2, img_height/2);   // 居中缩放点
    
    // 移除所有内边距和边框
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    lv_obj_set_style_outline_width(img, 0, 0);
    
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
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) {
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
    
    // 更新秒钟显示
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.sec_tens, sec / 10);
    update_digit_image(g_ui_mgr.handles.l2_digital_clock.sec_units, sec % 10);
    static int last_sec = -1;
    if (sec != last_sec) {
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
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) return;
    
    lv_obj_t *vol_up_icon = create_entrance_icon(g_ui_mgr.handles.left_panel, get_volup_image());
    if (vol_up_icon) {
        lv_obj_align(vol_up_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *vol_up_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(vol_up_hint, "音量+");
    lv_obj_add_style(vol_up_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(vol_up_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(vol_up_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_obj_t *vol_down_icon = create_entrance_icon(g_ui_mgr.handles.middle_panel, get_voldown_image());
    if (vol_down_icon) {
        lv_obj_align(vol_down_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *vol_down_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(vol_down_hint, "音量-");
    lv_obj_add_style(vol_down_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(vol_down_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(vol_down_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_obj_t *play_pause_icon = create_entrance_icon(g_ui_mgr.handles.right_panel, get_play_image());
    if (play_pause_icon) {
        lv_obj_align(play_pause_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *play_pause_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(play_pause_hint, "播放/暂停");
    lv_obj_add_style(play_pause_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(play_pause_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(play_pause_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * 构建网页控制L2页面
 */
static void build_l2_web_control_page(void)
{
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) return;
    
    lv_obj_t *page_up_icon = create_entrance_icon(g_ui_mgr.handles.left_panel, get_up_image());
    if (page_up_icon) {
        lv_obj_align(page_up_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *page_up_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(page_up_hint, "上翻页");
    lv_obj_add_style(page_up_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(page_up_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(page_up_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏：下翻页 */

    lv_obj_t *page_down_icon = create_entrance_icon(g_ui_mgr.handles.middle_panel, get_down_image());
    if (page_down_icon) {
        lv_obj_align(page_down_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *page_down_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(page_down_hint, "下翻页");
    lv_obj_add_style(page_down_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(page_down_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(page_down_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏：刷新 */
    lv_obj_t *refresh_icon = create_entrance_icon(g_ui_mgr.handles.right_panel, get_fresh_image());
    if (refresh_icon) {
        lv_obj_align(refresh_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *refresh_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(refresh_hint, "刷新F5");
    lv_obj_add_style(refresh_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(refresh_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(refresh_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * 构建快捷键控制L2页面
 */
static void build_l2_shortcut_control_page(void)
{
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) return;
    
    /* 左屏：复制 */
    lv_obj_t *copy_icon = create_entrance_icon(g_ui_mgr.handles.left_panel, get_ctrlc_image());
    if (copy_icon) {
        lv_obj_align(copy_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *copy_hint = lv_label_create(g_ui_mgr.handles.left_panel);
    lv_label_set_text(copy_hint, "复制");
    lv_obj_add_style(copy_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(copy_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(copy_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 中屏：粘贴 */
    lv_obj_t *paste_icon = create_entrance_icon(g_ui_mgr.handles.middle_panel, get_ctrlv_image());
    if (paste_icon) {
        lv_obj_align(paste_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *paste_hint = lv_label_create(g_ui_mgr.handles.middle_panel);
    lv_label_set_text(paste_hint, "粘贴");
    lv_obj_add_style(paste_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(paste_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(paste_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    /* 右屏：撤销 */
    lv_obj_t *undo_icon = create_entrance_icon(g_ui_mgr.handles.right_panel, get_ctrlz_image());
    if (undo_icon) {
        lv_obj_align(undo_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    lv_obj_t *undo_hint = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(undo_hint, "撤销");
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
        return 0;
    }

    /* 计算缩放因子 */
    g_ui_mgr.scale_factor = get_scale_factor();

    /* 创建动态字体 */
    if (create_fonts() != 0) {
        return -RT_ERROR;
    }

    /* 创建基础UI结构 */
    if (create_base_ui() != 0) {
        cleanup_fonts();
        return -RT_ERROR;
    }

    g_ui_mgr.current_group = SCREEN_GROUP_1;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    g_ui_mgr.initialized = true;
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
    return 0;
}

int screen_ui_build_group1(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    safe_cleanup_ui_objects();
    
    build_left_datetime_panel(g_ui_mgr.handles.left_panel);
    build_middle_weather_panel(g_ui_mgr.handles.middle_panel);
    build_right_stock_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_1;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_1);
    
    lv_obj_invalidate(lv_scr_act());
    
    return 0;
}

int screen_ui_build_group2(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    safe_cleanup_ui_objects();
    
    build_left_cpu_gpu_panel(g_ui_mgr.handles.left_panel);
    build_middle_memory_panel(g_ui_mgr.handles.middle_panel);
    build_right_network_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_2;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_2);
    
    lv_obj_invalidate(lv_scr_act());
    
    return 0;
}

int screen_ui_build_group3(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    
    safe_cleanup_ui_objects();
    
    build_left_media_panel(g_ui_mgr.handles.left_panel);
    build_middle_web_panel(g_ui_mgr.handles.middle_panel);
    build_right_shortcut_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_3;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_3);
    
    lv_obj_invalidate(lv_scr_act());
    
    return 0;
}

int screen_ui_build_l2_time(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    
    safe_cleanup_ui_objects();
    build_l2_time_detail_page();  // 使用新的数字时钟页面
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_TIME_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
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
    
    if (g_ui_mgr.handles.l2_muyu_main.counter_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.counter_label)) {
        // 在木鱼页面,调用木鱼更新函数
        return screen_ui_update_muyu_display();
    }
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

    /* 更新天气描述和天气图标 */
    if (g_ui_mgr.handles.group1_weather.weather_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.weather_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.weather_label, data->weather);
    }
    if (g_ui_mgr.handles.group1_weather.weather_icon && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.weather_icon)) {
        const lv_image_dsc_t* weather_icon = get_weather_icon_by_code(data->weather_code);
        lv_img_set_src(g_ui_mgr.handles.group1_weather.weather_icon, weather_icon);
    }
    /* 更新湿度 */
    if (g_ui_mgr.handles.group1_weather.humidity_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.humidity_label)) {
        char humidity_str[16];
        rt_snprintf(humidity_str, sizeof(humidity_str), "%.0f%%", data->humidity);
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

    /* ========== CPU 左屏更新 ========== */
    
    /* 更新CPU温度 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_temp && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->cpu_temp);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, temp_str);
    }

    /* 更新CPU使用率 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage)) {
        char usage_str[16];
        rt_snprintf(usage_str, sizeof(usage_str), "%.1f%%", data->cpu_usage);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, usage_str);
        
        /* ✅ 添加静态计数器，控制图表更新频率 */
        static uint8_t cpu_update_counter = 0;
        cpu_update_counter++;
        
        /* ✅ 每5次数据更新才滚动图表一次（可调整） */
        if (cpu_update_counter >= 5) {
            cpu_update_counter = 0;
            
            if (g_ui_mgr.handles.group2_cpu_gpu.cpu_chart && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_chart)) {
                /* 滚动历史数据 */
                for (int i = 0; i < 14; i++) {
                    chart_history.cpu_history[i] = chart_history.cpu_history[i + 1];
                }
                
                /* 计算新柱子高度 */
                lv_coord_t bar_height = (lv_coord_t)((data->cpu_usage * 55.0f / 100.0f) + 2.0f);

                chart_history.cpu_history[14] = bar_height;
                
                /* 更新所有柱子显示 */
                for (int i = 0; i < 15; i++) {
                    lv_obj_t *bar = lv_obj_get_child(g_ui_mgr.handles.group2_cpu_gpu.cpu_chart, i);
                    if (bar && lv_obj_is_valid(bar)) {
                        lv_coord_t h = chart_history.cpu_history[i];
                        if (h < 2) h = 2;
                        lv_obj_set_height(bar, h);
                        // ✅ 从 38-h 改为 48-h
                        lv_obj_set_y(bar, 48 - h);
                    }
                }
            }
        }
    }

    /* ========== GPU 右屏更新 ========== */
    
    /* 更新GPU温度 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_temp && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->gpu_temp);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, temp_str);
    }

    /* 更新GPU使用率 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage)) {
        char usage_str[16];
        rt_snprintf(usage_str, sizeof(usage_str), "%.1f%%", data->gpu_usage);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, usage_str);
        
        /* ✅ GPU图表也添加节流 */
        static uint8_t gpu_update_counter = 0;
        gpu_update_counter++;
        
        if (gpu_update_counter >= 5) {
            gpu_update_counter = 0;
            
            if (g_ui_mgr.handles.group2_cpu_gpu.gpu_chart && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_chart)) {
                /* 滚动历史数据 */
                for (int i = 0; i < 14; i++) {
                    chart_history.gpu_history[i] = chart_history.gpu_history[i + 1];
                }
                
                /* 计算新柱子高度 */
                lv_coord_t bar_height = (lv_coord_t)((data->gpu_usage * 33.0f / 100.0f) + 2.0f);
                if (bar_height < 2) bar_height = 2;
                if (bar_height > 35) bar_height = 35;
                chart_history.gpu_history[14] = bar_height;
                
                /* 更新所有柱子显示 */
                for (int i = 0; i < 15; i++) {
                    lv_obj_t *bar = lv_obj_get_child(g_ui_mgr.handles.group2_cpu_gpu.gpu_chart, i);
                    if (bar && lv_obj_is_valid(bar)) {
                        lv_coord_t h = chart_history.gpu_history[i];
                        if (h < 2) h = 2;
                        lv_obj_set_height(bar, h);
                        // ✅ 从 38-h 改为 48-h
                        lv_obj_set_y(bar, 48 - h);
                    }
                }
            }
        }
    }

    /* 更新内存使用率 - 中屏左侧 */
    if (g_ui_mgr.handles.group2_memory.ram_usage && 
        lv_obj_is_valid(g_ui_mgr.handles.group2_memory.ram_usage)) {
        char ram_str[16];
        rt_snprintf(ram_str, sizeof(ram_str), "%.1f%%", data->ram_usage);
        lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_usage, ram_str);
            /* 内存图表更新（节流处理，每5次更新一次） */
    static uint8_t mem_update_counter = 0;
    mem_update_counter++;
    
    if (mem_update_counter >= 5) {
        mem_update_counter = 0;
        
        if (g_ui_mgr.handles.group2_memory.ram_chart && 
            lv_obj_is_valid(g_ui_mgr.handles.group2_memory.ram_chart)) {
            
            /* 滚动历史数据（只有5个柱形） */
            for (int i = 0; i < 4; i++) {
                chart_history.mem_history[i] = chart_history.mem_history[i + 1];
            }
            
            /* 计算新柱子高度 (0-100 映射到 2-48像素) */
            lv_coord_t bar_height = (lv_coord_t)((data->ram_usage * 46.0f / 100.0f) + 2.0f);
            if (bar_height < 2) bar_height = 2;
            if (bar_height > 48) bar_height = 48;
            chart_history.mem_history[4] = bar_height;
            
            /* 更新所有柱子显示 */
            for (int i = 0; i < 5; i++) {
                lv_obj_t *bar = lv_obj_get_child(g_ui_mgr.handles.group2_memory.ram_chart, i);
                if (bar && lv_obj_is_valid(bar)) {
                    lv_coord_t h = chart_history.mem_history[i];
                    if (h < 2) h = 2;
                    lv_obj_set_height(bar, h);
                    lv_obj_set_y(bar, 48 - h);  // 底部对齐
                }
            }
        }
    }
}
    
    /* 更新网络上传速度 - 中屏右侧 */
    if (g_ui_mgr.handles.group2_network.net_upload && 
        lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_upload)) {
        char upload_str[32];
        rt_snprintf(upload_str, sizeof(upload_str), "%.2fMB/s", data->net_upload_speed);
        lv_label_set_text(g_ui_mgr.handles.group2_network.net_upload, upload_str);
    }
    
    /* 更新网络下载速度 - 中屏右侧 */
    if (g_ui_mgr.handles.group2_network.net_download && 
        lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_download)) {
        char download_str[32];
        rt_snprintf(download_str, sizeof(download_str), "%.2fMB/s", data->net_download_speed);
        lv_label_set_text(g_ui_mgr.handles.group2_network.net_download, download_str);
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
            rt_snprintf(sensor_str, sizeof(sensor_str), "当前: %.1f°C %.0f%%",
                      data.temperature_c, data.humidity_rh);
            lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, sensor_str);
        } else {
            lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "当前: --°C --%");
        }
    } else {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.sensor_label, "当前: --°C --%");
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


/*********************
 *   GROUP 4 UI BUILD - 新增实用工具页面
 *********************/


/**
 * 构建左屏 - 赛博木鱼面板
 */
static void build_left_muyu_panel(lv_obj_t *parent)
{
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

/**
 * 构建中屏 - 番茄钟面板
 */
static void build_middle_tomato_panel(lv_obj_t *parent)
{
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

/**
 * 构建右屏 - 计时器面板
 */
static void build_right_gallery_panel(lv_obj_t *parent)
{
    /* 计时器图片入口 - 上半部分居中 */
    g_ui_mgr.handles.group4_gallery.gallery_icon = create_entrance_icon(parent, get_calculagraph_image());
    if (g_ui_mgr.handles.group4_gallery.gallery_icon) {
        lv_obj_align(g_ui_mgr.handles.group4_gallery.gallery_icon, LV_ALIGN_CENTER, 0, -10);
    }
    
    /* 功能提示 - 底部小字 */
    g_ui_mgr.handles.group4_gallery.gallery_hint = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group4_gallery.gallery_hint, "计时器");
    lv_obj_add_style(g_ui_mgr.handles.group4_gallery.gallery_hint, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group4_gallery.gallery_hint, lv_color_make(200, 200, 200), 0);
    lv_obj_align(g_ui_mgr.handles.group4_gallery.gallery_hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}
/**
 * 创建入口图标(通用函数)
 * @param parent 父容器
 * @param img_src 图片资源
 * @return 创建的图片对象指针
 */
static lv_obj_t* create_entrance_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src)
{
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
        return NULL;
    }
    
    // 设置图片资源
    lv_image_set_src(img, img_src);
    
    /* 入口图标使用较小尺寸 - 缩小50% */
    lv_coord_t icon_size = (lv_coord_t)(SCREEN_WIDTH * 0.1f);  // 25%
    lv_obj_set_size(img, icon_size, icon_size);
    
    // 适中的缩放比例 - 缩小50%
    float scale = g_ui_mgr.scale_factor * 0.50f;
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    // 移除所有边距和边框
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    
    return img;
}
//  创建全板块尺寸图标（通用函数）
static lv_obj_t* create_fullsize_icon(lv_obj_t *parent, const lv_image_dsc_t *img_src)
{
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
        return NULL;
    }
    
    // 设置图片资源
    lv_img_set_src(img, img_src);
    
    // 全板块尺寸 - 占满整个面板
    lv_coord_t icon_size = SCREEN_WIDTH;
    lv_obj_set_size(img, icon_size, icon_size);
    
    // 居中显示
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    // 适中的缩放比例 - 根据实际图片大小调整
    float scale = g_ui_mgr.scale_factor * 0.57f;  // 可根据需要调整
    
    lv_img_set_zoom(img, (int)(LV_IMG_ZOOM_NONE * scale));
    lv_img_set_antialias(img, true);
    
    // 移除所有边距和边框
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_border_width(img, 0, 0);
    lv_obj_add_flag(img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    
    return img;
}

//获取CPU\MEM图片资源和GPU图片资源
static const lv_image_dsc_t* get_cpu_icon(void)
{
    return &cpuicon;
}
static const lv_image_dsc_t* get_gpu_icon(void)
{
    return &gpuicon;
}
static const lv_image_dsc_t* get_mem_icon(void)
{
    return &memicon;
}
//获取媒体控制图片资源
static const lv_image_dsc_t* get_media_image(void){
    return &media;
}
//获取网页控制图片资源
static const lv_image_dsc_t* get_web_image(void)
{
    return &web;
}
//获取快捷键图片资源
static const lv_image_dsc_t* get_shortcut_image(void)
{
    return &shortcut;
}
//获取木鱼图片资源
static const lv_image_dsc_t* get_muyu_image(void)
{
    return &muyu;
}
//获取番茄钟图片资源
static const lv_image_dsc_t* get_tomato_image(void)
{
    return &tomatolock;
}
//获取计时器图片资源
static const lv_image_dsc_t* get_calculagraph_image(void)
{
    return &calculagraph;
}
// ========== L2页面图标获取函数 ==========

// 获取音量+图片资源
static const lv_image_dsc_t* get_volup_image(void)
{
    return &volup;
}

// 获取音量-图片资源
static const lv_image_dsc_t* get_voldown_image(void)
{
    return &voldown;
}

// 获取播放/暂停图片资源
static const lv_image_dsc_t* get_play_image(void)
{
    return &play;
}

// 获取复制图片资源
static const lv_image_dsc_t* get_ctrlc_image(void)
{
    return &ctrlc;
}

// 获取粘贴图片资源
static const lv_image_dsc_t* get_ctrlv_image(void)
{
    return &ctrlv;
}

// 获取撤销图片资源
static const lv_image_dsc_t* get_ctrlz_image(void)
{
    return &ctrlz;
}

// 获取上翻页图片资源
static const lv_image_dsc_t* get_up_image(void)
{
    return &up;
}

// 获取下翻页图片资源
static const lv_image_dsc_t* get_down_image(void)
{
    return &down;
}

// 获取刷新图片资源
static const lv_image_dsc_t* get_fresh_image(void)
{
    return &fresh;
}

/**
 * 创建L2木鱼主界面的展示图片（非点击）
 */
static lv_obj_t* create_muyu_display_image(lv_obj_t *parent)
{
    lv_obj_t *img = lv_img_create(parent);
    if (!img) {
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
    
    return img;
}

/**
 * 构建L2赛博木鱼主界面
 */
static void build_l2_muyu_main_page(void)
{
    
    if (!g_ui_mgr.handles.left_panel || !g_ui_mgr.handles.middle_panel || !g_ui_mgr.handles.right_panel) {
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
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.total_label, "--");
    lv_obj_add_style(g_ui_mgr.handles.l2_muyu_main.total_label, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.l2_muyu_main.total_label, lv_color_white(), 0);
    lv_obj_align_to(g_ui_mgr.handles.l2_muyu_main.total_label, total_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    // 功德等级显示
    g_ui_mgr.handles.l2_muyu_main.merit_label = lv_label_create(g_ui_mgr.handles.right_panel);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.merit_label, "lv1");
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
    if (g_ui_mgr.muyu_data.sound_enabled == 0) {  // 仅首次初始化配置
        g_ui_mgr.muyu_data.sound_enabled = true;
        g_ui_mgr.muyu_data.auto_save = true;
        g_ui_mgr.muyu_data.tap_effect_level = 1;
    }
}


int screen_ui_build_group4(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    safe_cleanup_ui_objects();
    
    build_left_muyu_panel(g_ui_mgr.handles.left_panel);
    build_middle_tomato_panel(g_ui_mgr.handles.middle_panel);
    build_right_gallery_panel(g_ui_mgr.handles.right_panel);
    
    g_ui_mgr.current_group = SCREEN_GROUP_4;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    /* 激活按键上下文 */
    screen_context_activate_for_group(SCREEN_GROUP_4);
    
    lv_obj_invalidate(lv_scr_act());
    
    return 0;
}

int screen_ui_build_l2_muyu(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    safe_cleanup_ui_objects();
    build_l2_muyu_main_page();
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    
    /* 激活L2按键上下文 */
    screen_context_activate_for_level2(SCREEN_L2_MUYU_GROUP);
    
    lv_obj_invalidate(lv_scr_act());
    return 0;
}

int screen_ui_build_l2_tomato(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

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
        return 0; // 不在木鱼页面,不更新
    }
    
    // 从screen_context获取真实的计数数据
    uint32_t current_count = 0;
    uint32_t total_count = 0;
    screen_context_get_muyu_count(&current_count, &total_count);
    
    // 同步到本地数据结构(可选)
    g_ui_mgr.muyu_data.tap_count = current_count;
    g_ui_mgr.muyu_data.total_taps = total_count;
    
    // 更新当前计数显示
    char counter_str[16];
    rt_snprintf(counter_str, sizeof(counter_str), "%u", current_count);
    lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.counter_label, counter_str);
    
    // 更新总计数显示
    if (g_ui_mgr.handles.l2_muyu_main.total_label && 
        lv_obj_is_valid(g_ui_mgr.handles.l2_muyu_main.total_label)) {
        char total_str[16];
        rt_snprintf(total_str, sizeof(total_str), "%u", total_count);
        lv_label_set_text(g_ui_mgr.handles.l2_muyu_main.total_label, total_str);
    }
    
    // 更新功德等级显示
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

const muyu_data_t* screen_ui_get_muyu_data(void)
{
    return &g_ui_mgr.muyu_data;
}

int screen_ui_reset_muyu_counter(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }

    screen_context_handle_muyu_reset();

    screen_ui_update_muyu_display();
    
    return 0;
}
