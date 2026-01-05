/**
 * @file mp3_screen_ui.c
 * @brief MP3 页面 UI 构建模块 (LVGL)
 * 
 * 优化版本:
 * 1. 左面板: 切歌控制 + 歌曲索引显示
 * 2. 中面板: 歌曲名(滚动) + 进度条 + 播放时长 + 播放状态
 * 3. 右面板: 音量控制
 * 4. 删除提示文字，[未激活]/[已激活]移到底部
 * 
 * 三面板布局 (适配 128x128 三屏)
 */

#include <rtthread.h>
#include "lvgl.h"
#include "../mp3/mp3_player_controller.h"
#include "../mp3/mp3_screen_ui.h"
#include "../mp3/mp3_screen_context.h"
#include "../screen/screen_ui_manager.h"  /* 引入样式定义 */

#if defined(BSP_USING_RTTHREAD) && !defined(CFG_BOOTLOADER)

/*============================================================================
 * 配置 - 适配 128x128 三屏布局
 *============================================================================*/

/* 单屏尺寸 */
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       128

/* 三屏位置 */
#define LEFT_X              0
#define MID_X               128
#define RIGHT_X             256

/* 进度条配置 */
#define PROGRESS_BAR_WIDTH  110
#define PROGRESS_BAR_HEIGHT 6

/* 颜色定义 */
#define COLOR_BG            lv_color_black()            /* 黑色背景 */
#define COLOR_PRIMARY       lv_color_hex(0x00D9FF)      /* 青色主色 */
#define COLOR_SECONDARY     lv_color_hex(0x00FF88)      /* 绿色辅色 */
#define COLOR_VOLUME        lv_color_hex(0xFFD93D)      /* 黄色音量 */
#define COLOR_TEXT          lv_color_hex(0xFFFFFF)      /* 白色文字 */
#define COLOR_TEXT_DIM      lv_color_hex(0x808080)      /* 暗淡文字 */
#define COLOR_PLAYING       lv_color_hex(0x00FF00)      /* 播放中 - 绿色 */
#define COLOR_PAUSED        lv_color_hex(0xFFFF00)      /* 暂停 - 黄色 */
#define COLOR_ERROR         lv_color_hex(0xFF0000)      /* 错误 - 红色 */
#define COLOR_IDLE          lv_color_hex(0x606060)      /* 空闲 - 灰色 */
#define COLOR_MODE_ACTIVE   lv_color_hex(0x00FF00)      /* 模式激活 - 绿色 */
#define COLOR_MODE_INACTIVE lv_color_hex(0x404040)      /* 模式未激活 - 深灰 */
#define COLOR_PROGRESS_BG   lv_color_hex(0x333333)      /* 进度条背景 */
#define COLOR_PROGRESS_FG   lv_color_hex(0x00D9FF)      /* 进度条前景 */

/*============================================================================
 * UI 句柄结构体
 *============================================================================*/

typedef struct {
    /* 屏幕对象 */
    lv_obj_t *screen;
    
    /* 左面板 - 切歌 + 歌曲索引 */
    lv_obj_t *panel_left;
    lv_obj_t *label_track_title;
    lv_obj_t *label_prev_next;
    lv_obj_t *label_song_index;         /* 歌曲索引移到左面板 */
    lv_obj_t *label_track_mode;         /* 切歌模式状态 - 移到底部 */
    
    /* 中面板 - 播放信息 */
    lv_obj_t *panel_center;
    lv_obj_t *label_song_name;          /* 歌曲名 - 滚动显示 */
    lv_obj_t *progress_bar;             /* 新增: 进度条 */
    lv_obj_t *label_duration;           /* 新增: 播放时长/总时长 */
    lv_obj_t *label_state;              /* 播放状态 */
    lv_obj_t *label_center_hint;        /* 操作提示 */
    
    /* 右面板 - 音量 */
    lv_obj_t *panel_right;
    lv_obj_t *label_vol_title;
    lv_obj_t *label_vol_value;
    lv_obj_t *label_vol_mode;           /* 音量模式状态 - 移到底部 */
    
} mp3_ui_handles_t;

/*============================================================================
 * 全局变量
 *============================================================================*/

static mp3_ui_handles_t g_mp3_ui = {0};
static bool g_ui_built = false;

/* 外部样式管理器 - 在 screen_ui_manager.c 中定义 */
extern screen_ui_manager_t g_ui_mgr;

/*============================================================================
 * 辅助函数
 *============================================================================*/

/**
 * @brief 创建面板
 */
static lv_obj_t *create_panel(lv_obj_t *parent, lv_coord_t x_pos)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(panel, x_pos, 0);
    lv_obj_set_style_bg_color(panel, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

/**
 * @brief 检查对象是否有效
 */
static bool is_obj_valid(lv_obj_t *obj)
{
    return (obj != NULL && lv_obj_is_valid(obj));
}

/**
 * @brief 格式化时间为 mm:ss 格式
 */
static void format_time(uint32_t seconds, char *buf, size_t buf_size)
{
    uint32_t min = seconds / 60;
    uint32_t sec = seconds % 60;
    rt_snprintf(buf, buf_size, "%02d:%02d", (int)min, (int)sec);
}

/*============================================================================
 * UI 构建
 *============================================================================*/

/**
 * @brief 构建左面板 (切歌控制 + 歌曲索引)
 */
static void build_left_panel(lv_obj_t *parent)
{
    /* 标题 */
    g_mp3_ui.label_track_title = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_track_title, "歌曲切换");
    lv_obj_add_style(g_mp3_ui.label_track_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_track_title, COLOR_PRIMARY, 0);
    lv_obj_align(g_mp3_ui.label_track_title, LV_ALIGN_TOP_MID, 0, 8);
    
    /* 歌曲索引 - 从中间面板移到这里 */
    g_mp3_ui.label_song_index = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_song_index, "0/0");
    lv_obj_add_style(g_mp3_ui.label_song_index, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_song_index, COLOR_TEXT, 0);
    lv_obj_align(g_mp3_ui.label_song_index, LV_ALIGN_CENTER, 0, 0);
    
    /* 模式状态指示 - 移到底部 */
    g_mp3_ui.label_track_mode = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_track_mode, "[ 未激活 ]");
    lv_obj_add_style(g_mp3_ui.label_track_mode, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_track_mode, COLOR_MODE_INACTIVE, 0);
    lv_obj_align(g_mp3_ui.label_track_mode, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/**
 * @brief 构建中面板 (播放信息)
 */
static void build_center_panel(lv_obj_t *parent)
{
    /* 歌曲名称 - 使用 SCROLL_CIRCULAR 模式实现循环滚动 */
    g_mp3_ui.label_song_name = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_song_name, "无音乐");
    lv_obj_set_width(g_mp3_ui.label_song_name, SCREEN_WIDTH - 10);
    lv_label_set_long_mode(g_mp3_ui.label_song_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_add_style(g_mp3_ui.label_song_name, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_song_name, COLOR_TEXT, 0);
    lv_obj_set_style_anim_time(g_mp3_ui.label_song_name, 5000, 0);  /* 5秒完成一次滚动 */
    lv_obj_align(g_mp3_ui.label_song_name, LV_ALIGN_TOP_MID, 0, 8);
    
    /* 进度条 - 在歌曲名下方 */
    g_mp3_ui.progress_bar = lv_bar_create(parent);
    lv_obj_set_size(g_mp3_ui.progress_bar, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT);
    lv_bar_set_range(g_mp3_ui.progress_bar, 0, 100);
    lv_bar_set_value(g_mp3_ui.progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_mp3_ui.progress_bar, COLOR_PROGRESS_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_mp3_ui.progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_mp3_ui.progress_bar, COLOR_PROGRESS_FG, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_mp3_ui.progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_mp3_ui.progress_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(g_mp3_ui.progress_bar, 3, LV_PART_INDICATOR);
    lv_obj_align(g_mp3_ui.progress_bar, LV_ALIGN_TOP_MID, 0, 35);
    
    /* 播放时长 - 当前时间/总时长 */
    g_mp3_ui.label_duration = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_duration, "00:00/00:00");
    lv_obj_add_style(g_mp3_ui.label_duration, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_duration, COLOR_PRIMARY, 0);
    lv_obj_align(g_mp3_ui.label_duration, LV_ALIGN_CENTER, 0, 5);
    
    /* 播放状态 */
    g_mp3_ui.label_state = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_state, "停止");
    lv_obj_add_style(g_mp3_ui.label_state, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_state, COLOR_IDLE, 0);
    lv_obj_align(g_mp3_ui.label_state, LV_ALIGN_CENTER, 0, 35);
    
    /* 操作提示 - 保留播放/暂停提示 */
    g_mp3_ui.label_center_hint = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_center_hint, "播放/暂停");
    lv_obj_add_style(g_mp3_ui.label_center_hint, &g_ui_mgr.handles.style_xsmall, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_center_hint, COLOR_TEXT_DIM, 0);
    lv_obj_align(g_mp3_ui.label_center_hint, LV_ALIGN_BOTTOM_MID, 0, -8);
}

/**
 * @brief 构建右面板 (音量控制)
 */
static void build_right_panel(lv_obj_t *parent)
{
    /* 标题 */
    g_mp3_ui.label_vol_title = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_vol_title, "音量");
    lv_obj_add_style(g_mp3_ui.label_vol_title, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_vol_title, COLOR_VOLUME, 0);
    lv_obj_align(g_mp3_ui.label_vol_title, LV_ALIGN_TOP_MID, 0, 8);
    
    /* 音量数值 */
    g_mp3_ui.label_vol_value = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_vol_value, "8");
    lv_obj_add_style(g_mp3_ui.label_vol_value, &g_ui_mgr.handles.style_xxlarge, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_vol_value, COLOR_TEXT, 0);
    lv_obj_align(g_mp3_ui.label_vol_value, LV_ALIGN_CENTER, 0, 0);
    
    /* 模式状态指示 - 移到底部 */
    g_mp3_ui.label_vol_mode = lv_label_create(parent);
    lv_label_set_text(g_mp3_ui.label_vol_mode, "[ 未激活 ]");
    lv_obj_add_style(g_mp3_ui.label_vol_mode, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_mp3_ui.label_vol_mode, COLOR_MODE_INACTIVE, 0);
    lv_obj_align(g_mp3_ui.label_vol_mode, LV_ALIGN_BOTTOM_MID, 0, 0);
}

/*============================================================================
 * 公共 API
 *============================================================================*/

/**
 * @brief 构建 MP3 页面 UI (Group 6)
 * @param parent 父对象 (屏幕)
 * @return 0 成功
 */
int mp3_screen_ui_build(lv_obj_t *parent)
{
    if (g_ui_built) {
        /* 已构建，只需更新屏幕引用 */
        g_mp3_ui.screen = parent;
        return 0;
    }
    
    if (!parent) {
        rt_kprintf("[MP3_UI] Error: parent is NULL\n");
        return -1;
    }
    
    rt_kprintf("[MP3_UI] Building UI...\n");
    
    /* 保存屏幕引用 */
    g_mp3_ui.screen = parent;
    
    /* 创建三个面板 */
    g_mp3_ui.panel_left = create_panel(parent, LEFT_X);
    g_mp3_ui.panel_center = create_panel(parent, MID_X);
    g_mp3_ui.panel_right = create_panel(parent, RIGHT_X);
    
    /* 构建各面板内容 */
    build_left_panel(g_mp3_ui.panel_left);
    build_center_panel(g_mp3_ui.panel_center);
    build_right_panel(g_mp3_ui.panel_right);
    
    g_ui_built = true;
    rt_kprintf("[MP3_UI] UI built successfully\n");
    
    /* 初始更新 */
    mp3_screen_ui_update();
    
    return 0;
}

/**
 * @brief 更新 MP3 页面 UI
 * @return 0 成功
 */
int mp3_screen_ui_update(void)
{
    if (!g_ui_built) {
        return -1;
    }
    
    mp3_display_data_t data;
    if (mp3_player_get_data(&data) != 0) {
        return -1;
    }
    
    /* 获取当前编码器模式 */
    mp3_encoder_mode_t encoder_mode = mp3_screen_context_get_encoder_mode();
    
    /* 更新歌曲名 */
    if (is_obj_valid(g_mp3_ui.label_song_name)) {
        if (data.sd_card_ready && data.total_songs > 0) {
            lv_label_set_text(g_mp3_ui.label_song_name, data.current_song);
        } else if (!data.sd_card_ready) {
            lv_label_set_text(g_mp3_ui.label_song_name, "SD卡未就绪");
        } else {
            lv_label_set_text(g_mp3_ui.label_song_name, "无音乐文件");
        }
    }
    
    /* 更新进度条 */
    if (is_obj_valid(g_mp3_ui.progress_bar)) {
        lv_bar_set_value(g_mp3_ui.progress_bar, data.progress_percent, LV_ANIM_OFF);
    }
    
    /* 更新播放时长 */
    if (is_obj_valid(g_mp3_ui.label_duration)) {
        char current_time[8];
        char total_time[8];
        char duration_buf[20];
        
        format_time(data.current_pos_sec, current_time, sizeof(current_time));
        format_time(data.total_duration_sec, total_time, sizeof(total_time));
        rt_snprintf(duration_buf, sizeof(duration_buf), "%s/%s", current_time, total_time);
        lv_label_set_text(g_mp3_ui.label_duration, duration_buf);
    }
    
    /* 更新歌曲索引 - 现在在左面板 */
    if (is_obj_valid(g_mp3_ui.label_song_index)) {
        char index_buf[16];
        rt_snprintf(index_buf, sizeof(index_buf), "%d/%d", 
                    data.current_index, data.total_songs);
        lv_label_set_text(g_mp3_ui.label_song_index, index_buf);
    }
    
    /* 更新播放状态 */
    if (is_obj_valid(g_mp3_ui.label_state)) {
        const char *state_text;
        lv_color_t state_color;
        
        switch (data.state) {
            case MP3_UI_STATE_PLAYING:
                state_text = "播放中";
                state_color = COLOR_PLAYING;
                break;
            case MP3_UI_STATE_PAUSED:
                state_text = "已暂停";
                state_color = COLOR_PAUSED;
                break;
            case MP3_UI_STATE_ERROR:
                state_text = "错误";
                state_color = COLOR_ERROR;
                break;
            default:
                state_text = "停止";
                state_color = COLOR_IDLE;
                break;
        }
        
        lv_label_set_text(g_mp3_ui.label_state, state_text);
        lv_obj_set_style_text_color(g_mp3_ui.label_state, state_color, 0);
    }
    
    /* 更新音量 */
    if (is_obj_valid(g_mp3_ui.label_vol_value)) {
        char vol_buf[8];
        rt_snprintf(vol_buf, sizeof(vol_buf), "%d", data.volume);
        lv_label_set_text(g_mp3_ui.label_vol_value, vol_buf);
    }
    
    /* 更新切歌模式状态 - 底部位置 */
    if (is_obj_valid(g_mp3_ui.label_track_mode)) {
        if (encoder_mode == MP3_ENCODER_MODE_TRACK) {
            lv_label_set_text(g_mp3_ui.label_track_mode, "[ 旋转以调节 ]");
            lv_obj_set_style_text_color(g_mp3_ui.label_track_mode, COLOR_MODE_ACTIVE, 0);
        } else {
            lv_label_set_text(g_mp3_ui.label_track_mode, "[ 未激活 ]");
            lv_obj_set_style_text_color(g_mp3_ui.label_track_mode, COLOR_MODE_INACTIVE, 0);
        }
    }
    
    /* 更新音量模式状态 - 底部位置 */
    if (is_obj_valid(g_mp3_ui.label_vol_mode)) {
        if (encoder_mode == MP3_ENCODER_MODE_VOLUME) {
            lv_label_set_text(g_mp3_ui.label_vol_mode, "[ 旋转以调节 ]");
            lv_obj_set_style_text_color(g_mp3_ui.label_vol_mode, COLOR_MODE_ACTIVE, 0);
        } else {
            lv_label_set_text(g_mp3_ui.label_vol_mode, "[ 未激活 ]");
            lv_obj_set_style_text_color(g_mp3_ui.label_vol_mode, COLOR_MODE_INACTIVE, 0);
        }
    }
    
    return 0;
}

/**
 * @brief 检查 UI 是否已构建
 */
bool mp3_screen_ui_is_built(void)
{
    return g_ui_built;
}

/**
 * @brief 清理 MP3 页面 UI
 */
void mp3_screen_ui_cleanup(void)
{
    rt_kprintf("[MP3_UI] Cleaning up UI...\n");
    
    /* 清空句柄 */
    rt_memset(&g_mp3_ui, 0, sizeof(g_mp3_ui));
    g_ui_built = false;
    
    rt_kprintf("[MP3_UI] UI cleanup complete\n");
}

/**
 * @brief 获取 MP3 屏幕对象
 */
lv_obj_t* mp3_screen_ui_get_screen(void)
{
    return g_ui_built ? g_mp3_ui.screen : NULL;
}

#endif /* BSP_USING_RTTHREAD && !CFG_BOOTLOADER */