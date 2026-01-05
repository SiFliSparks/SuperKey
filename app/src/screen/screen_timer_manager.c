#include "../screen/screen_timer_manager.h"
#include "../screen/screen_core.h"
#include "../mp3/mp3_screen_ui.h"
#include <rtthread.h>
#include <string.h>

static screen_timer_manager_t g_timer_mgr = {0};

/**
 * @brief 默认定时器配置
 * 
 * 修复版本 - 添加了 MP3 定时器配置
 */
static const screen_timer_config_t default_configs[SCREEN_TIMER_MAX] = {
    {SCREEN_TIMER_CLOCK,     500,   true,  true,  "clock"},
    {SCREEN_TIMER_WEATHER,   500,   true,  true,  "weather"},
    {SCREEN_TIMER_SYSTEM,    500,   true,  true,  "system"},
    {SCREEN_TIMER_SENSOR,    500,   true,  true,  "sensor"},
    {SCREEN_TIMER_MUYU,      200,   true,  true,  "muyu"},
    {SCREEN_TIMER_STOPWATCH, 100,   true,  true,  "stopwatch"},
    {SCREEN_TIMER_TOMATO,    500,   true,  true,  "tomato"},
    {SCREEN_TIMER_MP3,       500,   true,  true,  "mp3"},      /* 新增 */
    {SCREEN_TIMER_CLEANUP,   60000, true,  true,  "cleanup"}
};

/**
 * @brief 安全的定时器回调函数
 */
static void safe_timer_callback(void *parameter)
{
    screen_timer_type_t type = (screen_timer_type_t)((uintptr_t)parameter);
    
    if (type >= SCREEN_TIMER_MAX) {
        return;
    }
    
    g_timer_mgr.trigger_counts[type]++;
    g_timer_mgr.last_trigger_times[type] = rt_tick_get();
    
    switch (type) {
        case SCREEN_TIMER_CLOCK:
            screen_core_post_update_time();
            break;
            
        case SCREEN_TIMER_WEATHER:
            screen_core_post_update_weather(NULL);
            break;
            
        case SCREEN_TIMER_SYSTEM:
            screen_core_post_update_system(NULL);
            break;
            
        case SCREEN_TIMER_SENSOR:
            screen_core_post_update_weather(NULL);
            break;
            
        case SCREEN_TIMER_CLEANUP:
            screen_core_post_cleanup_request();
            break;
            
        case SCREEN_TIMER_MUYU:
            screen_core_post_update_time();
            break;
            
        case SCREEN_TIMER_STOPWATCH:
            screen_core_post_update_time();
            break;
            
        case SCREEN_TIMER_TOMATO:
            screen_core_post_update_time();
            break;
            
        case SCREEN_TIMER_MP3:
            /* MP3 定时器触发 UI 更新 */
            screen_core_post_update_time();
            break;
            
        default:
            break;
    }
}

/**
 * @brief 启动 L2 木鱼页面定时器
 */
int screen_timer_start_l2_muyu_timers(void)
{
    return screen_timer_start(SCREEN_TIMER_MUYU);
}

/**
 * @brief 启动 L2 层级定时器
 */
int screen_timer_start_l2_timers(void)
{
    int ret = 0;
    ret |= screen_timer_start(SCREEN_TIMER_CLOCK);
    return ret;
}

/**
 * @brief 启动 Group6 MP3 相关定时器
 */
int screen_timer_start_group6_timers(void)
{
    return screen_timer_start(SCREEN_TIMER_MP3);
}

/**
 * @brief 初始化定时器管理器
 */
int screen_timer_manager_init(void)
{
    if (g_timer_mgr.initialized) {
        return 0;
    }
    
    g_timer_mgr.lock = rt_mutex_create("timer_mgr_lock", RT_IPC_FLAG_PRIO);
    if (!g_timer_mgr.lock) {
        return -RT_ENOMEM;
    }
    
    memcpy(g_timer_mgr.configs, default_configs, sizeof(default_configs));
    
    for (int i = 0; i < SCREEN_TIMER_MAX; i++) {
        screen_timer_config_t *config = &g_timer_mgr.configs[i];
        
        g_timer_mgr.timers[i] = rt_timer_create(
            config->name,
            safe_timer_callback,
            (void*)((uintptr_t)i),
            rt_tick_from_millisecond(config->interval_ms),
            config->periodic ? RT_TIMER_FLAG_PERIODIC : RT_TIMER_FLAG_ONE_SHOT
        );
        
        if (!g_timer_mgr.timers[i]) {
            /* 创建失败，清理已创建的定时器 */
            for (int j = 0; j < i; j++) {
                if (g_timer_mgr.timers[j]) {
                    rt_timer_delete(g_timer_mgr.timers[j]);
                    g_timer_mgr.timers[j] = NULL;
                }
            }
            rt_mutex_delete(g_timer_mgr.lock);
            g_timer_mgr.lock = NULL;
            return -RT_ENOMEM;
        }
        
        g_timer_mgr.trigger_counts[i] = 0;
        g_timer_mgr.last_trigger_times[i] = 0;
    }
    
    g_timer_mgr.initialized = true;
    rt_kprintf("[TimerMgr] Initialized with %d timers\n", SCREEN_TIMER_MAX);
    
    return 0;
}

/**
 * @brief 反初始化定时器管理器
 */
int screen_timer_manager_deinit(void)
{
    if (!g_timer_mgr.initialized) {
        return 0;
    }
    
    for (int i = 0; i < SCREEN_TIMER_MAX; i++) {
        if (g_timer_mgr.timers[i]) {
            rt_timer_stop(g_timer_mgr.timers[i]);
            rt_timer_delete(g_timer_mgr.timers[i]);
            g_timer_mgr.timers[i] = NULL;
        }
    }
    
    if (g_timer_mgr.lock) {
        rt_mutex_delete(g_timer_mgr.lock);
        g_timer_mgr.lock = NULL;
    }
    
    memset(g_timer_mgr.trigger_counts, 0, sizeof(g_timer_mgr.trigger_counts));
    memset(g_timer_mgr.last_trigger_times, 0, sizeof(g_timer_mgr.last_trigger_times));
    
    g_timer_mgr.initialized = false;
    rt_kprintf("[TimerMgr] Deinitialized\n");
    
    return 0;
}

/**
 * @brief 启动指定定时器
 */
int screen_timer_start(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    int ret = 0;
    if (g_timer_mgr.configs[type].enabled && g_timer_mgr.timers[type]) {
        rt_err_t result = rt_timer_start(g_timer_mgr.timers[type]);
        if (result != RT_EOK) {
            ret = -RT_ERROR;
        }
    } else {
        ret = -RT_ERROR;
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return ret;
}

/**
 * @brief 停止指定定时器
 */
int screen_timer_stop(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    if (g_timer_mgr.timers[type]) {
        rt_timer_stop(g_timer_mgr.timers[type]);
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return 0;
}

/**
 * @brief 重启指定定时器
 */
int screen_timer_restart(screen_timer_type_t type)
{
    screen_timer_stop(type);
    rt_thread_mdelay(10);
    return screen_timer_start(type);
}

/**
 * @brief 启动 Group1 相关定时器
 */
int screen_timer_start_group1_timers(void)
{
    int ret = 0;
    ret |= screen_timer_start(SCREEN_TIMER_CLOCK);
    ret |= screen_timer_start(SCREEN_TIMER_WEATHER);
    ret |= screen_timer_start(SCREEN_TIMER_SENSOR);
    return ret;
}

/**
 * @brief 启动 Group2 相关定时器
 */
int screen_timer_start_group2_timers(void)
{
    int ret = 0;
    ret |= screen_timer_start(SCREEN_TIMER_SYSTEM);
    return ret;
}

/**
 * @brief 停止所有组定时器
 */
int screen_timer_stop_all_group_timers(void)
{
    int ret = 0;
    ret |= screen_timer_stop(SCREEN_TIMER_CLOCK);
    ret |= screen_timer_stop(SCREEN_TIMER_WEATHER);
    ret |= screen_timer_stop(SCREEN_TIMER_SYSTEM);
    ret |= screen_timer_stop(SCREEN_TIMER_SENSOR);
    ret |= screen_timer_stop(SCREEN_TIMER_MP3);
    return ret;
}

/**
 * @brief 设置定时器间隔
 */
int screen_timer_set_interval(screen_timer_type_t type, uint32_t interval_ms)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    g_timer_mgr.configs[type].interval_ms = interval_ms;
    
    if (g_timer_mgr.timers[type]) {
        rt_mutex_release(g_timer_mgr.lock);
        return screen_timer_restart(type);
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return 0;
}

/**
 * @brief 启用/禁用定时器
 */
int screen_timer_enable(screen_timer_type_t type, bool enabled)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    g_timer_mgr.configs[type].enabled = enabled;
    rt_mutex_release(g_timer_mgr.lock);
    return 0;
}

/**
 * @brief 检查定时器是否运行中
 */
bool screen_timer_is_running(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX || !g_timer_mgr.timers[type]) {
        return false;
    }
    
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    bool enabled = g_timer_mgr.configs[type].enabled;
    bool timer_exists = (g_timer_mgr.timers[type] != NULL);
    rt_mutex_release(g_timer_mgr.lock);
    
    return enabled && timer_exists;
}

/**
 * @brief 获取定时器触发计数
 */
uint32_t screen_timer_get_trigger_count(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return 0;
    }
    
    return g_timer_mgr.trigger_counts[type];
}

/**
 * @brief 获取定时器最后触发时间
 */
rt_tick_t screen_timer_get_last_trigger_time(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return 0;
    }
    
    return g_timer_mgr.last_trigger_times[type];
}

/**
 * @brief 获取定时器状态字符串
 */
int screen_timer_get_status_string(char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size < 200 || !g_timer_mgr.initialized) {
        return -RT_EINVAL;
    }
    
    int offset = 0;
    offset += rt_snprintf(buffer + offset, buffer_size - offset, "Timer Status:\n");
    
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    for (int i = 0; i < SCREEN_TIMER_MAX && offset < buffer_size - 50; i++) {
        bool running = g_timer_mgr.configs[i].enabled && (g_timer_mgr.timers[i] != NULL);
        uint32_t interval = g_timer_mgr.configs[i].interval_ms;
        uint32_t triggers = g_timer_mgr.trigger_counts[i];
        
        offset += rt_snprintf(buffer + offset, buffer_size - offset,
                             "  %s: %s, %ums, %u triggers\n",
                             g_timer_mgr.configs[i].name,
                             running ? "RUN" : "STOP",
                             interval, triggers);
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return 0;
}