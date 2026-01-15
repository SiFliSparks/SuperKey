/**
 * @file widget_manager.c
 * @brief 小工具管理模块 - 实现
 */

#include "../widget/widget_manager.h"
#include "../widget/widget_storage.h"
#include <rtthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * 菜名数组 - 今天吃什么
 * ============================================================================ */

/**
 * 菜名数组 - 可自由扩展
 * 格式: 每个菜名不超过30个字符(UTF-8编码下约10个中文字符)
 */
static const char* g_food_menu[] = {
    /* 中式快餐 */
    "红烧肉",
    "宫保鸡丁",
    "鱼香肉丝",
    "麻婆豆腐",
    "糖醋排骨",
    "回锅肉",
    "水煮鱼",
    "酸菜鱼",
    "番茄炒蛋",
    "青椒肉丝",
    "地三鲜",
    "蒜蓉西兰花",
    "干煸四季豆",
    "蛋炒饭",
    "扬州炒饭",
    
    /* 面食 */
    "兰州拉面",
    "炸酱面",
    "担担面",
    "刀削面",
    "热干面",
    "重庆小面",
    "牛肉面",
    "阳春面",
    "炒面",
    
    /* 米饭套餐 */
    "黄焖鸡米饭",
    "卤肉饭",
    "煲仔饭",
    "盖浇饭",
    "咖喱饭",
    
    /* 小吃 */
    "饺子",
    "馄饨",
    "包子",
    "煎饼果子",
    "肉夹馍",
    "凉皮",
    "米线",
    "螺蛳粉",
    "酸辣粉",
    
    /* 西式快餐 */
    "汉堡",
    "披萨",
    "炸鸡",
    "意大利面",
    "三明治",
    
    /* 日韩料理 */
    "寿司",
    "拉面",
    "咖喱乌冬",
    "石锅拌饭",
    "炸猪排",
    "韩式烤肉",
    
    /* 火锅烧烤 */
    "火锅",
    "麻辣烫",
    "串串香",
    "烤肉",
    "烤鱼",
    
    /* 粤式 */
    "叉烧饭",
    "烧鹅饭",
    "白切鸡",
    "肠粉",
    "云吞面",
    
    /* 其他 */
    "沙县小吃",
    "麻辣香锅",
    "冒菜",
    "关东煮",
    "便利店便当"
};

#define FOOD_MENU_COUNT (sizeof(g_food_menu) / sizeof(g_food_menu[0]))

/* ============================================================================
 * 全局状态
 * ============================================================================ */

static struct {
    bool initialized;
    rt_mutex_t lock;
    
    /* 各工具运行状态 */
    widget_random_state_t random_state;
    widget_offwork_state_t offwork_state;
    widget_food_state_t food_state;
    widget_selector_state_t selector_state;
    
} g_widget_mgr = {0};

/* 连击检测参数 */
#define DOUBLE_CLICK_THRESHOLD_MS  500   /* 双击间隔阈值 */

/* ============================================================================
 * 内部函数
 * ============================================================================ */

static uint32_t get_current_time_ms(void)
{
    return rt_tick_get() * 1000 / RT_TICK_PER_SECOND;
}

static void init_random_state(void)
{
    widget_config_t config;
    widget_storage_get_active(&config);
    
    g_widget_mgr.random_state.max_option = config.config.random.max_option;
    g_widget_mgr.random_state.current_value = 0;
    g_widget_mgr.random_state.in_setting_mode = false;
    g_widget_mgr.random_state.setting_flash_on = true;
    g_widget_mgr.random_state.click_count = 0;
    g_widget_mgr.random_state.last_click_time = 0;
}

static void init_offwork_state(void)
{
    widget_config_t config;
    widget_storage_get_active(&config);
    
    g_widget_mgr.offwork_state.target_hour = config.config.offwork.target_hour;
    g_widget_mgr.offwork_state.target_minute = config.config.offwork.target_minute;
    
    /* 如果目标时间为0，设置默认值18:00 */
    if (g_widget_mgr.offwork_state.target_hour == 0 && 
        g_widget_mgr.offwork_state.target_minute == 0) {
        g_widget_mgr.offwork_state.target_hour = 18;
        g_widget_mgr.offwork_state.target_minute = 0;
    }
    
    g_widget_mgr.offwork_state.remaining_seconds = 0;
    g_widget_mgr.offwork_state.is_off_work = false;
    g_widget_mgr.offwork_state.in_setting_mode = false;
    g_widget_mgr.offwork_state.setting_step = 0;
    g_widget_mgr.offwork_state.temp_hour = g_widget_mgr.offwork_state.target_hour;
    g_widget_mgr.offwork_state.temp_minute = g_widget_mgr.offwork_state.target_minute;
    g_widget_mgr.offwork_state.click_count = 0;
    g_widget_mgr.offwork_state.last_click_time = 0;
    
    /* 立即更新一次倒计时状态 */
    widget_offwork_tick();
}

static void init_food_state(void)
{
    memset(&g_widget_mgr.food_state, 0, sizeof(g_widget_mgr.food_state));
    strncpy(g_widget_mgr.food_state.current_food, "点击选择", 
            sizeof(g_widget_mgr.food_state.current_food) - 1);
    g_widget_mgr.food_state.has_result = false;
}

static void init_selector_state(void)
{
    g_widget_mgr.selector_state.current_index = 0;
    g_widget_mgr.selector_state.active = false;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

int widget_manager_init(void)
{
    if (g_widget_mgr.initialized) {
        return 0;
    }
    
    /* 先初始化存储模块 */
    int ret = widget_storage_init();
    if (ret != 0) {
        return ret;
    }
    
    /* 创建互斥锁 */
    g_widget_mgr.lock = rt_mutex_create("wdg_mgr", RT_IPC_FLAG_PRIO);
    if (!g_widget_mgr.lock) {
        return -RT_ENOMEM;
    }
    
    /* 初始化各工具状态 */
    init_random_state();
    init_offwork_state();
    init_food_state();
    init_selector_state();
    
    /* 设置随机种子 */
    srand(rt_tick_get());
    
    g_widget_mgr.initialized = true;
    
    return 0;
}

int widget_manager_deinit(void)
{
    if (!g_widget_mgr.initialized) {
        return 0;
    }
    
    if (g_widget_mgr.lock) {
        rt_mutex_delete(g_widget_mgr.lock);
        g_widget_mgr.lock = NULL;
    }
    
    g_widget_mgr.initialized = false;
    return 0;
}

widget_type_t widget_manager_get_active_type(void)
{
    widget_config_t config;
    if (widget_storage_get_active(&config) != 0) {
        return WIDGET_TYPE_RANDOM_NUMBER;
    }
    return config.type;
}

int widget_manager_set_active_type(widget_type_t type)
{
    widget_config_t config;
    widget_storage_get_active(&config);
    config.type = type;
    
    int ret = widget_storage_set_active(&config);
    
    /* 重新初始化对应工具状态 */
    switch (type) {
        case WIDGET_TYPE_RANDOM_NUMBER:
            init_random_state();
            break;
        case WIDGET_TYPE_OFF_WORK_COUNTDOWN:
            init_offwork_state();
            break;
        case WIDGET_TYPE_WHAT_TO_EAT:
            init_food_state();
            break;
        default:
            break;
    }
    
    return ret;
}

/* ============================================================================
 * 随机数工具实现
 * ============================================================================ */

int widget_random_get_state(widget_random_state_t *state)
{
    if (!state) return -RT_EINVAL;
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    *state = g_widget_mgr.random_state;
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    return 0;
}

int32_t widget_random_generate(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    int max_val = widget_get_random_max_value(g_widget_mgr.random_state.max_option);
    int32_t result = (rand() % max_val) + 1;  /* 1 ~ max_val */
    g_widget_mgr.random_state.current_value = result;
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    
    rt_kprintf("[Random] Generated: %d (max=%d)\n", result, max_val);
    return result;
}

int widget_random_handle_key(int key_idx)
{
    if (key_idx != 2) return 0;  /* 只处理第三个板块的按键 */
    
    uint32_t now = get_current_time_ms();
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    /* 连击检测 */
    if (now - g_widget_mgr.random_state.last_click_time < DOUBLE_CLICK_THRESHOLD_MS) {
        g_widget_mgr.random_state.click_count++;
    } else {
        g_widget_mgr.random_state.click_count = 1;
    }
    g_widget_mgr.random_state.last_click_time = now;
    
    bool in_setting = g_widget_mgr.random_state.in_setting_mode;
    uint8_t clicks = g_widget_mgr.random_state.click_count;
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    
    /* 处理按键 */
    if (in_setting) {
        /* 设置模式下，单击退出设置 */
        widget_random_exit_setting();
    } else {
        if (clicks >= 2) {
            /* 双击进入设置模式 */
            if (g_widget_mgr.lock) {
                rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
            }
            g_widget_mgr.random_state.in_setting_mode = true;
            g_widget_mgr.random_state.click_count = 0;
            rt_kprintf("[Random] Enter setting mode\n");
            if (g_widget_mgr.lock) {
                rt_mutex_release(g_widget_mgr.lock);
            }
        } else {
            /* 延迟判断：等待看是否有第二次点击 */
            /* 实际单击生成随机数会在下次tick中处理，或立即处理 */
            /* 这里简化处理：单击直接生成 */
            widget_random_generate();
        }
    }
    
    return 0;
}

void widget_random_change_max(int delta)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    int new_option = (int)g_widget_mgr.random_state.max_option + delta;
    if (new_option < 0) {
        new_option = RANDOM_MAX_OPTIONS - 1;
    } else if (new_option >= RANDOM_MAX_OPTIONS) {
        new_option = 0;
    }
    g_widget_mgr.random_state.max_option = (random_max_option_t)new_option;
    
    rt_kprintf("[Random] Max changed to: %d\n", 
               widget_get_random_max_value(g_widget_mgr.random_state.max_option));
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

void widget_random_exit_setting(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    g_widget_mgr.random_state.in_setting_mode = false;
    
    /* 保存配置 */
    widget_config_t config;
    widget_storage_get_active(&config);
    config.config.random.max_option = g_widget_mgr.random_state.max_option;
    widget_storage_set_active(&config);
    
    rt_kprintf("[Random] Exit setting mode, saved max=%d\n",
               widget_get_random_max_value(g_widget_mgr.random_state.max_option));
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

/* ============================================================================
 * 下班倒计时实现
 * ============================================================================ */

int widget_offwork_get_state(widget_offwork_state_t *state)
{
    if (!state) return -RT_EINVAL;
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    *state = g_widget_mgr.offwork_state;
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    return 0;
}

void widget_offwork_tick(void)
{
    if (g_widget_mgr.offwork_state.in_setting_mode) {
        return;  /* 设置模式下不更新 */
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (!tm_info) return;
    
    /* 计算当前时间的秒数 */
    int32_t current_seconds = tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec;
    
    /* 计算目标时间的秒数 */
    int32_t target_seconds = g_widget_mgr.offwork_state.target_hour * 3600 + 
                             g_widget_mgr.offwork_state.target_minute * 60;
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    g_widget_mgr.offwork_state.remaining_seconds = target_seconds - current_seconds;
    g_widget_mgr.offwork_state.is_off_work = (g_widget_mgr.offwork_state.remaining_seconds <= 0);
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

int widget_offwork_handle_key(int key_idx)
{
    if (key_idx != 2) return 0;
    
    uint32_t now = get_current_time_ms();
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    /* 连击检测 */
    if (now - g_widget_mgr.offwork_state.last_click_time < DOUBLE_CLICK_THRESHOLD_MS) {
        g_widget_mgr.offwork_state.click_count++;
    } else {
        g_widget_mgr.offwork_state.click_count = 1;
    }
    g_widget_mgr.offwork_state.last_click_time = now;
    
    bool in_setting = g_widget_mgr.offwork_state.in_setting_mode;
    uint8_t clicks = g_widget_mgr.offwork_state.click_count;
    uint8_t step = g_widget_mgr.offwork_state.setting_step;
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    
    if (in_setting) {
        /* 设置模式下，单击进入下一步 */
        widget_offwork_next_step();
    } else {
        if (clicks >= 2) {
            /* 双击进入设置模式 */
            if (g_widget_mgr.lock) {
                rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
            }
            g_widget_mgr.offwork_state.in_setting_mode = true;
            g_widget_mgr.offwork_state.setting_step = 0;
            g_widget_mgr.offwork_state.temp_hour = g_widget_mgr.offwork_state.target_hour;
            g_widget_mgr.offwork_state.temp_minute = g_widget_mgr.offwork_state.target_minute;
            g_widget_mgr.offwork_state.click_count = 0;
            rt_kprintf("[OffWork] Enter setting mode\n");
            if (g_widget_mgr.lock) {
                rt_mutex_release(g_widget_mgr.lock);
            }
        }
        /* 普通模式下单击无操作 */
    }
    
    return 0;
}

void widget_offwork_change_time(int delta)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    if (g_widget_mgr.offwork_state.setting_step == 0) {
        /* 调整小时 */
        int new_hour = (int)g_widget_mgr.offwork_state.temp_hour + delta;
        if (new_hour < 0) new_hour = 23;
        if (new_hour > 23) new_hour = 0;
        g_widget_mgr.offwork_state.temp_hour = (uint8_t)new_hour;
    } else {
        /* 调整分钟 */
        int new_min = (int)g_widget_mgr.offwork_state.temp_minute + delta;
        if (new_min < 0) new_min = 59;
        if (new_min > 59) new_min = 0;
        g_widget_mgr.offwork_state.temp_minute = (uint8_t)new_min;
    }
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

void widget_offwork_next_step(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    if (g_widget_mgr.offwork_state.setting_step == 0) {
        /* 从小时设置进入分钟设置 */
        g_widget_mgr.offwork_state.setting_step = 1;
        rt_kprintf("[OffWork] Setting step -> minute\n");
    } else {
        /* 从分钟设置保存并退出 */
        if (g_widget_mgr.lock) {
            rt_mutex_release(g_widget_mgr.lock);
        }
        widget_offwork_save_setting();
        return;
    }
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

void widget_offwork_save_setting(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    g_widget_mgr.offwork_state.target_hour = g_widget_mgr.offwork_state.temp_hour;
    g_widget_mgr.offwork_state.target_minute = g_widget_mgr.offwork_state.temp_minute;
    g_widget_mgr.offwork_state.in_setting_mode = false;
    g_widget_mgr.offwork_state.setting_step = 0;
    
    /* 保存到Flash */
    widget_config_t config;
    widget_storage_get_active(&config);
    config.config.offwork.target_hour = g_widget_mgr.offwork_state.target_hour;
    config.config.offwork.target_minute = g_widget_mgr.offwork_state.target_minute;
    widget_storage_set_active(&config);
    
    rt_kprintf("[OffWork] Saved target=%02d:%02d\n",
               g_widget_mgr.offwork_state.target_hour,
               g_widget_mgr.offwork_state.target_minute);
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    
    /* 立即更新倒计时状态 */
    widget_offwork_tick();
}

/* ============================================================================
 * 今天吃什么实现
 * ============================================================================ */

int widget_food_get_state(widget_food_state_t *state)
{
    if (!state) return -RT_EINVAL;
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    *state = g_widget_mgr.food_state;
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    return 0;
}

const char* widget_food_random_pick(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    int index = rand() % FOOD_MENU_COUNT;
    const char* food = g_food_menu[index];
    
    strncpy(g_widget_mgr.food_state.current_food, food,
            sizeof(g_widget_mgr.food_state.current_food) - 1);
    g_widget_mgr.food_state.has_result = true;
    
    rt_kprintf("[Food] Picked: %s (index=%d)\n", food, index);
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    
    return food;
}

int widget_food_handle_key(int key_idx)
{
    if (key_idx != 2) return 0;
    
    /* 单击直接随机选择 */
    widget_food_random_pick();
    return 0;
}

/* ============================================================================
 * 小工具选择器实现
 * ============================================================================ */

int widget_selector_get_state(widget_selector_state_t *state)
{
    if (!state) return -RT_EINVAL;
    
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    *state = g_widget_mgr.selector_state;
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
    return 0;
}

void widget_selector_activate(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    g_widget_mgr.selector_state.active = true;
    /* 默认选中第一个小工具(跳过NONE) */
    g_widget_mgr.selector_state.current_index = 0;
    
    rt_kprintf("[Selector] Activated\n");
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

void widget_selector_deactivate(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    g_widget_mgr.selector_state.active = false;
    rt_kprintf("[Selector] Deactivated\n");
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

void widget_selector_move(int delta)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    /* 可选小工具数量 (排除NONE) */
    int widget_count = WIDGET_TYPE_MAX - 1;  /* 3个工具 */
    
    int new_index = g_widget_mgr.selector_state.current_index + delta;
    if (new_index < 0) {
        new_index = widget_count - 1;
    } else if (new_index >= widget_count) {
        new_index = 0;
    }
    g_widget_mgr.selector_state.current_index = new_index;
    
    rt_kprintf("[Selector] Move to index=%d (%s)\n", new_index,
               widget_get_type_name(widget_selector_get_current_type()));
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}

widget_type_t widget_selector_confirm(void)
{
    widget_type_t type = widget_selector_get_current_type();
    
    rt_kprintf("[Selector] Confirmed: %s\n", widget_get_type_name(type));
    
    /* 设置为激活的小工具 */
    widget_manager_set_active_type(type);
    
    /* 关闭选择器 */
    widget_selector_deactivate();
    
    return type;
}

widget_type_t widget_selector_get_current_type(void)
{
    /* index 0-2 对应 WIDGET_TYPE_RANDOM_NUMBER 到 WIDGET_TYPE_WHAT_TO_EAT */
    return (widget_type_t)(g_widget_mgr.selector_state.current_index + 1);
}

/* ============================================================================
 * 定时器回调
 * ============================================================================ */

void widget_timer_tick(void)
{
    widget_type_t active = widget_manager_get_active_type();
    
    if (active == WIDGET_TYPE_OFF_WORK_COUNTDOWN) {
        widget_offwork_tick();
    }
}

void widget_setting_flash_tick(void)
{
    if (g_widget_mgr.lock) {
        rt_mutex_take(g_widget_mgr.lock, RT_WAITING_FOREVER);
    }
    
    /* 切换闪烁状态 */
    g_widget_mgr.random_state.setting_flash_on = !g_widget_mgr.random_state.setting_flash_on;
    
    if (g_widget_mgr.lock) {
        rt_mutex_release(g_widget_mgr.lock);
    }
}
