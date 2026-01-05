/**
 * @file mp3_screen_context.c
 * @brief MP3 页面按键上下文处理
 * 
 * 修改版本 - 实现编码器控制音量和切歌:
 * 1. KEY0 (左板块): 单击进入/退出切歌模式, 编码器旋转切换歌曲
 * 2. KEY1 (中板块): 单击=播放/暂停
 * 3. KEY2 (右板块): 单击进入/退出音量模式, 编码器旋转调节音量
 * 4. KEY3 (旋钮):   单击=返回上一组
 * 
 * 编码器模式说明:
 *   - TRACK_MODE: 编码器控制切歌 (顺时针下一曲, 逆时针上一曲)
 *   - VOLUME_MODE: 编码器控制音量 (顺时针增加, 逆时针减少)
 *   - NONE: 编码器控制屏幕组切换 (默认)
 * 
 * 实现方式: 采用与小工具系统相同的模式，通过 mp3_is_in_encoder_mode() 
 * 让 screen.c 的 screen_encoder_event_handler 判断是否拦截编码器事件
 */

#include <rtthread.h>
#include "../mp3/mp3_player_controller.h"
#include "../mp3/mp3_screen_context.h"
#include "../screen/screen.h"
#include "../middleware/event_bus.h"
#include "../screen/screen_core.h"

#if defined(BSP_USING_RTTHREAD) && !defined(CFG_BOOTLOADER)

/*============================================================================
 * 配置
 *============================================================================*/

/* LED 索引定义 - 根据硬件布局 */
#define LED_IDX_LEFT                2       /* 左板块对应 LED */
#define LED_IDX_CENTER              1       /* 中板块对应 LED */
#define LED_IDX_RIGHT               0       /* 右板块对应 LED */

/* LED 颜色定义 */
#define LED_COLOR_TRACK_MODE_ON     0x00FFFF    /* 青色 - 切歌模式开启 */
#define LED_COLOR_TRACK_MODE_OFF    0x008888    /* 暗青色 - 切歌模式关闭 */
#define LED_COLOR_PLAY_PAUSE        0x00FF00    /* 绿色 - 播放/暂停 */
#define LED_COLOR_VOL_MODE_ON       0xFFFF00    /* 黄色 - 音量模式开启 */
#define LED_COLOR_VOL_MODE_OFF      0x888800    /* 暗黄色 - 音量模式关闭 */
#define LED_COLOR_BACK              0xFFFFFF    /* 白色 - 返回 */
#define LED_COLOR_TRACK_CHANGE      0x00FF80    /* 绿青色 - 切歌动作 */
#define LED_COLOR_VOL_CHANGE        0xFFD700    /* 金色 - 音量变化 */

/* LED 反馈持续时间 */
#define LED_FEEDBACK_DURATION_MS    200
#define LED_MODE_FEEDBACK_MS        500

/*============================================================================
 * 类型定义
 *============================================================================*/

/* mp3_encoder_mode_t 已在 mp3_screen_context.h 中定义 */

typedef struct {
    bool active;
    screen_group_t previous_group;      /* 记录进入MP3页面前的组 */
    mp3_encoder_mode_t encoder_mode;    /* 当前编码器模式 */
} mp3_context_t;

/*============================================================================
 * 全局变量
 *============================================================================*/

static mp3_context_t g_mp3_ctx = {0};

/*============================================================================
 * 外部函数声明 - 使用正确的签名
 *============================================================================*/

/* LED 反馈 - 正确的三参数版本 */
extern int event_bus_publish_led_feedback(int led_index, uint32_t color, uint32_t duration_ms);

/* 屏幕更新通知 - 正确的返回类型 */
extern int screen_core_post_update_time(void);

/* 屏幕组切换 */
extern int screen_core_post_switch_group(screen_group_t target_group, bool force);

/* 获取当前屏幕组 */
extern screen_group_t screen_core_get_current_group(void);

/*============================================================================
 * 内部函数
 *============================================================================*/

/**
 * @brief LED 反馈
 */
static void led_feedback(int led_index, uint32_t color, uint32_t duration_ms)
{
    event_bus_publish_led_feedback(led_index, color, duration_ms);
}

/**
 * @brief 请求 UI 更新
 */
static void request_ui_update(void)
{
    screen_core_post_update_time();
}

/**
 * @brief 返回上一个屏幕组
 */
static void switch_to_previous_group(void)
{
    screen_group_t current = screen_core_get_current_group();
    screen_group_t prev;
    
    if (g_mp3_ctx.previous_group < SCREEN_GROUP_MAX && 
        g_mp3_ctx.previous_group != SCREEN_GROUP_6) {
        prev = g_mp3_ctx.previous_group;
    } else {
        prev = SCREEN_GROUP_1;
    }
    
    rt_kprintf("[MP3_CTX] Switching from Group %d to Group %d\n", current, prev);
    screen_core_post_switch_group(prev, false);
}

/*============================================================================
 * 模式切换函数
 *============================================================================*/

/**
 * @brief 切换到切歌模式或退出
 */
static void toggle_track_mode(void)
{
    if (g_mp3_ctx.encoder_mode == MP3_ENCODER_MODE_TRACK) {
        g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_NONE;
        led_feedback(LED_IDX_LEFT, LED_COLOR_TRACK_MODE_OFF, LED_MODE_FEEDBACK_MS);
        rt_kprintf("[MP3_CTX] Track mode OFF\n");
    } else {
        g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_TRACK;
        led_feedback(LED_IDX_LEFT, LED_COLOR_TRACK_MODE_ON, LED_MODE_FEEDBACK_MS);
        rt_kprintf("[MP3_CTX] Track mode ON\n");
    }
    request_ui_update();
}

/**
 * @brief 切换到音量模式或退出
 */
static void toggle_volume_mode(void)
{
    if (g_mp3_ctx.encoder_mode == MP3_ENCODER_MODE_VOLUME) {
        g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_NONE;
        led_feedback(LED_IDX_RIGHT, LED_COLOR_VOL_MODE_OFF, LED_MODE_FEEDBACK_MS);
        rt_kprintf("[MP3_CTX] Volume mode OFF\n");
    } else {
        g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_VOLUME;
        led_feedback(LED_IDX_RIGHT, LED_COLOR_VOL_MODE_ON, LED_MODE_FEEDBACK_MS);
        rt_kprintf("[MP3_CTX] Volume mode ON\n");
    }
    request_ui_update();
}

/*============================================================================
 * 按键处理
 *============================================================================*/

static void handle_key0_click(void)
{
    toggle_track_mode();
}

static void handle_key1_click(void)
{
    mp3_player_toggle_play_pause();
    led_feedback(LED_IDX_CENTER, LED_COLOR_PLAY_PAUSE, LED_FEEDBACK_DURATION_MS);
    request_ui_update();
    rt_kprintf("[MP3_CTX] Play/Pause toggled\n");
}

static void handle_key2_click(void)
{
    toggle_volume_mode();
}

static void handle_key3_click(void)
{
    if (g_mp3_ctx.encoder_mode != MP3_ENCODER_MODE_NONE) {
        g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_NONE;
        rt_kprintf("[MP3_CTX] Encoder mode reset to NONE before returning\n");
    }
    
    led_feedback(LED_IDX_CENTER, LED_COLOR_BACK, LED_FEEDBACK_DURATION_MS);
    switch_to_previous_group();
}

/*============================================================================
 * 公共 API
 *============================================================================*/

void mp3_screen_key_handler(uint8_t key_id, uint8_t event)
{
    if (!g_mp3_ctx.active) {
        return;
    }
    
    #define KEY_EVENT_CLICK 0
    
    if (event != KEY_EVENT_CLICK) {
        return;
    }
    
    switch (key_id) {
        case 0:
            handle_key0_click();
            break;
        case 1:
            handle_key1_click();
            break;
        case 2:
            handle_key2_click();
            break;
        case 3:
            handle_key3_click();
            break;
        default:
            break;
    }
}

int mp3_screen_context_init(void)
{
    rt_memset(&g_mp3_ctx, 0, sizeof(g_mp3_ctx));
    rt_kprintf("[MP3_CTX] Context initialized\n");
    return 0;
}

void mp3_screen_context_deinit(void)
{
    g_mp3_ctx.active = false;
    g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_NONE;
    rt_kprintf("[MP3_CTX] Context deinitialized\n");
}

void mp3_screen_context_activate(void)
{
    screen_group_t current = screen_core_get_current_group();
    if (current != SCREEN_GROUP_6) {
        g_mp3_ctx.previous_group = current;
    }
    
    g_mp3_ctx.active = true;
    g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_NONE;
    
    rt_kprintf("[MP3_CTX] Activated (previous group: %d)\n", g_mp3_ctx.previous_group);
}

void mp3_screen_context_deactivate(void)
{
    g_mp3_ctx.active = false;
    g_mp3_ctx.encoder_mode = MP3_ENCODER_MODE_NONE;
    rt_kprintf("[MP3_CTX] Deactivated (playback continues in background)\n");
}

bool mp3_screen_context_is_active(void)
{
    return g_mp3_ctx.active;
}

mp3_encoder_mode_t mp3_screen_context_get_encoder_mode(void)
{
    return g_mp3_ctx.encoder_mode;
}

/**
 * @brief 检查编码器是否在MP3控制模式
 * @return true 如果编码器正在控制音量或切歌
 * 
 * 此函数供 screen.c 的 screen_encoder_event_handler 调用
 */
bool mp3_is_in_encoder_mode(void)
{
    return g_mp3_ctx.active && 
           (g_mp3_ctx.encoder_mode != MP3_ENCODER_MODE_NONE);
}

/**
 * @brief MP3 编码器事件处理
 * @param delta 编码器增量
 * @return 0 成功处理, -1 未处理
 * 
 * 此函数供 screen.c 的 screen_encoder_event_handler 调用
 */
int mp3_handle_encoder(int delta)
{
    if (!g_mp3_ctx.active || delta == 0) {
        return -1;
    }
    
    switch (g_mp3_ctx.encoder_mode) {
        case MP3_ENCODER_MODE_TRACK:
            if (delta > 0) {
                for (int i = 0; i < delta; i++) {
                    mp3_player_next();
                }
                rt_kprintf("[MP3_CTX] Encoder: Next track (delta=%d)\n", delta);
            } else {
                for (int i = 0; i < -delta; i++) {
                    mp3_player_prev();
                }
                rt_kprintf("[MP3_CTX] Encoder: Prev track (delta=%d)\n", delta);
            }
            led_feedback(LED_IDX_LEFT, LED_COLOR_TRACK_CHANGE, LED_FEEDBACK_DURATION_MS);
            request_ui_update();
            return 0;
            
        case MP3_ENCODER_MODE_VOLUME:
            if (delta > 0) {
                for (int i = 0; i < delta; i++) {
                    mp3_player_volume_up();
                }
                rt_kprintf("[MP3_CTX] Encoder: Volume up (delta=%d)\n", delta);
            } else {
                for (int i = 0; i < -delta; i++) {
                    mp3_player_volume_down();
                }
                rt_kprintf("[MP3_CTX] Encoder: Volume down (delta=%d)\n", delta);
            }
            led_feedback(LED_IDX_RIGHT, LED_COLOR_VOL_CHANGE, LED_FEEDBACK_DURATION_MS);
            request_ui_update();
            return 0;
            
        case MP3_ENCODER_MODE_NONE:
        default:
            return -1;
    }
}

bool mp3_screen_context_is_encoder_active(void)
{
    return mp3_is_in_encoder_mode();
}

int mp3_screen_context_handle_encoder(int delta)
{
    return mp3_handle_encoder(delta);
}

#endif /* BSP_USING_RTTHREAD && !CFG_BOOTLOADER */