#include "screen_context.h"
#include "led_effects_manager.h"
#include "app_controller.h"
#include <rtthread.h>
#include "hid_device.h"  
#include "event_bus.h"
#include "led_effects_manager.h"
#include "screen_ui_manager.h"
#include <time.h>
#include <string.h>
#include "screen_core.h"
#include "encoder_controller.h"

static rt_tick_t last_muyu_tap_time = 0;

#define MUYU_DEBOUNCE_MS 100  // 100ms防抖
/* LED映射函数 - 解决硬件映射问题 */
static int get_led_index_for_key(int key_idx)
{
    switch (key_idx) {
        case 0: return 2;  // Key0 -> LED2
        case 1: return 1;  // Key1 -> LED1  
        case 2: return 0;  // Key2 -> LED0
        case 3: return 1;  // Key3 -> LED1 (特殊情况，保持不变)
        default: return key_idx;
    }
}
static int screen_group4_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_muyu_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_weather_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_tomato_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_stopwatch_key_handler(int key_idx, button_action_t action, void *user_data);

/* Encoder事件处理相关 */
static int l2_media_encoder_event_handler(const event_t *event, void *user_data);
static bool g_encoder_subscribed = false;

/* 全局蓝色呼吸灯效果句柄 - 用于恢复背景特效 */
static led_effect_handle_t g_background_breathing_effect = NULL;



/* 简化版恢复机制 - 避免在ISR中创建复杂对象 */
static rt_timer_t g_delayed_restore_timer = NULL;

/* 启动背景蓝色呼吸灯 - 非ISR版本 */
static void start_background_breathing_effect(void)
{
    if (g_background_breathing_effect) {
        led_effects_stop_effect(g_background_breathing_effect);
        g_background_breathing_effect = NULL;
    }
    g_background_breathing_effect = led_effects_breathing(RGB_COLOR_BLUE, 2000, 255, 0);
}

/* 简化的ISR安全定时器回调 - 只设置标志位 */
static volatile bool g_need_restore_background = false;

static void restore_background_timer_callback(void *parameter)
{
    (void)parameter;
    /* 在ISR中只设置标志位，不执行任何复杂操作 */
    g_need_restore_background = true;
}

/* 检查并处理背景恢复 - 在主循环或非ISR上下文调用 */
static void check_and_restore_background(void)
{
    if (g_need_restore_background) {
        g_need_restore_background = false;
        start_background_breathing_effect();
    }
}

// 木鱼数据定义
typedef struct {
    uint32_t tap_count;
    uint32_t total_taps;
    rt_tick_t last_update_tick;
} muyu_counter_t;

static muyu_counter_t g_muyu_counter = {0};
static rt_mutex_t g_muyu_counter_lock = NULL;
static bool g_muyu_counter_initialized = false;
// 全局番茄钟数据和互斥锁定义
static tomato_data_t g_tomato_data = {0};
static rt_mutex_t g_tomato_lock = NULL;
static bool g_tomato_initialized = false;

// 初始化木鱼计数器
void screen_context_init_muyu_counter(void)
{
    // 只在第一次创建互斥锁和初始化总计数
    if (!g_muyu_counter_lock) {
        g_muyu_counter_lock = rt_mutex_create("muyu_cnt", RT_IPC_FLAG_PRIO);
        
        if (g_muyu_counter_lock) {
            // 第一次初始化时清零所有数据
            memset(&g_muyu_counter, 0, sizeof(g_muyu_counter));
            g_muyu_counter_initialized = true;
        }
    } else {
        // 非第一次进入,只清零本次计数,保留总计数
        rt_mutex_take(g_muyu_counter_lock, RT_WAITING_FOREVER);
        g_muyu_counter.tap_count = 0;  // 清零本次计数
        // g_muyu_counter.total_taps 永远不清零,保留累积值
        g_muyu_counter.last_update_tick = rt_tick_get();
        rt_mutex_release(g_muyu_counter_lock);
    }
}


// 线程安全的计数增加
static void muyu_increment_counter(void)
{
    if (!g_muyu_counter_lock) return;
    
    rt_mutex_take(g_muyu_counter_lock, RT_WAITING_FOREVER);
    g_muyu_counter.tap_count++;  
    g_muyu_counter.total_taps++;   
    g_muyu_counter.last_update_tick = rt_tick_get();
    rt_mutex_release(g_muyu_counter_lock);
}

// 线程安全的计数重置
static void muyu_reset_counter(void)
{
    if (!g_muyu_counter_lock) return;
    
    rt_mutex_take(g_muyu_counter_lock, RT_WAITING_FOREVER);
    g_muyu_counter.tap_count = 0;  // 只重置本次计数
    g_muyu_counter.last_update_tick = rt_tick_get();
    rt_mutex_release(g_muyu_counter_lock);
}

// 线程安全的获取计数
static void muyu_get_counter(uint32_t *tap_count, uint32_t *total_taps)
{
    if (!g_muyu_counter_lock) return;
    
    rt_mutex_take(g_muyu_counter_lock, RT_WAITING_FOREVER);
    if (tap_count) *tap_count = g_muyu_counter.tap_count;
    if (total_taps) *total_taps = g_muyu_counter.total_taps;
    rt_mutex_release(g_muyu_counter_lock);
}

/* 按键-LED映射定义 - 使用修正后的映射关系 */
typedef struct {
    int key_index;
    int led_index;  //  实际LED索引
    uint32_t color;
} key_led_binding_t;

/* 不同组的按键LED绑定配置 - 修正映射关系 */
static const key_led_binding_t group1_led_bindings[] = {
    {0, 2, 0xCCFFFF},   // 按键0 -> LED2: 青色呼吸
    {1, 1, 0xFFCCE5},   // 按键1 -> LED1: 粉色呼吸
    {2, 0, 0xFFFFFF},   // 按键2 -> LED0: 白色呼吸
    {3, 1, 0x00FF00},   // 按键3 -> LED1: 绿色呼吸（保持原有映射）
};

static const key_led_binding_t group2_led_bindings[] = {
    {0, 2, 0xFF8000},   // 按键0 -> LED2: 橙色呼吸
    {1, 1, 0xFFFF00},   // 按键1 -> LED1: 黄色呼吸
    {2, 0, 0x00FF00},   // 按键2 -> LED0: 绿色呼吸
    {3, 1, 0xFF0080},   // 按键3 -> LED1: 洋红呼吸
};

static const key_led_binding_t group3_led_bindings[] = {
    {0, 2, 0x8000FF},   // 按键0 -> LED2: 紫色呼吸
    {1, 1, 0x0080FF},   // 按键1 -> LED1: 蓝色呼吸
    {2, 0, 0xFF4000},   // 按键2 -> LED0: 红橙呼吸
    {3, 1, 0xFFE080},   // 按键3 -> LED1: 浅黄呼吸
};

static const key_led_binding_t l2_media_led_bindings[] = {
    {0, 2, 0x00FF80},   // 上一曲 -> LED2: 绿色呼吸
    {1, 1, 0xFF8000},   // 下一曲 -> LED1: 橙色呼吸
    {2, 0, 0xFF00FF},   // 播放/暂停 -> LED0: 紫色呼吸
    {3, 1, 0xFFFFFF},   // 返回 -> LED1: 白色呼吸
};

static const key_led_binding_t l2_web_led_bindings[] = {
    {0, 2, 0x00BFFF},   // 上翻页 -> LED2: 深天蓝呼吸
    {1, 1, 0x1E90FF},   // 下翻页 -> LED1: 道奇蓝呼吸
    {2, 0, 0x00CED1},   // 刷新 -> LED0: 深青绿呼吸
    {3, 1, 0xFFFFFF},   // 返回 -> LED1: 白色呼吸
};

static const key_led_binding_t l2_shortcut_led_bindings[] = {
    {0, 2, 0x32CD32},   // 复制 -> LED2: 绿色呼吸
    {1, 1, 0xFFD700},   // 粘贴 -> LED1: 金色呼吸
    {2, 0, 0xFF6347},   // 撤销 -> LED0: 番茄红呼吸
    {3, 1, 0xFFFFFF},   // 返回 -> LED1: 白色呼吸
};

/* 改进的LED特效触发函数 - 使用简化的恢复机制 */
static void trigger_key_led_effect(int key_idx, const key_led_binding_t *bindings, int binding_count)
{
    const key_led_binding_t *binding = NULL;
    for (int i = 0; i < binding_count; i++) {
        if (bindings[i].key_index == key_idx) {
            binding = &bindings[i];
            break;
        }
    }
    
    if (!binding) {
        return;
    }
    
    int led_index = binding->led_index;
    
    event_bus_publish_led_feedback(binding->led_index, binding->color, 1000);

}

/* 前向声明 */
static int screen_group1_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_group2_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_group3_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_time_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_media_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_web_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_shortcut_key_handler(int key_idx, button_action_t action, void *user_data);

static key_context_config_t g_l2_time_config = {
    .id = KEY_CTX_L2_TIME,
    .name = "SCREEN_L2_TIME",
    .handler = screen_l2_time_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_media_config = {
    .id = KEY_CTX_L2_MEDIA,
    .name = "SCREEN_L2_MEDIA", 
    .handler = screen_l2_media_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_web_config = {
    .id = KEY_CTX_L2_WEB,
    .name = "SCREEN_L2_WEB",
    .handler = screen_l2_web_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_shortcut_config = {
    .id = KEY_CTX_L2_SHORTCUT,
    .name = "SCREEN_L2_SHORTCUT",
    .handler = screen_l2_shortcut_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};
static key_context_config_t g_l2_muyu_config = {
    .id = KEY_CTX_L2_MUYU,
    .name = "SCREEN_L2_MUYU",
    .handler = screen_l2_muyu_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static key_context_config_t g_l2_weather_config = {
    .id = KEY_CTX_L2_WEATHER,
    .name = "SCREEN_L2_WEATHER",
    .handler = screen_l2_weather_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};
static key_context_config_t stopwatch_config = {
    .id = KEY_CTX_L2_STOPWATCH,
    .name = "SCREEN_L2_STOPWATCH",
    .handler = screen_l2_stopwatch_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

static int screen_group1_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    // 修复连击问题：只处理按下事件，忽略抬起事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // 立即触发对应的LED呼吸灯特效（使用修正后的映射）
    trigger_key_led_effect(key_idx, group1_led_bindings, 
                          sizeof(group1_led_bindings)/sizeof(group1_led_bindings[0]));
    
    switch (key_idx) {
        case 0:
            if (screen_enter_level2_auto(SCREEN_GROUP_1) != 0) {
            }
            break;
            
        case 1:
            screen_enter_level2(SCREEN_L2_WEATHER_GROUP, SCREEN_L2_WEATHER_FORECAST);
            break;
            
        case 2:
            break;
            
        case 3:
            screen_next_group();
            break;
    }
    return 0;
}

static int screen_group2_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件，忽略抬起事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // 使用修正后的映射触发LED特效
    trigger_key_led_effect(key_idx, group2_led_bindings, 
                          sizeof(group2_led_bindings)/sizeof(group2_led_bindings[0]));
    return 0;
}

static int screen_group3_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // 触发LED特效
    trigger_key_led_effect(key_idx, group3_led_bindings, 
                          sizeof(group3_led_bindings)/sizeof(group3_led_bindings[0]));
    
    // ⭐  按键功能实现
    switch (key_idx) {
        case 0:
            // KEY1: 进入媒体控制L2页面
            screen_enter_level2(SCREEN_L2_MEDIA_GROUP, SCREEN_L2_MEDIA_CONTROL);
            break;
            
        case 1:
            // KEY2: 进入网页控制L2页面
            screen_enter_level2(SCREEN_L2_WEB_GROUP, SCREEN_L2_WEB_CONTROL);
            break;
            
        case 2:
            // KEY3: 进入快捷键L2页面
            screen_enter_level2(SCREEN_L2_SHORTCUT_GROUP, SCREEN_L2_SHORTCUT_CONTROL);
            break;
            
        case 3:
            // KEY4: 切换到下一组
            screen_next_group();
            break;
    }
    
    return 0;
}
static int screen_l2_time_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件，忽略抬起事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // 创建L2 Time的LED绑定配置
    key_led_binding_t l2_time_led_bindings[] = {
        {0, 2, 0x00FFFF},   // 按键0 -> LED2: 青色呼吸
        {1, 1, 0xFFFF00},   // 按键1 -> LED1: 黄色呼吸
        {2, 0, 0xFF00FF},   // 按键2 -> LED0: 洋红呼吸
        {3, 1, 0xFFFFFF},   // 按键3 -> LED1: 白色呼吸
    };
    
    // 使用统一的LED特效触发函数
    trigger_key_led_effect(key_idx, l2_time_led_bindings, 
                          sizeof(l2_time_led_bindings)/sizeof(l2_time_led_bindings[0]));
    
    switch (key_idx) {
        case 0:
            break;
            
        case 1:
            break;
            
        case 2:
            break;
            
        case 3:
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static int screen_l2_media_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件，忽略抬起事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    bool hid_ready = hid_device_ready();
    
    // 使用修正后的映射触发LED特效
    trigger_key_led_effect(key_idx, l2_media_led_bindings, 
                          sizeof(l2_media_led_bindings)/sizeof(l2_media_led_bindings[0]));
    
    switch (key_idx) {
        case 0:
            // 上一曲
            if (hid_ready) {
                hid_consumer_click(CC_SCAN_PREV);
            }
            break;
            
        case 1:
            // 下一曲
            if (hid_ready) {
                hid_consumer_click(CC_SCAN_NEXT);
            }
            break;
            
        case 2:
            // 播放/暂停
            if (hid_ready) {
                hid_consumer_click(CC_PLAY_PAUSE);
            }
            break;
            
        case 3:
            // 返回L1
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

/* L2媒体控制encoder事件处理函数 - 用于音量控制 */
static int l2_media_encoder_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (!event || event->type != EVENT_ENCODER_ROTATED) {
        return 0;
    }
    
    if (!hid_device_ready()) {
        return 0;
    }
    
    // 直接访问union中的encoder数据
    int32_t delta = event->data.encoder.delta;
    
    // 根据旋转方向发送音量控制指令
    // delta > 0: 顺时针旋转 -> 音量+
    // delta < 0: 逆时针旋转 -> 音量-
    if (delta > 0) {
        // 顺时针 - 音量+
        for (int i = 0; i < delta && i < 3; i++) {
            hid_consumer_click(CC_VOL_UP);
            rt_thread_mdelay(20);  // 短暂延迟避免命令冲突
        }
    } else if (delta < 0) {
        // 逆时针 - 音量-
        int abs_delta = -delta;
        for (int i = 0; i < abs_delta && i < 3; i++) {
            hid_consumer_click(CC_VOL_DOWN);
            rt_thread_mdelay(20);
        }
    }
    
    return 0;
}

static int screen_l2_web_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件，忽略抬起事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    bool hid_ready = hid_device_ready();
    
    // 使用修正后的映射触发LED特效
    trigger_key_led_effect(key_idx, l2_web_led_bindings, 
                          sizeof(l2_web_led_bindings)/sizeof(l2_web_led_bindings[0]));
    
    switch (key_idx) {
        case 0:
            if (hid_ready) {
                hid_kbd_send_combo(0, KEY_PAGE_UP);
            } else {
            }
            break;
            
        case 1:
            if (hid_ready) {
                hid_kbd_send_combo(0, KEY_PAGE_DOWN);
            } else {
            }
            break;
            
        case 2:
            if (hid_ready) {
                hid_kbd_send_combo(0, KEY_F5);
            } else {
            }
            break;
            
        case 3:
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static int screen_l2_shortcut_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件，忽略抬起事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    bool hid_ready = hid_device_ready();
    
    // 使用修正后的映射触发LED特效
    trigger_key_led_effect(key_idx, l2_shortcut_led_bindings, 
                          sizeof(l2_shortcut_led_bindings)/sizeof(l2_shortcut_led_bindings[0]));
    
    switch (key_idx) {
        case 0:
            if (hid_ready) {
                hid_kbd_send_combo(OS_MODIFIER, KEY_C);
            } else {
            }
            break;
            
        case 1:
            if (hid_ready) {
                hid_kbd_send_combo(OS_MODIFIER, KEY_V);
            } else {
            }
            break;
            
        case 2:
            if (hid_ready) {
                hid_kbd_send_combo(OS_MODIFIER, KEY_Z);
            } else {
            }
            break;
            
        case 3:
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

static bool g_contexts_initialized = false;

int screen_context_init_all(void)
{
    if (g_contexts_initialized) {
        return 0;
    }
    
    int ret;
    
    key_context_config_t config1 = {
        .id = KEY_CTX_MENU_NAVIGATION,
        .name = "SCREEN_GROUP_1",
        .handler = screen_group1_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config1);
    if (ret != 0) {
        return ret;
    }
    
    key_context_config_t config2 = {
        .id = KEY_CTX_SYSTEM,
        .name = "SCREEN_GROUP_2",
        .handler = screen_group2_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config2);
    if (ret != 0) {
        return ret;
    }
    
    key_context_config_t config3 = {
        .id = KEY_CTX_SETTINGS,
        .name = "SCREEN_GROUP_3", 
        .handler = screen_group3_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config3);
    if (ret != 0) {
        return ret;
    }
    
    key_context_config_t config4 = {
        .id = KEY_CTX_UTILITIES,
        .name = "SCREEN_GROUP_4",
        .handler = screen_group4_key_handler,
        .user_data = NULL,
        .priority = 100,
        .exclusive = false
    };
    ret = key_manager_register_context(&config4);
    if (ret != 0) {
        return ret;
    }

    g_contexts_initialized = true;
    
    return 0;
}

int screen_context_deinit_all(void)
{
    if (!g_contexts_initialized) {
        return 0;
    }
    
    // 先清理背景呼吸灯
    screen_context_cleanup_background_breathing();
    
    // 清理定时器
    if (g_delayed_restore_timer) {
        rt_timer_delete(g_delayed_restore_timer);
        g_delayed_restore_timer = NULL;
    }
    
    key_manager_unregister_context(KEY_CTX_MENU_NAVIGATION);
    key_manager_unregister_context(KEY_CTX_SYSTEM);
    key_manager_unregister_context(KEY_CTX_SETTINGS);
    key_manager_unregister_context(KEY_CTX_L2_WEATHER);
    key_manager_unregister_context(KEY_CTX_L2_TIME);
    key_manager_unregister_context(KEY_CTX_L2_MEDIA);
    key_manager_unregister_context(KEY_CTX_L2_WEB);
    key_manager_unregister_context(KEY_CTX_L2_SHORTCUT);
    key_manager_unregister_context(KEY_CTX_UTILITIES);
    key_manager_unregister_context(KEY_CTX_L2_MUYU);    
    g_contexts_initialized = false;
    return 0;
}

int screen_context_activate_for_group(screen_group_t group)
{
    int ret = 0;
    
    if (!g_contexts_initialized) {
        return -RT_ERROR;
    }
    
    screen_context_deactivate_all();
    
    switch (group) {
        case SCREEN_GROUP_1:
            ret = key_manager_activate_context(KEY_CTX_MENU_NAVIGATION);
            break;
            
        case SCREEN_GROUP_2:
            ret = key_manager_activate_context(KEY_CTX_SYSTEM);
            break;
            
        case SCREEN_GROUP_3:
            ret = key_manager_activate_context(KEY_CTX_SETTINGS);
            if (ret == 0) {
            }
            break;
        case SCREEN_GROUP_4:
            ret = key_manager_activate_context(KEY_CTX_UTILITIES);
            break;           
        default:
            return -RT_EINVAL;
    }
    
    if (ret != 0) {
    }
    
    return ret;
}

int screen_context_deactivate_all(void)
{
    key_manager_deactivate_context(KEY_CTX_MENU_NAVIGATION);
    key_manager_deactivate_context(KEY_CTX_SYSTEM);
    key_manager_deactivate_context(KEY_CTX_SETTINGS);
    key_manager_deactivate_context(KEY_CTX_UTILITIES);
    key_manager_deactivate_context(KEY_CTX_L2_WEATHER);

    return 0;
}


static key_context_config_t g_l2_tomato_config = {
    .id = KEY_CTX_L2_TOMATO,
    .name = "SCREEN_L2_TOMATO",
    .handler = screen_l2_tomato_key_handler,
    .user_data = NULL,
    .priority = 110,
    .exclusive = false
};

int screen_context_activate_for_level2(screen_l2_group_t l2_group)
{
    if (!g_contexts_initialized) {
        return -RT_ERROR;
    }
    
    screen_context_deactivate_all();
    
    int ret = 0;
    switch (l2_group) {
        case SCREEN_L2_TIME_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_TIME) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_time_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_TIME);
            if (ret == 0) {
            }
            break;
            
        case SCREEN_L2_MEDIA_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_MEDIA) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_media_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_MEDIA);
            if (ret == 0) {
                // 设置encoder为音量控制模式
                encoder_controller_set_mode(ENCODER_MODE_VOLUME);
                
                // 订阅encoder事件
                event_bus_subscribe(
                    EVENT_ENCODER_ROTATED,
                    l2_media_encoder_event_handler,
                    NULL,
                    EVENT_PRIORITY_HIGH
                );
                g_encoder_subscribed = true;
                
                // 启动encoder polling
                encoder_controller_start_polling();
            }
            break;
            
        case SCREEN_L2_WEB_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_WEB) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_web_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_WEB);
            if (ret == 0) {
            }
            break;
            
        case SCREEN_L2_SHORTCUT_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_SHORTCUT) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_shortcut_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_SHORTCUT);
            break;

        case SCREEN_L2_MUYU_GROUP:
            // 初始化木鱼计数器(创建互斥锁)
            screen_context_init_muyu_counter();
            
            if (key_manager_get_context_name(KEY_CTX_L2_MUYU) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_muyu_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_MUYU);
            if (ret == 0) {
            }
            break;

        case SCREEN_L2_WEATHER_GROUP:
            if (key_manager_get_context_name(KEY_CTX_L2_WEATHER) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_weather_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_WEATHER);
            break;
        case SCREEN_L2_TOMATO_GROUP:
            screen_context_init_tomato_data();
            
            if (key_manager_get_context_name(KEY_CTX_L2_TOMATO) == "UNREGISTERED") {
                ret = key_manager_register_context(&g_l2_tomato_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_TOMATO);
            break;
        case SCREEN_L2_STOPWATCH_GROUP:
            screen_context_init_stopwatch_data();
            
            if (key_manager_get_context_name(KEY_CTX_L2_STOPWATCH) == "UNREGISTERED") {
                ret = key_manager_register_context(&stopwatch_config);
                if (ret != 0) {
                    return ret;
                }
            }
            
            ret = key_manager_activate_context(KEY_CTX_L2_STOPWATCH);
            break;    
        default:
            ret = -RT_EINVAL;
            break;
    }
    
    return ret;
}

int screen_context_deactivate_level2(void)
{
    // 停止L2的encoder订阅
    if (g_encoder_subscribed) {
        event_bus_unsubscribe(EVENT_ENCODER_ROTATED, l2_media_encoder_event_handler);
        g_encoder_subscribed = false;
    }
    
    // 恢复encoder到L1的屏幕切换模式
    encoder_controller_set_mode(ENCODER_MODE_SCREEN_SWITCH);
    // 确保encoder polling保持运行状态（L1需要用它切换页面）
    encoder_controller_start_polling();
    
    key_manager_deactivate_context(KEY_CTX_L2_TIME);
    key_manager_deactivate_context(KEY_CTX_L2_MEDIA);
    key_manager_deactivate_context(KEY_CTX_L2_WEB);
    key_manager_deactivate_context(KEY_CTX_L2_SHORTCUT);
    key_manager_deactivate_context(KEY_CTX_L2_MUYU);
    key_manager_deactivate_context(KEY_CTX_L2_TOMATO);
    key_manager_deactivate_context(KEY_CTX_L2_STOPWATCH);
    return 0;
}

int screen_context_init_background_breathing(void)
{
    start_background_breathing_effect();
    return 0;
}

int screen_context_cleanup_background_breathing(void)
{
    if (g_background_breathing_effect) {
        led_effects_stop_effect(g_background_breathing_effect);
        g_background_breathing_effect = NULL;
    }
    return 0;
}

int screen_context_restore_background_breathing(void)
{
    start_background_breathing_effect();
    return 0;
}

void screen_context_process_background_restore(void)
{
    check_and_restore_background();
}

static int screen_group4_key_handler(int key_idx, button_action_t action, void *user_data);
static int screen_l2_muyu_key_handler(int key_idx, button_action_t action, void *user_data);

static const key_led_binding_t group4_led_bindings[] = {
    {0, 2, 0xFFD700},   // 按键0 -> LED2: 金色呼吸（木鱼）
    {1, 1, 0xFF6347},   // 按键1 -> LED1: 番茄红呼吸（番茄钟）
    {2, 0, 0x90EE90},   // 按键2 -> LED0: 浅绿色呼吸（秒表）
    {3, 1, 0xFFFFFF},   // 按键3 -> LED1: 白色呼吸（下一组）
};

static const key_led_binding_t l2_muyu_led_bindings[] = {
    {0, 2, 0xFFD700},   // 重置 -> LED2: 金色呼吸
    {1, 1, 0xFF8C00},   // 统计 -> LED1: 橙色呼吸
    {2, 0, 0xFFA500},   // 设置 -> LED0: 橙色呼吸
    {3, 1, 0xFFFFFF},   // 返回 -> LED1: 白色呼吸
};

static const key_led_binding_t l2_weather_led_bindings[] = {
    {0, 2, 0x00FFFF},   // 按键0 -> LED2: 青色
    {1, 1, 0xFFFF00},   // 按键1 -> LED1: 黄色
    {2, 0, 0xFF00FF},   // 按键2 -> LED0: 洋红
    {3, 1, 0xFFFFFF},   // 按键3 -> LED1: 白色（返回）
};

/* Group 4按键处理函数实现 */
static int screen_group4_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    // 修复连击问题：只处理按下事件
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // 触发LED特效
    trigger_key_led_effect(key_idx, group4_led_bindings, 
                          sizeof(group4_led_bindings)/sizeof(group4_led_bindings[0]));
    
    // ⭐  按键功能实现
    switch (key_idx) {
        case 0:
            // KEY1: 进入木鱼L2页面
            screen_enter_level2(SCREEN_L2_MUYU_GROUP, SCREEN_L2_MUYU_MAIN);
            break;
            
        case 1:
            // KEY2: 进入番茄钟L2页面（预留）
            screen_enter_level2(SCREEN_L2_TOMATO_GROUP, SCREEN_L2_TOMATO_TIMER);
            break;
            
        case 2:
            // KEY3: 进入秒表L2页面
            screen_enter_level2(SCREEN_L2_STOPWATCH_GROUP, SCREEN_L2_STOPWATCH_TIMER);
            break;
            
        case 3:
            // KEY4: 切换到下一组
            screen_next_group();
            break;
    }
    
    return 0;
}

/* L2天气预报按键处理函数 */
static int screen_l2_weather_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    /* LED绑定配置 */
    static const key_led_binding_t l2_weather_led_bindings[] = {
        {0, 2, 0x00FFFF},   // 青色
        {1, 1, 0xFFFF00},   // 黄色
        {2, 0, 0xFF00FF},   // 洋红
        {3, 1, 0xFFFFFF},   // 白色（返回）
    };
    
    /* LED反馈 */
    const key_led_binding_t *binding = NULL;
    for (int i = 0; i < sizeof(l2_weather_led_bindings)/sizeof(l2_weather_led_bindings[0]); i++) {
        if (l2_weather_led_bindings[i].key_index == key_idx) {
            binding = &l2_weather_led_bindings[i];
            break;
        }
    }
    
    if (binding) {
        event_bus_publish_led_feedback(binding->led_index, binding->color, 800);
    }
    
    switch (key_idx) {
        case 0:
        case 1:
        case 2:
            /* 预留功能 */
            break;
            
        case 3:
            /* 返回L1 */
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

/* L2木鱼按键处理函数实现 */
static int screen_l2_muyu_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // 防抖处理
    rt_tick_t now = rt_tick_get();
    if (key_idx == 0 && (now - last_muyu_tap_time) < rt_tick_from_millisecond(MUYU_DEBOUNCE_MS)) {
        return 0;  // 忽略过快的按键
    }
    
    // LED特效(异步)
    const key_led_binding_t *binding = NULL;
    for (int i = 0; i < sizeof(l2_muyu_led_bindings)/sizeof(l2_muyu_led_bindings[0]); i++) {
        if (l2_muyu_led_bindings[i].key_index == key_idx) {
            binding = &l2_muyu_led_bindings[i];
            break;
        }
    }
    
    if (binding) {
        event_bus_publish_led_feedback(binding->led_index, binding->color, 800);
    }
    
    switch (key_idx) {
        case 0:
            // KEY1: 计数并立即触发UI更新
            last_muyu_tap_time = now;
            muyu_increment_counter();
            
            // 立即通过消息队列请求UI更新(线程安全)
            screen_core_post_update_time();
            break;
            
        case 1:
            // KEY2: 只做重置,不调用UI
            muyu_reset_counter();
            
            // 【关键修复】立即通过消息队列请求UI更新(线程安全)
            screen_core_post_update_time();
            break;
            
        case 2:
            // KEY3: 预留
            break;
            
        case 3:
            // KEY4: 返回L1
            screen_return_to_level1();
            break;
    }
    return 0;
}


/* 木鱼重置事件处理实现 */
int screen_context_handle_muyu_reset(void)
{
    muyu_reset_counter();   
    return 0;
}

/* 获取木鱼计数 - 公共接口 */
int screen_context_get_muyu_count(uint32_t *tap_count, uint32_t *total_taps)
{
    if (!tap_count && !total_taps) {
        return -RT_EINVAL;
    }
    
    muyu_get_counter(tap_count, total_taps);
    return 0;
}


// 3. 番茄钟初始化函数 (添加到木鱼初始化函数之后)

void screen_context_init_tomato_data(void)
{
    if (!g_tomato_lock) {
        g_tomato_lock = rt_mutex_create("tomato_lock", RT_IPC_FLAG_PRIO);
        
        if (g_tomato_lock) {
            rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
            
            // 初始化番茄钟数据
            memset(&g_tomato_data, 0, sizeof(g_tomato_data));
            
            // 设置默认配置
            g_tomato_data.focus_duration_min = 25;
            g_tomato_data.short_break_min = 5;
            g_tomato_data.long_break_min = 15;
            g_tomato_data.long_break_interval = 4;
            
            // 设置初始状态
            g_tomato_data.current_mode = TOMATO_MODE_FOCUS;
            g_tomato_data.current_state = TOMATO_STATE_IDLE;
            g_tomato_data.total_seconds = 25 * 60;
            g_tomato_data.remaining_seconds = g_tomato_data.total_seconds;
            g_tomato_data.current_round = 0;
            g_tomato_data.progress_percent = 0;
            
            g_tomato_data.valid = true;
            g_tomato_data.last_update_tick = rt_tick_get();
            
            g_tomato_initialized = true;
            
            rt_mutex_release(g_tomato_lock);
        }
    }
}

void tomato_process_countdown(void)
{
    if (!g_tomato_lock) return;
    
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    
    // 只有RUNNING状态才执行倒计时
    if (g_tomato_data.current_state == TOMATO_STATE_RUNNING) {
        rt_tick_t current_tick = rt_tick_get();
        rt_tick_t elapsed_ticks = current_tick - g_tomato_data.last_update_tick;
        
        // 【修复3: 改进时间精度计算】
        // 计算已经过的完整秒数,避免累积误差
        uint32_t elapsed_seconds = elapsed_ticks / RT_TICK_PER_SECOND;
        
        if (elapsed_seconds > 0) {
            // 一次性减去所有已过的秒数
            if (g_tomato_data.remaining_seconds >= elapsed_seconds) {
                g_tomato_data.remaining_seconds -= elapsed_seconds;
                
                // 更新进度百分比
                if (g_tomato_data.total_seconds > 0) {
                    uint16_t elapsed = g_tomato_data.total_seconds - g_tomato_data.remaining_seconds;
                    g_tomato_data.progress_percent = (elapsed * 100) / g_tomato_data.total_seconds;
                }
                
                // 【关键】更新时间戳,只保留不足1秒的余数,避免时间漂移
                g_tomato_data.last_update_tick = current_tick - (elapsed_ticks % RT_TICK_PER_SECOND);
                
            } else {
                // 倒计时完成
                g_tomato_data.remaining_seconds = 0;
                g_tomato_data.current_state = TOMATO_STATE_COMPLETED;
                g_tomato_data.progress_percent = 100;
                
                // 更新统计数据
                if (g_tomato_data.current_mode == TOMATO_MODE_FOCUS) {
                    g_tomato_data.today_completed++;
                    g_tomato_data.total_completed++;
                    g_tomato_data.continuous_count++;
                }
                
                g_tomato_data.last_update_tick = current_tick;
            }
        }
    }
    
    rt_mutex_release(g_tomato_lock);
}

// 番茄钟操作函数
int screen_context_get_tomato_data(tomato_data_t *data)
{
    if (!data) return -RT_EINVAL;
    
    if (!g_tomato_lock) {
        *data = g_tomato_data;
        return 0;
    }
    
    if (rt_interrupt_get_nest() > 0) {
        *data = g_tomato_data;
        return 0;
    }
    
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    *data = g_tomato_data;
    rt_mutex_release(g_tomato_lock);
    
    return 0;
}

// 开始/继续番茄钟
int screen_context_handle_tomato_start(void)
{
    if (!g_tomato_lock) return -RT_ERROR;
    
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    
    if (g_tomato_data.current_state == TOMATO_STATE_IDLE ||
        g_tomato_data.current_state == TOMATO_STATE_PAUSED) {
        
        g_tomato_data.current_state = TOMATO_STATE_RUNNING;
        g_tomato_data.last_update_tick = rt_tick_get();
        
        rt_mutex_release(g_tomato_lock);
        screen_ui_start_tomato_background_display();
        
        screen_core_post_update_time();
        return 0;
    }
    
    rt_mutex_release(g_tomato_lock);
    return -RT_ERROR;
}


// 暂停番茄钟
int screen_context_handle_tomato_pause(void)
{
    if (!g_tomato_lock) return -RT_ERROR;
    
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    
    if (g_tomato_data.current_state == TOMATO_STATE_RUNNING) {
        g_tomato_data.current_state = TOMATO_STATE_PAUSED;
        g_tomato_data.last_update_tick = rt_tick_get();
        
        rt_mutex_release(g_tomato_lock);
        
        screen_ui_stop_tomato_background_display();
        
        screen_core_post_update_time();
        return 0;
    }
    
    rt_mutex_release(g_tomato_lock);
    return -RT_ERROR;
}

// 停止番茄钟(放弃当前)
int screen_context_handle_tomato_stop(void)
{
    if (!g_tomato_lock) return -RT_ERROR;
    
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    
    // 重置连续计数
    if (g_tomato_data.current_mode == TOMATO_MODE_FOCUS) {
        g_tomato_data.continuous_count = 0;
    }
    
    // 重置状态
    g_tomato_data.current_state = TOMATO_STATE_IDLE;
    g_tomato_data.remaining_seconds = g_tomato_data.total_seconds;
    g_tomato_data.progress_percent = 0;
    g_tomato_data.last_update_tick = rt_tick_get();
    
    rt_mutex_release(g_tomato_lock);
    screen_ui_stop_tomato_background_display();
    screen_core_post_update_time();
    return 0;
}

// 切换模式
int screen_context_handle_tomato_mode_switch(void)
{
    if (!g_tomato_lock) return -RT_ERROR;
    
    // ← 删除ISR检查,统一使用互斥锁
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    
    if (g_tomato_data.current_state != TOMATO_STATE_IDLE) {
        rt_mutex_release(g_tomato_lock);
        return -RT_ERROR;
    }
    
    g_tomato_data.current_mode = (g_tomato_data.current_mode + 1) % TOMATO_MODE_MAX;
    
    switch (g_tomato_data.current_mode) {
        case TOMATO_MODE_FOCUS:
            g_tomato_data.total_seconds = g_tomato_data.focus_duration_min * 60;
            break;
        case TOMATO_MODE_SHORT_BREAK:
            g_tomato_data.total_seconds = g_tomato_data.short_break_min * 60;
            break;
        case TOMATO_MODE_LONG_BREAK:
            g_tomato_data.total_seconds = g_tomato_data.long_break_min * 60;
            break;
        default:
            g_tomato_data.total_seconds = 25 * 60;
            break;
    }
    
    g_tomato_data.remaining_seconds = g_tomato_data.total_seconds;
    g_tomato_data.progress_percent = 0;
    g_tomato_data.last_update_tick = rt_tick_get();
    
    rt_mutex_release(g_tomato_lock);
    
    screen_core_post_update_time();
    return 0;
}
// 完成确认(用于完成状态后进入下一模式)
int screen_context_handle_tomato_complete(void)
{
    if (!g_tomato_lock) return -RT_ERROR;
    
    if (rt_interrupt_get_nest() > 0) {
        if (g_tomato_data.current_state == TOMATO_STATE_COMPLETED) {
            tomato_mode_t next_mode;
            
            if (g_tomato_data.current_mode == TOMATO_MODE_FOCUS) {
                g_tomato_data.current_round++;
                
                if (g_tomato_data.current_round >= g_tomato_data.long_break_interval) {
                    next_mode = TOMATO_MODE_LONG_BREAK;
                    g_tomato_data.current_round = 0;
                } else {
                    next_mode = TOMATO_MODE_SHORT_BREAK;
                }
            } else {
                next_mode = TOMATO_MODE_FOCUS;
            }
            
            g_tomato_data.current_mode = next_mode;
            
            switch (next_mode) {
                case TOMATO_MODE_FOCUS:
                    g_tomato_data.total_seconds = g_tomato_data.focus_duration_min * 60;
                    break;
                case TOMATO_MODE_SHORT_BREAK:
                    g_tomato_data.total_seconds = g_tomato_data.short_break_min * 60;
                    break;
                case TOMATO_MODE_LONG_BREAK:
                    g_tomato_data.total_seconds = g_tomato_data.long_break_min * 60;
                    break;
            }
            
            g_tomato_data.remaining_seconds = g_tomato_data.total_seconds;
            g_tomato_data.current_state = TOMATO_STATE_IDLE;
            g_tomato_data.progress_percent = 0;
            g_tomato_data.last_update_tick = rt_tick_get();
            
            screen_core_post_update_time();
            return 0;
        }
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_tomato_lock, RT_WAITING_FOREVER);
    
    if (g_tomato_data.current_state == TOMATO_STATE_COMPLETED) {
        tomato_mode_t next_mode;
        
        if (g_tomato_data.current_mode == TOMATO_MODE_FOCUS) {
            g_tomato_data.current_round++;
            
            if (g_tomato_data.current_round >= g_tomato_data.long_break_interval) {
                next_mode = TOMATO_MODE_LONG_BREAK;
                g_tomato_data.current_round = 0;
            } else {
                next_mode = TOMATO_MODE_SHORT_BREAK;
            }
        } else {
            next_mode = TOMATO_MODE_FOCUS;
        }
        
        g_tomato_data.current_mode = next_mode;
        
        switch (next_mode) {
            case TOMATO_MODE_FOCUS:
                g_tomato_data.total_seconds = g_tomato_data.focus_duration_min * 60;
                break;
            case TOMATO_MODE_SHORT_BREAK:
                g_tomato_data.total_seconds = g_tomato_data.short_break_min * 60;
                break;
            case TOMATO_MODE_LONG_BREAK:
                g_tomato_data.total_seconds = g_tomato_data.long_break_min * 60;
                break;
        }
        
        g_tomato_data.remaining_seconds = g_tomato_data.total_seconds;
        g_tomato_data.current_state = TOMATO_STATE_IDLE;
        g_tomato_data.progress_percent = 0;
        g_tomato_data.last_update_tick = rt_tick_get();
        
        rt_mutex_release(g_tomato_lock);
        screen_ui_stop_tomato_background_display();
        screen_core_post_update_time();
        return 0;
    }
    
    rt_mutex_release(g_tomato_lock);
    return -RT_ERROR;
}

// 6. 番茄钟按键处理函数 (添加到screen_context.c中的L2按键处理函数区域)

/* LED绑定配置 - 番茄钟 */
static const key_led_binding_t l2_tomato_led_bindings[] = {
    {0, 2, 0xFF6347},   // 模式切换/放弃 -> LED2: 番茄红
    {1, 1, 0x00FF00},   // 开始/暂停 -> LED1: 绿色
    {2, 0, 0xFFD700},   // 设置(预留) -> LED0: 金色
    {3, 1, 0xFFFFFF},   // 返回 -> LED1: 白色
};

/* L2番茄钟按键处理函数 */
static int screen_l2_tomato_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    // LED反馈
    const key_led_binding_t *binding = NULL;
    for (int i = 0; i < sizeof(l2_tomato_led_bindings)/sizeof(l2_tomato_led_bindings[0]); i++) {
        if (l2_tomato_led_bindings[i].key_index == key_idx) {
            binding = &l2_tomato_led_bindings[i];
            break;
        }
    }
    
    if (binding) {
        event_bus_publish_led_feedback(binding->led_index, binding->color, 800);
    }
    
    // 获取当前番茄钟状态
    tomato_data_t tomato_data;
    screen_context_get_tomato_data(&tomato_data);
    
    switch (key_idx) {
        case 0:  // KEY1 - 改进: 待机=切换模式, 运行/暂停=停止重置, 完成=确认
            if (tomato_data.current_state == TOMATO_STATE_IDLE) {
                // 待机状态: 模式切换
                screen_context_handle_tomato_mode_switch();
            } 
            else if (tomato_data.current_state == TOMATO_STATE_COMPLETED) {
                // 完成状态: 确认完成,进入下一模式
                screen_context_handle_tomato_complete();
            }
            // 新增: 运行/暂停状态下,KEY1 = 停止并重置
            else if (tomato_data.current_state == TOMATO_STATE_RUNNING ||
                     tomato_data.current_state == TOMATO_STATE_PAUSED) {
                screen_context_handle_tomato_stop();
            }
            break;
            
        case 1:  // KEY2: 开始/暂停
            if (tomato_data.current_state == TOMATO_STATE_IDLE ||
                tomato_data.current_state == TOMATO_STATE_PAUSED) {
                screen_context_handle_tomato_start();
            } else if (tomato_data.current_state == TOMATO_STATE_RUNNING) {
                screen_context_handle_tomato_pause();
            } else if (tomato_data.current_state == TOMATO_STATE_COMPLETED) {
                // 完成状态下也可以作为确认键
                screen_context_handle_tomato_complete();
            }
            break;
            
        case 2:  // KEY3: 设置(预留)
            // 预留用于进入设置界面
            break;
            
        case 3:  // KEY4: 返回L1
            screen_return_to_level1();
            break;
    }
    
    return 0;
}

/* 全局秒表数据和互斥锁 */
static stopwatch_data_t g_stopwatch_data = {0};
static rt_mutex_t g_stopwatch_lock = NULL;
static bool g_stopwatch_initialized = false;

/* 秒表初始化 */
void screen_context_init_stopwatch_data(void)
{
    if (!g_stopwatch_lock) {
        g_stopwatch_lock = rt_mutex_create("sw_lock", RT_IPC_FLAG_PRIO);
        
        if (g_stopwatch_lock) {
            rt_mutex_take(g_stopwatch_lock, RT_WAITING_FOREVER);
            
            memset(&g_stopwatch_data, 0, sizeof(g_stopwatch_data));
            g_stopwatch_data.valid = true;
            g_stopwatch_data.is_running = false;
            g_stopwatch_data.elapsed_deciseconds = 0;
            g_stopwatch_data.pause_duration = 0;
            
            g_stopwatch_initialized = true;
            
            rt_mutex_release(g_stopwatch_lock);
        }
    } else {
        // 非首次进入,保持数据不清零
        rt_mutex_take(g_stopwatch_lock, RT_WAITING_FOREVER);
        // 不修改 elapsed_deciseconds,保留计时数据
        rt_mutex_release(g_stopwatch_lock);
    }
}


/* 获取秒表数据 */
int screen_context_get_stopwatch_data(stopwatch_data_t *data)
{
    if (!data) return -RT_EINVAL;
    
    if (!g_stopwatch_lock) {
        *data = g_stopwatch_data;
        return 0;
    }
    
    if (rt_interrupt_get_nest() > 0) {
        *data = g_stopwatch_data;
        return 0;
    }
    
    rt_mutex_take(g_stopwatch_lock, RT_WAITING_FOREVER);

    if (g_stopwatch_data.is_running) {
        rt_tick_t current_tick = rt_tick_get();
        rt_tick_t elapsed_ticks = current_tick - g_stopwatch_data.start_tick - g_stopwatch_data.pause_duration;
        
        // 更新实时经过的时间（十分之一秒）
        g_stopwatch_data.elapsed_deciseconds = (elapsed_ticks * 10) / RT_TICK_PER_SECOND;
    }
    
    *data = g_stopwatch_data;
    rt_mutex_release(g_stopwatch_lock);
    
    return 0;
}

/* 开始/继续秒表 */
int screen_context_handle_stopwatch_start(void)
{
    if (!g_stopwatch_lock) return -RT_ERROR;
    
    rt_mutex_take(g_stopwatch_lock, RT_WAITING_FOREVER);
    
    if (!g_stopwatch_data.is_running) {
        if (g_stopwatch_data.elapsed_deciseconds == 0) {
            // 从零开始
            g_stopwatch_data.start_tick = rt_tick_get();
            g_stopwatch_data.pause_duration = 0;
        } else {
            // 从暂停继续
            rt_tick_t pause_elapsed = rt_tick_get() - g_stopwatch_data.pause_tick;
            g_stopwatch_data.pause_duration += pause_elapsed;
        }
        
        g_stopwatch_data.is_running = true;
    }
    
    rt_mutex_release(g_stopwatch_lock);
    
    screen_core_post_update_time();
    return 0;
}

/* 暂停秒表 */
int screen_context_handle_stopwatch_pause(void)
{
    if (!g_stopwatch_lock) return -RT_ERROR;
    
    rt_mutex_take(g_stopwatch_lock, RT_WAITING_FOREVER);
    
    if (g_stopwatch_data.is_running) {
        g_stopwatch_data.is_running = false;
        g_stopwatch_data.pause_tick = rt_tick_get();
    }
    
    rt_mutex_release(g_stopwatch_lock);
    
    screen_core_post_update_time();
    return 0;
}

/* 重置秒表 */
int screen_context_handle_stopwatch_reset(void)
{
    if (!g_stopwatch_lock) return -RT_ERROR;
    
    rt_mutex_take(g_stopwatch_lock, RT_WAITING_FOREVER);
    
    g_stopwatch_data.is_running = false;
    g_stopwatch_data.elapsed_deciseconds = 0;
    g_stopwatch_data.start_tick = 0;
    g_stopwatch_data.pause_tick = 0;
    g_stopwatch_data.pause_duration = 0;
    
    rt_mutex_release(g_stopwatch_lock);
    
    screen_core_post_update_time();
    return 0;
}

/* L2秒表按键处理函数 */
static int screen_l2_stopwatch_key_handler(int key_idx, button_action_t action, void *user_data)
{
    (void)user_data;
    
    if (action != BUTTON_PRESSED) {
        return 0;
    }
    
    /* LED绑定配置 */
    static const key_led_binding_t l2_stopwatch_led_bindings[] = {
        {0, 2, 0x00FF00},   // 开始/暂停 -> LED2: 绿色
        {1, 1, 0xFF0000},   // 重置 -> LED1: 红色
        {2, 0, 0xFFD700},   // 预留 -> LED0: 金色
        {3, 1, 0xFFFFFF},   // 返回 -> LED1: 白色
    };
    
    /* LED反馈 */
    const key_led_binding_t *binding = NULL;
    for (int i = 0; i < sizeof(l2_stopwatch_led_bindings)/sizeof(l2_stopwatch_led_bindings[0]); i++) {
        if (l2_stopwatch_led_bindings[i].key_index == key_idx) {
            binding = &l2_stopwatch_led_bindings[i];
            break;
        }
    }
    
    if (binding) {
        event_bus_publish_led_feedback(binding->led_index, binding->color, 800);
    }
    
    // 获取当前秒表状态
    stopwatch_data_t stopwatch_data;
    screen_context_get_stopwatch_data(&stopwatch_data);
    
    switch (key_idx) {
        case 0:  // KEY1: 开始/暂停
            if (stopwatch_data.is_running) {
                screen_context_handle_stopwatch_pause();
            } else {
                screen_context_handle_stopwatch_start();
            }
            break;
            
        case 1:  // KEY2: 重置
            screen_context_handle_stopwatch_reset();
            break;
            
        case 2:  // KEY3: 预留功能
            break;
            
        case 3:  // KEY4: 返回L1
            screen_return_to_level1();
            break;
    }
    
    return 0;
}