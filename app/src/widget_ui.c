/**
 * @file widget_ui.c
 * @brief 小工具UI显示模块
 * 
 * 负责在Group1第三板块显示当前激活的小工具界面
 * 以及L2小工具选择器界面
 */

#include "widget_manager.h"
#include "widget_storage.h"
#include "lvgl.h"
#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * 外部资源声明
 * ============================================================================ */

/* 字体声明 (需要与screen_ui_manager.c中的字体保持一致) */
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;

/* ============================================================================
 * 屏幕尺寸定义
 * ============================================================================ */

#define WIDGET_PANEL_WIDTH   128
#define WIDGET_PANEL_HEIGHT  128

/* ============================================================================
 * UI句柄
 * ============================================================================ */

typedef struct {
    /* Group1第三板块小工具UI */
    lv_obj_t *widget_panel;        /* 面板容器 */
    lv_obj_t *title_label;         /* 标题标签 */
    lv_obj_t *main_value_label;    /* 主显示值 */
    lv_obj_t *sub_info_label;      /* 副信息标签 */
    lv_obj_t *hint_label;          /* 提示标签 */
    
    /* L2选择器UI */
    lv_obj_t *selector_panel;
    lv_obj_t *selector_items[3];   /* 三个选择项 */
    lv_obj_t *selector_labels[3];  /* 选择项标签 */
    lv_obj_t *selector_hint;       /* 选择提示 */
    
    /* 字体 */
    lv_font_t *font_small;
    lv_font_t *font_medium;
    lv_font_t *font_large;
    lv_font_t *font_xlarge;
    
    bool initialized;
} widget_ui_t;

static widget_ui_t g_widget_ui = {0};

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 创建TTF字体
 */
static lv_font_t* create_widget_font(int size)
{
    /* 这里需要使用项目中已有的字体创建方式 */
    /* 假设使用lv_tiny_ttf */
    return lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, size);
}

/**
 * @brief 格式化倒计时时间
 */
static void format_countdown_time(int32_t seconds, char *buf, size_t buf_size)
{
    if (seconds <= 0) {
        snprintf(buf, buf_size, "00:00:00");
        return;
    }
    
    int hours = seconds / 3600;
    int mins = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    snprintf(buf, buf_size, "%02d:%02d:%02d", hours, mins, secs);
}

/* ============================================================================
 * Group1第三板块 - 小工具显示
 * ============================================================================ */

/**
 * @brief 构建小工具面板UI
 * @param parent 父容器(Group1的第三个panel)
 */
int widget_ui_build_panel(lv_obj_t *parent)
{
    if (!parent) return -1;
    
    g_widget_ui.widget_panel = parent;
    
    /* 清理现有内容 */
    lv_obj_clean(parent);
    
    /* 设置面板样式 */
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    
    /* 创建字体 */
    if (!g_widget_ui.font_small) {
        g_widget_ui.font_small = create_widget_font(12);
    }
    if (!g_widget_ui.font_medium) {
        g_widget_ui.font_medium = create_widget_font(16);
    }
    if (!g_widget_ui.font_large) {
        g_widget_ui.font_large = create_widget_font(24);
    }
    if (!g_widget_ui.font_xlarge) {
        g_widget_ui.font_xlarge = create_widget_font(36);
    }
    
    /* 标题标签 - 顶部居中 */
    g_widget_ui.title_label = lv_label_create(parent);
    lv_obj_set_style_text_color(g_widget_ui.title_label, lv_color_make(0x00, 0xBF, 0xFF), 0);
    if (g_widget_ui.font_medium) {
        lv_obj_set_style_text_font(g_widget_ui.title_label, g_widget_ui.font_medium, 0);
    }
    lv_obj_align(g_widget_ui.title_label, LV_ALIGN_TOP_MID, 0, 8);
    lv_label_set_text(g_widget_ui.title_label, "随机数");
    
    /* 主显示值 - 中央大字 */
    g_widget_ui.main_value_label = lv_label_create(parent);
    lv_obj_set_style_text_color(g_widget_ui.main_value_label, lv_color_white(), 0);
    if (g_widget_ui.font_xlarge) {
        lv_obj_set_style_text_font(g_widget_ui.main_value_label, g_widget_ui.font_xlarge, 0);
    }
    lv_obj_align(g_widget_ui.main_value_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(g_widget_ui.main_value_label, "--");
    
    /* 副信息标签 - 显示最大值/目标时间等 */
    g_widget_ui.sub_info_label = lv_label_create(parent);
    lv_obj_set_style_text_color(g_widget_ui.sub_info_label, lv_color_make(0x80, 0x80, 0x80), 0);
    if (g_widget_ui.font_small) {
        lv_obj_set_style_text_font(g_widget_ui.sub_info_label, g_widget_ui.font_small, 0);
    }
    lv_obj_align(g_widget_ui.sub_info_label, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(g_widget_ui.sub_info_label, "");
    
    /* 提示标签 - 底部 */
    g_widget_ui.hint_label = lv_label_create(parent);
    lv_obj_set_style_text_color(g_widget_ui.hint_label, lv_color_make(0x60, 0x60, 0x60), 0);
    if (g_widget_ui.font_small) {
        lv_obj_set_style_text_font(g_widget_ui.hint_label, g_widget_ui.font_small, 0);
    }
    lv_obj_align(g_widget_ui.hint_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_label_set_text(g_widget_ui.hint_label, "长按选择工具");
    
    g_widget_ui.initialized = true;
    
    return 0;
}

/**
 * @brief 更新随机数工具显示
 */
static void update_random_display(void)
{
    widget_random_state_t state;
    widget_random_get_state(&state);
    
    /* 标题 */
    lv_label_set_text(g_widget_ui.title_label, "随机数");
    lv_obj_set_style_text_color(g_widget_ui.title_label, 
                                 lv_color_make(0x00, 0xBF, 0xFF), 0);
    
    /* 主显示值 */
    if (state.in_setting_mode) {
        /* 设置模式: 显示最大值(闪烁) */
        int max_val = widget_get_random_max_value(state.max_option);
        char buf[16];
        
        if (state.setting_flash_on) {
            snprintf(buf, sizeof(buf), "<%d>", max_val);
            lv_obj_set_style_text_color(g_widget_ui.main_value_label, 
                                         lv_color_make(0xFF, 0xFF, 0x00), 0);
        } else {
            snprintf(buf, sizeof(buf), " %d ", max_val);
            lv_obj_set_style_text_color(g_widget_ui.main_value_label, 
                                         lv_color_make(0x80, 0x80, 0x00), 0);
        }
        lv_label_set_text(g_widget_ui.main_value_label, buf);
        
        lv_label_set_text(g_widget_ui.sub_info_label, "旋钮调节范围");
        lv_label_set_text(g_widget_ui.hint_label, "点击确认");
    } else {
        /* 正常模式: 显示随机数结果 */
        char buf[16];
        if (state.current_value > 0) {
            snprintf(buf, sizeof(buf), "%d", state.current_value);
        } else {
            snprintf(buf, sizeof(buf), "--");
        }
        lv_label_set_text(g_widget_ui.main_value_label, buf);
        lv_obj_set_style_text_color(g_widget_ui.main_value_label, lv_color_white(), 0);
        
        /* 显示当前范围 */
        int max_val = widget_get_random_max_value(state.max_option);
        snprintf(buf, sizeof(buf), "1~%d", max_val);
        lv_label_set_text(g_widget_ui.sub_info_label, buf);
        
        lv_label_set_text(g_widget_ui.hint_label, "双击设置");
    }
}

/**
 * @brief 更新下班倒计时显示
 */
static void update_offwork_display(void)
{
    widget_offwork_state_t state;
    widget_offwork_get_state(&state);
    
    /* 标题 */
    lv_label_set_text(g_widget_ui.title_label, "下班倒计时");
    /* 设置字体 */
    if (g_widget_ui.font_xlarge) {
        lv_obj_set_style_text_font(g_widget_ui.main_value_label, 
                                    g_widget_ui.font_large, 0);
    }    
    if (state.in_setting_mode) {
        /* 设置模式 */
        char buf[32];
        if (state.setting_step == 0) {
            /* 设置小时 */
            snprintf(buf, sizeof(buf), "[%02d]:%02d", state.temp_hour, state.temp_minute);
            lv_obj_set_style_text_color(g_widget_ui.title_label, 
                                         lv_color_make(0xFF, 0xFF, 0x00), 0);
        } else {
            /* 设置分钟 */
            snprintf(buf, sizeof(buf), "%02d:[%02d]", state.temp_hour, state.temp_minute);
            lv_obj_set_style_text_color(g_widget_ui.title_label, 
                                         lv_color_make(0x00, 0xFF, 0x00), 0);
        }
        lv_label_set_text(g_widget_ui.main_value_label, buf);
        lv_obj_set_style_text_color(g_widget_ui.main_value_label, 
                                     lv_color_make(0xFF, 0xFF, 0x00), 0);
        
        lv_label_set_text(g_widget_ui.sub_info_label, "旋钮调节时间");
        lv_label_set_text(g_widget_ui.hint_label, "点击下一步");
    } else {
        /* 正常模式 */
        lv_obj_set_style_text_color(g_widget_ui.title_label, 
                                     lv_color_make(0x00, 0xFF, 0x00), 0);
        
        if (state.is_off_work) {
            /* 已下班 */
            lv_label_set_text(g_widget_ui.main_value_label, "已下班");
            lv_obj_set_style_text_color(g_widget_ui.main_value_label, 
                                         lv_color_make(0x00, 0xFF, 0x00), 0);
            lv_label_set_text(g_widget_ui.sub_info_label, "恭喜！");
        } else {
            /* 倒计时中 */
            char buf[16];
            format_countdown_time(state.remaining_seconds, buf, sizeof(buf));
            lv_label_set_text(g_widget_ui.main_value_label, buf);
            lv_obj_set_style_text_color(g_widget_ui.main_value_label, lv_color_white(), 0);
            
            snprintf(buf, sizeof(buf), "目标: %02d:%02d", state.target_hour, state.target_minute);
            lv_label_set_text(g_widget_ui.sub_info_label, buf);
        }
        
        lv_label_set_text(g_widget_ui.hint_label, "双击设置");
    }
}

/**
 * @brief 更新今天吃什么显示
 */
static void update_food_display(void)
{
    widget_food_state_t state;
    widget_food_get_state(&state);
    
    /* 标题 */
    lv_label_set_text(g_widget_ui.title_label, "今天吃什么");
    lv_obj_set_style_text_color(g_widget_ui.title_label, 
                                 lv_color_make(0xFF, 0x80, 0x00), 0);
    
    /* 主显示值 - 菜名 */
    lv_label_set_text(g_widget_ui.main_value_label, state.current_food);
    lv_obj_set_style_text_color(g_widget_ui.main_value_label, lv_color_white(), 0);
    
    /* 如果菜名较长，使用较小字体 */
    size_t len = strlen(state.current_food);
    if (len > 5 && g_widget_ui.font_large) {
        lv_obj_set_style_text_font(g_widget_ui.main_value_label, 
                                    g_widget_ui.font_large, 0);
    } else if (g_widget_ui.font_xlarge) {
        lv_obj_set_style_text_font(g_widget_ui.main_value_label, 
                                    g_widget_ui.font_xlarge, 0);
    }
    
    /* 副信息 */
    if (state.has_result) {
        lv_label_set_text(g_widget_ui.sub_info_label, "就决定是你了!");
    } else {
        lv_label_set_text(g_widget_ui.sub_info_label, "");
    }
    
    lv_label_set_text(g_widget_ui.hint_label, "点击随机");
}

/**
 * @brief 更新小工具面板显示
 */
int widget_ui_update_panel(void)
{
    if (!g_widget_ui.initialized || !g_widget_ui.widget_panel) {
        return -1;
    }
    
    widget_type_t active = widget_manager_get_active_type();
    
    switch (active) {
        case WIDGET_TYPE_RANDOM_NUMBER:
            update_random_display();
            break;
            
        case WIDGET_TYPE_OFF_WORK_COUNTDOWN:
            update_offwork_display();
            break;
            
        case WIDGET_TYPE_WHAT_TO_EAT:
            update_food_display();
            break;
            
        default:
            lv_label_set_text(g_widget_ui.title_label, "小工具");
            lv_label_set_text(g_widget_ui.main_value_label, "未选择");
            lv_label_set_text(g_widget_ui.sub_info_label, "");
            lv_label_set_text(g_widget_ui.hint_label, "长按选择");
            break;
    }
    
    return 0;
}

/* ============================================================================
 * L2小工具选择器UI
 * ============================================================================ */

/**
 * @brief 构建L2小工具选择器页面
 * @param screen L2屏幕对象
 */
int widget_ui_build_selector(lv_obj_t *screen)
{
    if (!screen) return -1;
    
    g_widget_ui.selector_panel = screen;
    
    /* 设置背景 */
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    
    /* 顶部标题 */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "选择小工具");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    if (g_widget_ui.font_medium) {
        lv_obj_set_style_text_font(title, g_widget_ui.font_medium, 0);
    }
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    /* 三个选择项 - 水平排列 */
    static const char* widget_names[] = {"随机数", "下班倒计时", "今天吃什么"};
    static const uint32_t widget_colors[] = {0x00BFFF, 0x00FF00, 0xFF8000};
    
    int total_width = WIDGET_PANEL_WIDTH * 3;
    int item_width = 100;
    int item_height = 70;
    int spacing = (total_width - item_width * 3) / 4;
    
    for (int i = 0; i < 3; i++) {
        /* 创建选择项容器 */
        g_widget_ui.selector_items[i] = lv_obj_create(screen);
        lv_obj_remove_style_all(g_widget_ui.selector_items[i]);
        lv_obj_set_size(g_widget_ui.selector_items[i], item_width, item_height);
        
        int x_pos = spacing + i * (item_width + spacing);
        lv_obj_set_pos(g_widget_ui.selector_items[i], x_pos, 40);
        
        /* 设置边框和背景 */
        lv_obj_set_style_border_width(g_widget_ui.selector_items[i], 2, 0);
        lv_obj_set_style_border_color(g_widget_ui.selector_items[i], 
                                       lv_color_hex(widget_colors[i]), 0);
        lv_obj_set_style_bg_color(g_widget_ui.selector_items[i], 
                                   lv_color_make(0x20, 0x20, 0x20), 0);
        lv_obj_set_style_bg_opa(g_widget_ui.selector_items[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(g_widget_ui.selector_items[i], 8, 0);
        
        /* 创建标签 */
        g_widget_ui.selector_labels[i] = lv_label_create(g_widget_ui.selector_items[i]);
        lv_label_set_text(g_widget_ui.selector_labels[i], widget_names[i]);
        lv_obj_set_style_text_color(g_widget_ui.selector_labels[i], 
                                     lv_color_hex(widget_colors[i]), 0);
        if (g_widget_ui.font_small) {
            lv_obj_set_style_text_font(g_widget_ui.selector_labels[i], 
                                        g_widget_ui.font_small, 0);
        }
        lv_obj_center(g_widget_ui.selector_labels[i]);
    }
    
    /* 底部提示 */
    g_widget_ui.selector_hint = lv_label_create(screen);
    lv_label_set_text(g_widget_ui.selector_hint, "点击选择 | 按键4返回");
    lv_obj_set_style_text_color(g_widget_ui.selector_hint, 
                                 lv_color_make(0x80, 0x80, 0x80), 0);
    if (g_widget_ui.font_small) {
        lv_obj_set_style_text_font(g_widget_ui.selector_hint, 
                                    g_widget_ui.font_small, 0);
    }
    lv_obj_align(g_widget_ui.selector_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    return 0;
}

/**
 * @brief 更新选择器高亮显示
 */
int widget_ui_update_selector(void)
{
    if (!g_widget_ui.selector_panel) return -1;
    
    widget_selector_state_t state;
    widget_selector_get_state(&state);
    
    /* 更新高亮状态 */
    for (int i = 0; i < 3; i++) {
        if (!g_widget_ui.selector_items[i]) continue;
        
        if (i == state.current_index) {
            /* 当前选中项 - 高亮 */
            lv_obj_set_style_border_width(g_widget_ui.selector_items[i], 3, 0);
            lv_obj_set_style_bg_color(g_widget_ui.selector_items[i], 
                                       lv_color_make(0x30, 0x30, 0x50), 0);
        } else {
            /* 未选中项 */
            lv_obj_set_style_border_width(g_widget_ui.selector_items[i], 2, 0);
            lv_obj_set_style_bg_color(g_widget_ui.selector_items[i], 
                                       lv_color_make(0x20, 0x20, 0x20), 0);
        }
    }
    
    return 0;
}

/* ============================================================================
 * 清理
 * ============================================================================ */

/**
 * @brief 清理小工具UI资源
 */
void widget_ui_cleanup(void)
{
    /* 字体由LVGL管理，不需要手动释放 */
    memset(&g_widget_ui, 0, sizeof(g_widget_ui));
}
