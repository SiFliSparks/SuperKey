#include "../screen/screen_ui_manager.h"
#include "../screen/screen_core.h"
#include "../custom/custom_icon_loader.h"
#include "../manager/data_manager.h"
#include "../device/sht30_controller.h"
#include "../screen/screen_context.h"
#include "../screen/screen_init/screen_ui_common.h"
#include "../screen/screen_init/screen_ui_resources.h"
#include "../screen/screen_init/screen_ui_group1.h"
#include "../screen/screen_init/screen_ui_group2.h"
#include "../screen/screen_init/screen_ui_group3.h"
#include "../screen/screen_init/screen_ui_group4.h"
#include "../screen/screen_init/screen_ui_group5.h"
#include "../screen/screen_init/screen_l2/screen_ui_l2_time.h"
#include "lv_tiny_ttf.h"
#include <rtthread.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "../widget/widget_ui.h"
#include "../widget/widget_manager.h"
#include "../mp3/mp3_screen_ui.h"
#include "../mp3/mp3_screen_context.h"

/* 注意: 屏幕尺寸宏、布局宏、中文数组等已在 screen_ui_common.h 中定义 */

static bool is_showing_tomato_mode(void);
static int screen_ui_update_time_display_normal_mode(void);
static int screen_ui_update_time_display_tomato_mode(void);

/* 外部字体数据声明 */
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;

/* 注意: 图片资源声明已移至 screen_ui_resources.h */

/*********************
 *  STATIC VARIABLES
 *********************/
screen_ui_manager_t g_ui_mgr = {0};
static struct {
    lv_coord_t mem_history[5];
} chart_history = {0};
/* 番茄钟后台显示轮换状态 */
static struct {
    bool enabled;               // 是否启用轮换
    bool show_tomato;          // true=显示番茄钟, false=显示正常时间
    rt_timer_t switch_timer;   // 切换定时器
    rt_tick_t last_switch_tick;
} g_tomato_background_display = {0};
/*********************
 *  STATIC PROTOTYPES
 *********************/

/* 注意: Group 1 UI构建函数已移至 screen_ui_group1.h/c */
/* 注意: Group 2 UI构建函数已移至 screen_ui_group2.h/c */
/* 注意: Group 3 UI构建函数已移至 screen_ui_group3.h/c */
/* 注意: Group 4 UI构建函数已移至 screen_ui_group4.h/c */
/* 注意: Group 5 UI构建函数已移至 screen_ui_group5.h/c */

/* Group 4 UI构建 - 包装函数声明 */
static void build_left_muyu_panel(lv_obj_t *parent);
static void build_middle_tomato_panel(lv_obj_t *parent);
static void build_right_stopwatch_panel(lv_obj_t *parent);
static void build_l2_muyu_main_page(lv_obj_t *screen);

/* Group 5 UI构建 - 包装函数声明 */
static void build_left_custom1_panel(lv_obj_t *parent);
static void build_middle_custom2_panel(lv_obj_t *parent);
static void build_right_custom3_panel(lv_obj_t *parent);

/* 注意: 图片资源获取函数已移至 screen_ui_resources.h/c */

/* L2 UI构建 - 数字时钟已移至 screen_ui_l2_time.h/c */
/* 包装函数声明 */
static void build_l2_time_detail_page(lv_obj_t *screen);
static int screen_ui_update_l2_digital_clock(void);

/* 注意: build_l2_muyu_main_page, build_l2_tomato_page 已移至 screen_ui_group4.h/c */
/* 注意: build_l2_media_control_page, build_l2_web_control_page, build_l2_shortcut_control_page
         已移至 screen_ui_group3.h/c */

/* 注意: get_weather_icon_by_code 已改名为 get_weather_icon 并移至 screen_ui_resources.c */
#define get_weather_icon_by_code(code) get_weather_icon(code)

/*********************
 *   GROUP 1 UI BUILD
 *   (已移至 screen_ui_group1.c)
 *********************/

/* 包装函数 - 调用新模块 */
static void build_left_datetime_panel(lv_obj_t *parent)
{
    group1_build_left_datetime_panel(parent);
}

static void build_middle_weather_panel(lv_obj_t *parent)
{
    group1_build_middle_weather_panel(parent);
}

static void build_right_widget_panel(lv_obj_t *parent)
{
    group1_build_right_widget_panel(parent);
}

static void build_l2_weather_forecast_page(lv_obj_t *screen)
{
    group1_build_l2_weather_forecast_page(screen);
}

int screen_ui_update_l2_weather_forecast(const weather_forecast_data_t *forecast)
{
    return group1_update_l2_weather_forecast(forecast);
}


/*********************
 *   GROUP 2 UI BUILD
 *   (已移至 screen_ui_group2.c)
 *********************/

/* 包装函数 - 调用新模块 */
static void build_left_cpu_gpu_panel(lv_obj_t *parent)
{
    group2_build_left_cpu_panel(parent);
}

static void build_middle_memory_panel(lv_obj_t *parent)
{
    group2_build_middle_memory_panel(parent);
}

static void build_right_network_panel(lv_obj_t *parent)
{
    group2_build_right_gpu_panel(parent);
}

/*********************
 *   GROUP 3 UI BUILD
 *   (已移至 screen_ui_group3.c)
 *********************/

/* 包装函数 - 调用新模块 */
static void build_left_media_panel(lv_obj_t *parent)
{
    group3_build_left_media_panel(parent);
}

static void build_middle_web_panel(lv_obj_t *parent)
{
    group3_build_middle_web_panel(parent);
}

static void build_right_shortcut_panel(lv_obj_t *parent)
{
    group3_build_right_shortcut_panel(parent);
}


/*********************
 *   GROUP 5 UI BUILD
 *   (已移至 screen_ui_group5.c)
 *********************/

/* 包装函数 - 调用新模块 */
static void build_left_custom1_panel(lv_obj_t *parent)
{
    group5_build_left_custom1_panel(parent);
}

static void build_middle_custom2_panel(lv_obj_t *parent)
{
    group5_build_middle_custom2_panel(parent);
}

static void build_right_custom3_panel(lv_obj_t *parent)
{
    group5_build_right_custom3_panel(parent);
}

/*********************
 *   L2 DIGITAL CLOCK
 *   (已移至 screen_ui_l2_time.c)
 *********************/

/* 包装函数 - 调用新模块 */
static void build_l2_time_detail_page(lv_obj_t *screen)
{
    l2_time_build_digital_clock_page(screen);
}

static int screen_ui_update_l2_digital_clock(void)
{
    return l2_time_update_digital_clock();
}

/*********************
 *   OTHER L2 UI BUILD
 *   (Group 3 L2页面已移至 screen_ui_group3.c)
 *********************/

/* 包装函数 - 调用新模块 */
static void build_l2_media_control_page(lv_obj_t *screen)
{
    group3_build_l2_media_control_page(screen);
}

static void build_l2_web_control_page(lv_obj_t *screen)
{
    group3_build_l2_web_control_page(screen);
}

static void build_l2_shortcut_control_page(lv_obj_t *screen)
{
    group3_build_l2_shortcut_control_page(screen);
}

/*********************
 *   PUBLIC API
 *********************/

int screen_ui_manager_init(void)
{
    if (g_ui_mgr.initialized) {
        return 0;
    }
    
    /* 使用 screen_ui_common 模块的函数 */
    g_ui_mgr.scale_factor = calc_scale_factor();
    
    if (create_fonts() != 0) {
        return -RT_ERROR;
    }
    
    g_ui_mgr.handles.screen_group1 = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_group2 = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_group3 = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_group4 = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_group5 = lv_obj_create(NULL);
    
    g_ui_mgr.handles.screen_l2_time_detail = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_weather_forecast = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_muyu = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_tomato = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_stopwatch = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_media = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_web = lv_obj_create(NULL);
    g_ui_mgr.handles.screen_l2_shortcut = lv_obj_create(NULL);
    
    if (!g_ui_mgr.handles.screen_group1 || !g_ui_mgr.handles.screen_group2 ||
        !g_ui_mgr.handles.screen_group3 || !g_ui_mgr.handles.screen_group4 || !g_ui_mgr.handles.screen_group5) {
        cleanup_fonts();
        return -RT_ERROR;
    }
    
    if (!g_ui_mgr.handles.screen_l2_time_detail || !g_ui_mgr.handles.screen_l2_weather_forecast ||
        !g_ui_mgr.handles.screen_l2_muyu || !g_ui_mgr.handles.screen_l2_tomato ||
        !g_ui_mgr.handles.screen_l2_stopwatch || !g_ui_mgr.handles.screen_l2_media ||
        !g_ui_mgr.handles.screen_l2_web || !g_ui_mgr.handles.screen_l2_shortcut) {
        cleanup_fonts();
        return -RT_ERROR;
    }
    
    setup_screen_base_style(g_ui_mgr.handles.screen_group1);
    setup_screen_base_style(g_ui_mgr.handles.screen_group2);
    setup_screen_base_style(g_ui_mgr.handles.screen_group3);
    setup_screen_base_style(g_ui_mgr.handles.screen_group5);
    setup_screen_base_style(g_ui_mgr.handles.screen_group4);
    
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_time_detail);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_weather_forecast);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_muyu);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_tomato);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_stopwatch);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_media);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_web);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_shortcut);
    
    g_ui_mgr.current_group = SCREEN_GROUP_1;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    g_ui_mgr.current_l2_group = SCREEN_L2_TIME_GROUP;
    
    g_ui_mgr.group1_built = false;
    g_ui_mgr.group2_built = false;
    g_ui_mgr.group5_built = false;
    g_ui_mgr.group3_built = false;
    g_ui_mgr.group4_built = false;
    g_ui_mgr.group6_built = false;
    g_ui_mgr.l2_time_built = false;
    g_ui_mgr.l2_weather_built = false;
    g_ui_mgr.l2_muyu_built = false;
    g_ui_mgr.l2_tomato_built = false;
    g_ui_mgr.l2_stopwatch_built = false;
    g_ui_mgr.l2_media_built = false;
    g_ui_mgr.l2_web_built = false;
    g_ui_mgr.l2_shortcut_built = false;
    
    g_ui_mgr.initialized = true;
    
    return 0;
}

/* ========== Group1 构建 ========== */
int screen_ui_build_group1(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.group1_built) {
        return 0;
    }
    
    lv_obj_t *screen = g_ui_mgr.handles.screen_group1;
    
    g_ui_mgr.handles.g1_left_panel = create_panel(screen, LEFT_X);
    g_ui_mgr.handles.g1_middle_panel = create_panel(screen, MID_X);
    g_ui_mgr.handles.g1_right_panel = create_panel(screen, RIGHT_X);
    
    build_left_datetime_panel(g_ui_mgr.handles.g1_left_panel);
    build_middle_weather_panel(g_ui_mgr.handles.g1_middle_panel);
    build_right_widget_panel(g_ui_mgr.handles.g1_right_panel);
    
    g_ui_mgr.group1_built = true;
    
    weather_data_t weather = {0};
    if (data_manager_get_weather(&weather) == 0 && weather.valid) {
        screen_ui_update_weather_display(&weather);
    }
    
    screen_ui_update_time_display();
    screen_ui_update_sensor_display();
    
    return 0;
}

/* ========== Group2 构建 ========== */
int screen_ui_build_group2(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.group2_built) {
        return 0;
    }
    
    lv_obj_t *screen = g_ui_mgr.handles.screen_group2;
    
    g_ui_mgr.handles.g2_left_panel = create_panel(screen, LEFT_X);
    g_ui_mgr.handles.g2_middle_panel = create_panel(screen, MID_X);
    g_ui_mgr.handles.g2_right_panel = create_panel(screen, RIGHT_X);
    
    build_left_cpu_gpu_panel(g_ui_mgr.handles.g2_left_panel);
    build_middle_memory_panel(g_ui_mgr.handles.g2_middle_panel);
    build_right_network_panel(g_ui_mgr.handles.g2_right_panel);
    
    g_ui_mgr.group2_built = true;
    
    system_monitor_data_t system_data = {0};
    if (data_manager_get_system(&system_data) == 0 && system_data.valid) {
        screen_ui_update_system_display(&system_data);
    }
    
    return 0;
}

/* ========== Group3 构建 ========== */
int screen_ui_build_group3(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.group3_built) {
        return 0;
    }
    
    lv_obj_t *screen = g_ui_mgr.handles.screen_group3;
    
    g_ui_mgr.handles.g3_left_panel = create_panel(screen, LEFT_X);
    g_ui_mgr.handles.g3_middle_panel = create_panel(screen, MID_X);
    g_ui_mgr.handles.g3_right_panel = create_panel(screen, RIGHT_X);
    
    build_left_media_panel(g_ui_mgr.handles.g3_left_panel);
    build_middle_web_panel(g_ui_mgr.handles.g3_middle_panel);
    build_right_shortcut_panel(g_ui_mgr.handles.g3_right_panel);
    
    g_ui_mgr.group3_built = true;
    
    return 0;
}

/* ========== Group4 构建 ========== */
int screen_ui_build_group4(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.group4_built) {
        return 0;
    }
    
    lv_obj_t *screen = g_ui_mgr.handles.screen_group4;
    
    g_ui_mgr.handles.g4_left_panel = create_panel(screen, LEFT_X);
    g_ui_mgr.handles.g4_middle_panel = create_panel(screen, MID_X);
    g_ui_mgr.handles.g4_right_panel = create_panel(screen, RIGHT_X);
    
    build_left_muyu_panel(g_ui_mgr.handles.g4_left_panel);
    build_middle_tomato_panel(g_ui_mgr.handles.g4_middle_panel);
    build_right_stopwatch_panel(g_ui_mgr.handles.g4_right_panel);
    
    g_ui_mgr.group4_built = true;
    
    return 0;
}


/* ========== Group5 构建 ========== */
int screen_ui_build_group5(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.group5_built) {
        return 0;
    }
    
    /* 初始化自定义图标加载器（首次构建时会扫描SD卡） */
    custom_icon_loader_init();
    
    lv_obj_t *screen = g_ui_mgr.handles.screen_group5;
    
    g_ui_mgr.handles.g5_left_panel = create_panel(screen, LEFT_X);
    g_ui_mgr.handles.g5_middle_panel = create_panel(screen, MID_X);
    g_ui_mgr.handles.g5_right_panel = create_panel(screen, RIGHT_X);
    
    build_left_custom1_panel(g_ui_mgr.handles.g5_left_panel);
    build_middle_custom2_panel(g_ui_mgr.handles.g5_middle_panel);
    build_right_custom3_panel(g_ui_mgr.handles.g5_right_panel);
    
    g_ui_mgr.group5_built = true;
    
    return 0;
}

/**
 * @brief 构建 Group6 MP3 播放器界面
 */
int screen_ui_build_group6(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.group6_built) {
        return 0;
    }
    
    /* 创建屏幕 */
    if (g_ui_mgr.handles.screen_group6_mp3 == NULL) {
        g_ui_mgr.handles.screen_group6_mp3 = lv_obj_create(NULL);
        setup_screen_base_style(g_ui_mgr.handles.screen_group6_mp3);
    }
    
    /* 构建 MP3 UI */
    mp3_screen_ui_build(g_ui_mgr.handles.screen_group6_mp3);
    
    g_ui_mgr.group6_built = true;
    
    return 0;
}

int screen_ui_build_l2_time(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_time_built) {
        return 0;
    }
    
    build_l2_time_detail_page(g_ui_mgr.handles.screen_l2_time_detail);
    
    g_ui_mgr.l2_time_built = true;
    
    screen_ui_update_l2_digital_clock();
    
    return 0;
}

int screen_ui_build_l2_muyu(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_muyu_built) {
        return 0;
    }
    
    build_l2_muyu_main_page(g_ui_mgr.handles.screen_l2_muyu);
    
    g_ui_mgr.l2_muyu_built = true;
    
    screen_ui_update_muyu_display();
    
    return 0;
}

int screen_ui_build_l2_tomato(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_tomato_built) {
        return 0;
    }
    
    /* 调用新模块构建番茄钟L2页面 */
    group4_build_l2_tomato_page(g_ui_mgr.handles.screen_l2_tomato);
    
    /* 标记已构建 */
    g_ui_mgr.l2_tomato_built = true;
    
    /* 初始更新显示 */
    screen_ui_update_tomato_display();
    
    return 0;
}

int screen_ui_build_l2_widget_selector(void)
{
    if (g_ui_mgr.l2_widget_selector_built) {
        return 0;
    }
    
    /* 创建L2屏幕 */
    g_ui_mgr.handles.screen_l2_widget_selector = lv_obj_create(NULL);
    setup_screen_base_style(g_ui_mgr.handles.screen_l2_widget_selector);
    
    /* 使用小工具UI模块构建选择器 */
    widget_ui_build_selector(g_ui_mgr.handles.screen_l2_widget_selector);
    
    g_ui_mgr.l2_widget_selector_built = true;
    rt_kprintf("[UI] L2 Widget Selector built\n");
    
    return 0;
}

int screen_ui_build_l2_stopwatch(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_stopwatch_built) {
        return 0;
    }
    
    /* 调用新模块构建秒表L2页面 */
    group4_build_l2_stopwatch_page(g_ui_mgr.handles.screen_l2_stopwatch);
    
    /* 标记已构建 */
    g_ui_mgr.l2_stopwatch_built = true;
    
    /* 初始更新显示 */
    screen_ui_update_stopwatch_display();
    
    return 0;
}

int screen_ui_build_l2_weather(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_weather_built) {
        return 0;
    }
    
    build_l2_weather_forecast_page(g_ui_mgr.handles.screen_l2_weather_forecast);
    
    g_ui_mgr.l2_weather_built = true;
    
    weather_forecast_data_t forecast = {0};
    if (data_manager_get_forecast(&forecast) == 0 && forecast.valid) {
        screen_ui_update_l2_weather_forecast(&forecast);
    }
    
    return 0;
}

int screen_ui_build_l2_media(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_media_built) {
        return 0;
    }
    
    build_l2_media_control_page(g_ui_mgr.handles.screen_l2_media);
    
    g_ui_mgr.l2_media_built = true;
    
    return 0;
}

int screen_ui_build_l2_web(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_web_built) {
        return 0;
    }
    
    build_l2_web_control_page(g_ui_mgr.handles.screen_l2_web);
    
    g_ui_mgr.l2_web_built = true;
    
    return 0;
}

int screen_ui_build_l2_shortcut(void)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.l2_shortcut_built) {
        return 0;
    }
    
    build_l2_shortcut_control_page(g_ui_mgr.handles.screen_l2_shortcut);
    
    g_ui_mgr.l2_shortcut_built = true;
    
    return 0;
}


int screen_ui_switch_to_group(screen_group_t target_group)
{
    if (!g_ui_mgr.initialized || target_group >= SCREEN_GROUP_MAX) {
        return -RT_ERROR;
    }
    
    lv_obj_t *target_screen = NULL;
    
    switch (target_group) {
        case SCREEN_GROUP_1:
            screen_ui_build_group1();
            target_screen = g_ui_mgr.handles.screen_group1;
            break;
            
        case SCREEN_GROUP_2:
            screen_ui_build_group2();
            target_screen = g_ui_mgr.handles.screen_group2;
            break;
            
        case SCREEN_GROUP_3:
            screen_ui_build_group3();
            target_screen = g_ui_mgr.handles.screen_group3;
            break;
            
        case SCREEN_GROUP_4:
            screen_ui_build_group4();
            target_screen = g_ui_mgr.handles.screen_group4;
            break;
        
        case SCREEN_GROUP_5:
            screen_ui_build_group5();
            target_screen = g_ui_mgr.handles.screen_group5;
            break;

        case SCREEN_GROUP_6:
        {
            if (g_ui_mgr.handles.screen_group6_mp3 == NULL) {
                g_ui_mgr.handles.screen_group6_mp3 = lv_obj_create(NULL);
                if (g_ui_mgr.handles.screen_group6_mp3) {
                    setup_screen_base_style(g_ui_mgr.handles.screen_group6_mp3);
                }
            }
            
            if (g_ui_mgr.handles.screen_group6_mp3) {
                if (!mp3_screen_ui_is_built()) {
                    mp3_screen_ui_build(g_ui_mgr.handles.screen_group6_mp3);
                }
                g_ui_mgr.group6_built = true;
                mp3_screen_ui_update();
                target_screen = g_ui_mgr.handles.screen_group6_mp3;
            } else {
                return -RT_ERROR;
            }
        }
        break;
        break;

        default:
            return -RT_EINVAL;
    }
    
    if (!target_screen) {
        return -RT_ERROR;
    }
    
    #if USE_SCREEN_ANIMATIONS
    lv_scr_load_anim(target_screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
    #else
    lv_scr_load(target_screen);
    #endif
    
    g_ui_mgr.current_group = target_group;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    screen_context_activate_for_group(target_group);
    
    return 0;
}

int screen_ui_switch_to_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    lv_obj_t *target_screen = NULL;
    
    switch (l2_group) {
        case SCREEN_L2_TIME_GROUP:
            screen_ui_build_l2_time();
            target_screen = g_ui_mgr.handles.screen_l2_time_detail;
            break;
            
        case SCREEN_L2_WEATHER_GROUP:
            screen_ui_build_l2_weather();
            target_screen = g_ui_mgr.handles.screen_l2_weather_forecast;
            break;
            
        case SCREEN_L2_MUYU_GROUP:
            screen_ui_build_l2_muyu();
            target_screen = g_ui_mgr.handles.screen_l2_muyu;
            break;
            
        case SCREEN_L2_TOMATO_GROUP:
            screen_ui_build_l2_tomato();
            target_screen = g_ui_mgr.handles.screen_l2_tomato;
            break;
            
        case SCREEN_L2_STOPWATCH_GROUP:
            screen_ui_build_l2_stopwatch();
            target_screen = g_ui_mgr.handles.screen_l2_stopwatch;
            break;
            
        case SCREEN_L2_MEDIA_GROUP:
            screen_ui_build_l2_media();
            target_screen = g_ui_mgr.handles.screen_l2_media;
            break;
            
        case SCREEN_L2_WEB_GROUP:
            screen_ui_build_l2_web();
            target_screen = g_ui_mgr.handles.screen_l2_web;
            break;
            
        case SCREEN_L2_SHORTCUT_GROUP:
            screen_ui_build_l2_shortcut();
            target_screen = g_ui_mgr.handles.screen_l2_shortcut;
            break;

        case SCREEN_L2_WIDGET_SELECTOR_GROUP:
            if (!g_ui_mgr.l2_widget_selector_built) {
                screen_ui_build_l2_widget_selector();
            }
            target_screen = g_ui_mgr.handles.screen_l2_widget_selector;
            break;

        default:
            return -RT_EINVAL;
    }
    
    if (!target_screen) {
        return -RT_ERROR;
    }
    
    load_screen_with_anim(target_screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200);
    
    g_ui_mgr.current_level = SCREEN_LEVEL_2;
    g_ui_mgr.current_l2_group = l2_group;
    
    screen_context_activate_for_level2(l2_group);
    
    return 0;
}

int screen_ui_return_to_l1(screen_group_t l1_group)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    lv_obj_t *target_screen = NULL;
    
    switch (l1_group) {
        case SCREEN_GROUP_1:
            target_screen = g_ui_mgr.handles.screen_group1;
            break;
        case SCREEN_GROUP_2:
            target_screen = g_ui_mgr.handles.screen_group2;
            break;
        case SCREEN_GROUP_3:
            target_screen = g_ui_mgr.handles.screen_group3;
            break;
        case SCREEN_GROUP_4:
            target_screen = g_ui_mgr.handles.screen_group4;
            break;
        default:
            return -RT_EINVAL;
    }
    
    if (!target_screen) {
        return -RT_ERROR;
    }
    
    load_screen_with_anim(target_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200);
    g_ui_mgr.current_group = l1_group;
    g_ui_mgr.current_level = SCREEN_LEVEL_1;
    
    screen_context_activate_for_group(l1_group);
    screen_context_deactivate_level2();
    
    return 0;
}

/*********************
 *   UI UPDATE API
 *********************/

int screen_ui_update_time_display(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    if (g_tomato_background_display.enabled) {
        extern void tomato_process_countdown(void);
        tomato_process_countdown();
    }    
    if (g_ui_mgr.current_level == SCREEN_LEVEL_1) {
        /* L1层级：只更新L1主页时间 */
        if (g_ui_mgr.current_group != SCREEN_GROUP_1) {
            return 0;
        }
        
        int ret;
        if (is_showing_tomato_mode()) {
            ret = screen_ui_update_time_display_tomato_mode();
        } else {
            ret = screen_ui_update_time_display_normal_mode();
        }
        
        /* 更新小工具面板 - 修复：确保每次时间更新时也更新小工具面板 */
        widget_ui_update_panel();
        
        /* 如果是下班倒计时，每秒更新一次 */
        widget_timer_tick();
        
        return ret;
    }
    
    /* L2层级：根据当前L2组判断更新哪个显示 */
    if (g_ui_mgr.current_level == SCREEN_LEVEL_2) {
        switch (g_ui_mgr.current_l2_group) {
            case SCREEN_L2_MUYU_GROUP:
                return screen_ui_update_muyu_display();
                
            case SCREEN_L2_TIME_GROUP:
                return screen_ui_update_l2_digital_clock();
                
            case SCREEN_L2_TOMATO_GROUP:
                return screen_ui_update_tomato_display();
                
            case SCREEN_L2_STOPWATCH_GROUP:
                return screen_ui_update_stopwatch_display();
                
            default:
                break;
        }
    }
    
    /* 如果在小工具选择器L2页面，更新高亮显示 */
    if (g_ui_mgr.current_level == SCREEN_LEVEL_2 &&
        g_ui_mgr.current_l2_group == SCREEN_L2_WIDGET_SELECTOR_GROUP) {
        widget_ui_update_selector();
    }
    return 0;
}

int screen_ui_update_weather_display(const weather_data_t *data)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.current_group != SCREEN_GROUP_1 || !data || !data->valid) {
        return 0;
    }

    if (g_ui_mgr.handles.group1_weather.city_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.city_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.city_label, data->city);
    }

    if (g_ui_mgr.handles.group1_weather.temperature_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.temperature_label)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->temperature);
        lv_label_set_text(g_ui_mgr.handles.group1_weather.temperature_label, temp_str);
    }

    if (g_ui_mgr.handles.group1_weather.weather_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.weather_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_weather.weather_label, data->weather);
    }
    if (g_ui_mgr.handles.group1_weather.weather_icon && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.weather_icon)) {
        const lv_image_dsc_t* weather_icon = get_weather_icon_by_code(data->weather_code);
        lv_img_set_src(g_ui_mgr.handles.group1_weather.weather_icon, weather_icon);
    }
    if (g_ui_mgr.handles.group1_weather.humidity_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.humidity_label)) {
        char humidity_str[16];
        rt_snprintf(humidity_str, sizeof(humidity_str), "%.0f%%", data->humidity);
        lv_label_set_text(g_ui_mgr.handles.group1_weather.humidity_label, humidity_str);
    }

    if (g_ui_mgr.handles.group1_weather.pressure_label && lv_obj_is_valid(g_ui_mgr.handles.group1_weather.pressure_label)) {
        char pressure_str[16];
        rt_snprintf(pressure_str, sizeof(pressure_str), "%dhPa", data->pressure);
        lv_label_set_text(g_ui_mgr.handles.group1_weather.pressure_label, pressure_str);
    }

    return 0;
}

/* Usage color: 0-50% green, 50-80% yellow, 80-100% red */
static lv_color_t usage_color(float pct)
{
    if (pct >= 80.0f) return lv_color_make(255, 60, 60);
    if (pct >= 50.0f) return lv_color_make(255, 215, 0);
    return lv_color_make(100, 255, 150);
}

/* Temp color: <80 light blue, >=80 light red */
static lv_color_t temp_color(float deg)
{
    if (deg >= 80.0f) return lv_color_make(255, 100, 100);
    return lv_color_make(100, 180, 255);
}

int screen_ui_update_system_display(const system_monitor_data_t *data)
{
    if (!g_ui_mgr.initialized || g_ui_mgr.current_group != SCREEN_GROUP_2 || !data || !data->valid) {
        return 0;
    }

    /* CPU温度 - 动态颜色 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_temp && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->cpu_temp);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, temp_str);
        lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, temp_color(data->cpu_temp), 0);
    }

    /* CPU仪表 - 圆环 + 占用率文字动态颜色 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge)) {
        lv_color_t cpu_clr = usage_color(data->cpu_usage);
        lv_arc_set_value(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge, (int16_t)data->cpu_usage);
        lv_obj_set_style_arc_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge, cpu_clr, LV_PART_INDICATOR);
        
        if (g_ui_mgr.handles.group2_cpu_gpu.cpu_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage)) {
            char usage_str[16];
            rt_snprintf(usage_str, sizeof(usage_str), "%.0f%%", data->cpu_usage);
            lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, usage_str);
            lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, cpu_clr, 0);
        }
    }

    /* CPU频率 */
    if (g_ui_mgr.handles.group2_cpu_gpu.cpu_freq && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.cpu_freq)) {
        char freq_str[16];
        if (data->cpu_freq > 0) {
            rt_snprintf(freq_str, sizeof(freq_str), "%.0fMHz", data->cpu_freq);
        } else {
            rt_snprintf(freq_str, sizeof(freq_str), "-- MHz");
        }
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_freq, freq_str);
    }

    /* GPU温度 - 动态颜色 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_temp && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp)) {
        char temp_str[16];
        rt_snprintf(temp_str, sizeof(temp_str), "%.1f°C", data->gpu_temp);
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, temp_str);
        lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, temp_color(data->gpu_temp), 0);
    }

    /* GPU仪表 - 圆环 + 占用率文字动态颜色 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge)) {
        lv_color_t gpu_clr = usage_color(data->gpu_usage);
        lv_arc_set_value(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge, (int16_t)data->gpu_usage);
        lv_obj_set_style_arc_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge, gpu_clr, LV_PART_INDICATOR);
        
        if (g_ui_mgr.handles.group2_cpu_gpu.gpu_usage && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage)) {
            char usage_str[16];
            rt_snprintf(usage_str, sizeof(usage_str), "%.0f%%", data->gpu_usage);
            lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, usage_str);
            lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, gpu_clr, 0);
        }
    }

    /* GPU显存 */
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used)) {
        char buf[16];
        if (data->gpu_mem_used > 0) {
            rt_snprintf(buf, sizeof(buf), "%.1fG", data->gpu_mem_used);
        } else {
            rt_snprintf(buf, sizeof(buf), "-- GB");
        }
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used, buf);
    }
    if (g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total && lv_obj_is_valid(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total)) {
        char buf[16];
        if (data->gpu_mem_total > 0) {
            rt_snprintf(buf, sizeof(buf), "%.1fG", data->gpu_mem_total);
        } else {
            rt_snprintf(buf, sizeof(buf), "-- GB");
        }
        lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total, buf);
    }

    /* 内存进度条 - 动态颜色 */
    if (g_ui_mgr.handles.group2_memory.ram_usage && 
        lv_obj_is_valid(g_ui_mgr.handles.group2_memory.ram_usage)) {
        lv_bar_set_value(g_ui_mgr.handles.group2_memory.ram_usage,
                         (int32_t)data->ram_usage, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(g_ui_mgr.handles.group2_memory.ram_usage,
                                  usage_color(data->ram_usage), LV_PART_INDICATOR);

        if (g_ui_mgr.handles.group2_memory.ram_used &&
            lv_obj_is_valid(g_ui_mgr.handles.group2_memory.ram_used)) {
            char buf[16];
            if (data->ram_used > 0) {
                rt_snprintf(buf, sizeof(buf), "%.1f GB", data->ram_used);
            } else {
                rt_snprintf(buf, sizeof(buf), "-- GB");
            }
            lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_used, buf);
        }

        if (g_ui_mgr.handles.group2_memory.ram_total &&
            lv_obj_is_valid(g_ui_mgr.handles.group2_memory.ram_total)) {
            char buf[16];
            if (data->ram_total > 0) {
                rt_snprintf(buf, sizeof(buf), "%.1f GB", data->ram_total);
            } else {
                rt_snprintf(buf, sizeof(buf), "-- GB");
            }
            lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_total, buf);
        }
    }
    
    if (g_ui_mgr.handles.group2_network.net_upload && 
        lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_upload)) {
        char upload_str[32];
        rt_snprintf(upload_str, sizeof(upload_str), "%.2fM/s", data->net_upload_speed);
        lv_label_set_text(g_ui_mgr.handles.group2_network.net_upload, upload_str);
    }
    
    if (g_ui_mgr.handles.group2_network.net_download && 
        lv_obj_is_valid(g_ui_mgr.handles.group2_network.net_download)) {
        char download_str[32];
        rt_snprintf(download_str, sizeof(download_str), "%.2fM/s", data->net_download_speed);
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

int screen_ui_cleanup_all(void)
{
    if (!g_ui_mgr.initialized) {
        return 0;
    }
    
    if (g_ui_mgr.handles.screen_group1) {
        lv_obj_del(g_ui_mgr.handles.screen_group1);
        g_ui_mgr.handles.screen_group1 = NULL;
    }
    if (g_ui_mgr.handles.screen_group2) {
        lv_obj_del(g_ui_mgr.handles.screen_group2);
        g_ui_mgr.handles.screen_group2 = NULL;
    }
    if (g_ui_mgr.handles.screen_group3) {
        lv_obj_del(g_ui_mgr.handles.screen_group3);
        g_ui_mgr.handles.screen_group3 = NULL;
    }
    if (g_ui_mgr.handles.screen_group4) {
        lv_obj_del(g_ui_mgr.handles.screen_group4);
        g_ui_mgr.handles.screen_group4 = NULL;
    }
    if (g_ui_mgr.handles.screen_group5) {
        lv_obj_del(g_ui_mgr.handles.screen_group5);
        g_ui_mgr.handles.screen_group5 = NULL;
    }
    if (g_ui_mgr.handles.screen_group6_mp3) {
        mp3_screen_ui_cleanup();
        lv_obj_del(g_ui_mgr.handles.screen_group6_mp3);
        g_ui_mgr.handles.screen_group6_mp3 = NULL;
    }

    if (g_ui_mgr.handles.screen_l2_time_detail) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_time_detail);
        g_ui_mgr.handles.screen_l2_time_detail = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_weather_forecast) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_weather_forecast);
        g_ui_mgr.handles.screen_l2_weather_forecast = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_muyu) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_muyu);
        g_ui_mgr.handles.screen_l2_muyu = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_tomato) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_tomato);
        g_ui_mgr.handles.screen_l2_tomato = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_stopwatch) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_stopwatch);
        g_ui_mgr.handles.screen_l2_stopwatch = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_media) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_media);
        g_ui_mgr.handles.screen_l2_media = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_web) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_web);
        g_ui_mgr.handles.screen_l2_web = NULL;
    }
    if (g_ui_mgr.handles.screen_l2_shortcut) {
        lv_obj_del(g_ui_mgr.handles.screen_l2_shortcut);
        g_ui_mgr.handles.screen_l2_shortcut = NULL;
    }
    
    cleanup_fonts();
    
    memset(&g_ui_mgr, 0, sizeof(g_ui_mgr));
    
    return 0;
}

int screen_ui_manager_deinit(void)
{
    widget_ui_cleanup();
    return screen_ui_cleanup_all();
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
 *   GROUP 4 UI BUILD
 *   (已移至 screen_ui_group4.c)
 *********************/

/* L1 面板包装函数 */
static void build_left_muyu_panel(lv_obj_t *parent)
{
    group4_build_left_muyu_panel(parent);
}

static void build_middle_tomato_panel(lv_obj_t *parent)
{
    group4_build_middle_tomato_panel(parent);
}

static void build_right_stopwatch_panel(lv_obj_t *parent)
{
    group4_build_right_stopwatch_panel(parent);
}

/* L2 页面包装函数 */
static void build_l2_muyu_main_page(lv_obj_t *screen)
{
    group4_build_l2_muyu_main_page(screen);
}

/* 更新函数 - 直接调用新模块 */
int screen_ui_update_muyu_display(void)
{
    return group4_update_muyu_display();
}

int screen_ui_update_tomato_display(void)
{
    return group4_update_tomato_display();
}

int screen_ui_update_stopwatch_display(void)
{
    return group4_update_stopwatch_display();
}

const muyu_data_t* screen_ui_get_muyu_data(void)
{
    return &g_ui_mgr.muyu_data;
}

int screen_ui_reset_muyu_counter(void)
{
    return group4_reset_muyu_counter();
}


/* 番茄钟后台显示切换定时器回调 */
static void tomato_background_switch_callback(void *parameter)
{
    (void)parameter;
    
    // 切换显示模式
    g_tomato_background_display.show_tomato = !g_tomato_background_display.show_tomato;
    g_tomato_background_display.last_switch_tick = rt_tick_get();
    
    // 触发时间显示更新
    screen_core_post_update_time();
}

/**
 * @brief 检查是否正在显示番茄钟模式
 */
static bool is_showing_tomato_mode(void)
{
    return g_tomato_background_display.enabled && 
           g_tomato_background_display.show_tomato;
}

/**
 * @brief 正常时间显示模式
 */
static int screen_ui_update_time_display_normal_mode(void)
{
    time_t now = time(NULL);
    if (now == (time_t)-1) return -1;
    
    struct tm *tm_info = localtime(&now);
    if (!tm_info) return -1;

    /* 更新年份 */
    if (g_ui_mgr.handles.group1_time.year_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.year_label)) {
        char year_str[16];
        rt_snprintf(year_str, sizeof(year_str), "%d年", tm_info->tm_year + 1900);
        lv_label_set_text(g_ui_mgr.handles.group1_time.year_label, year_str);
    }

    /* 更新时间 */
    if (g_ui_mgr.handles.group1_time.time_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.time_label)) {
        char time_str[16];
        rt_snprintf(time_str, sizeof(time_str), "%02d:%02d", 
                   tm_info->tm_hour, tm_info->tm_min);
        lv_label_set_text(g_ui_mgr.handles.group1_time.time_label, time_str);
    }

    /* 更新中文日期 */
    if (g_ui_mgr.handles.group1_time.date_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.date_label)) {
        char date_str[32];
        rt_snprintf(date_str, sizeof(date_str), "%s%d日", 
                   chinese_months[tm_info->tm_mon], tm_info->tm_mday);
        lv_label_set_text(g_ui_mgr.handles.group1_time.date_label, date_str);
    }

    /* 更新中文星期 */
    if (g_ui_mgr.handles.group1_time.weekday_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.weekday_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_time.weekday_label, 
                         chinese_weekdays[tm_info->tm_wday]);
    }

    return 0;
}

/**
 * @brief 番茄钟显示模式
 */
static int screen_ui_update_time_display_tomato_mode(void)
{
    extern void tomato_process_countdown(void);
    tomato_process_countdown();
    // 获取番茄钟数据
    tomato_data_t tomato_data;
    if (screen_context_get_tomato_data(&tomato_data) != 0) {
        return screen_ui_update_time_display_normal_mode();
    }
    
    /* 第一行: "番茄钟" */
    if (g_ui_mgr.handles.group1_time.year_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.year_label)) {
        lv_label_set_text(g_ui_mgr.handles.group1_time.year_label, "番茄钟");
    }
    
    /* 第二行: 模式 */
    if (g_ui_mgr.handles.group1_time.date_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.date_label)) {
        const char *mode_text;
        switch (tomato_data.current_mode) {
            case TOMATO_MODE_FOCUS:
                mode_text = "专注中";
                break;
            case TOMATO_MODE_SHORT_BREAK:
                mode_text = "短休息";
                break;
            case TOMATO_MODE_LONG_BREAK:
                mode_text = "长休息";
                break;
            default:
                mode_text = "运行中";
                break;
        }
        lv_label_set_text(g_ui_mgr.handles.group1_time.date_label, mode_text);
    }
    
    /* 第三行: 轮次 */
    if (g_ui_mgr.handles.group1_time.weekday_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.weekday_label)) {
        char round_str[16];
        if (tomato_data.current_mode == TOMATO_MODE_FOCUS) {
            rt_snprintf(round_str, sizeof(round_str), "%d/%d轮", 
                       tomato_data.current_round + 1, 
                       tomato_data.long_break_interval);
        } else {
            rt_snprintf(round_str, sizeof(round_str), "休息中");
        }
        lv_label_set_text(g_ui_mgr.handles.group1_time.weekday_label, round_str);
    }
    
    /* 第四行(大字): 剩余时间 */
    if (g_ui_mgr.handles.group1_time.time_label && 
        lv_obj_is_valid(g_ui_mgr.handles.group1_time.time_label)) {
        uint16_t minutes = tomato_data.remaining_seconds / 60;
        uint16_t seconds = tomato_data.remaining_seconds % 60;
        
        char time_str[16];
        rt_snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
        lv_label_set_text(g_ui_mgr.handles.group1_time.time_label, time_str);
    }
    
    return 0;
}

/**
 * @brief 启动番茄钟后台显示轮换
 */
int screen_ui_start_tomato_background_display(void)
{
    if (!g_ui_mgr.initialized) {
        return -RT_ERROR;
    }
    
    // 如果已经启动,直接返回
    if (g_tomato_background_display.enabled) {
        return 0;
    }
    
    // 创建切换定时器(5秒)
    if (!g_tomato_background_display.switch_timer) {
        g_tomato_background_display.switch_timer = rt_timer_create(
            "tomato_bg",
            tomato_background_switch_callback,
            RT_NULL,
            rt_tick_from_millisecond(5000),
            RT_TIMER_FLAG_PERIODIC
        );
    }
    
    if (g_tomato_background_display.switch_timer) {
        g_tomato_background_display.enabled = true;
        g_tomato_background_display.show_tomato = false;  // 从正常时间开始
        g_tomato_background_display.last_switch_tick = rt_tick_get();
        
        rt_timer_start(g_tomato_background_display.switch_timer);
        return 0;
    }
    
    return -RT_ERROR;
}

/**
 * @brief 停止番茄钟后台显示轮换
 */
int screen_ui_stop_tomato_background_display(void)
{
    if (!g_tomato_background_display.enabled) {
        return 0;
    }
    
    if (g_tomato_background_display.switch_timer) {
        rt_timer_stop(g_tomato_background_display.switch_timer);
    }
    
    g_tomato_background_display.enabled = false;
    g_tomato_background_display.show_tomato = false;
    
    // 恢复正常时间显示
    screen_core_post_update_time();
    
    return 0;
}