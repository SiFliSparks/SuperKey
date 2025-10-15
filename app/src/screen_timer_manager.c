#include "screen_timer_manager.h"
#include "screen_core.h"
#include <rtthread.h>
#include <string.h>

/* 静态实例 */
static screen_timer_manager_t g_timer_mgr = {0};

/* 默认定时器配置 */
static const screen_timer_config_t default_configs[SCREEN_TIMER_MAX] = {
    {SCREEN_TIMER_CLOCK,   1000,  true,  true,  "clock"},
    {SCREEN_TIMER_WEATHER, 30000, true,  true,  "weather"},
    {SCREEN_TIMER_STOCK,   10000, true,  true,  "stock"},
    {SCREEN_TIMER_SYSTEM,  2000,  true,  true,  "system"},
    {SCREEN_TIMER_SENSOR,  5000,  true,  true,  "sensor"},
    {SCREEN_TIMER_CLEANUP, 60000, true,  true,  "cleanup"}
};

/* 极简的ISR安全定时器回调 - 避免所有同步操作 */
static void safe_timer_callback(void *parameter)
{
    screen_timer_type_t type = (screen_timer_type_t)((uintptr_t)parameter);
    
    if (type >= SCREEN_TIMER_MAX) {
        return;
    }
    
    /* 极简方案：只更新触发计数，不使用任何锁或中断控制 
     * 在32位系统上，简单的整数递增通常是原子的 */
    g_timer_mgr.trigger_counts[type]++;
    g_timer_mgr.last_trigger_times[type] = rt_tick_get();
    
    /* 根据定时器类型发送相应消息 - 确保screen_core的消息发送是ISR安全的 */
    switch (type) {
        case SCREEN_TIMER_CLOCK:
            screen_core_post_update_time();
            break;
            
        case SCREEN_TIMER_WEATHER:
            screen_core_post_update_weather(NULL);
            break;
            
        case SCREEN_TIMER_STOCK:
            screen_core_post_update_stock(NULL);
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
            
        default:
            break;
    }
}
// 添加新的函数：专门为L2层级启动时钟定时器
int screen_timer_start_l2_timers(void)
{
    int ret = 0;
    // L2时间详情页面需要时钟定时器保持运行
    ret |= screen_timer_start(SCREEN_TIMER_CLOCK);
    
    return ret;
}
int screen_timer_manager_init(void)
{
    if (g_timer_mgr.initialized) {
        return 0;
    }
    
    /* 创建互斥锁 - 只用于非ISR上下文的API */
    g_timer_mgr.lock = rt_mutex_create("timer_mgr_lock", RT_IPC_FLAG_PRIO);
    if (!g_timer_mgr.lock) {
        return -RT_ENOMEM;
    }
    
    /* 复制默认配置 */
    memcpy(g_timer_mgr.configs, default_configs, sizeof(default_configs));
    
    /* 创建所有定时器但不启动 */
    for (int i = 0; i < SCREEN_TIMER_MAX; i++) {
        screen_timer_config_t *config = &g_timer_mgr.configs[i];
        
        g_timer_mgr.timers[i] = rt_timer_create(
            config->name,
            safe_timer_callback,
            (void*)((uintptr_t)i),  /* 传递定时器类型作为参数 */
            rt_tick_from_millisecond(config->interval_ms),
            config->periodic ? RT_TIMER_FLAG_PERIODIC : RT_TIMER_FLAG_ONE_SHOT
        );
        
        if (!g_timer_mgr.timers[i]) {
            /* 清理已创建的定时器 */
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
        
        /* 简单初始化，不使用复杂的同步 */
        g_timer_mgr.trigger_counts[i] = 0;
        g_timer_mgr.last_trigger_times[i] = 0;
    }
    
    g_timer_mgr.initialized = true;
    
    return 0;
}

int screen_timer_manager_deinit(void)
{
    if (!g_timer_mgr.initialized) {
        return 0;
    }
    
    /* 停止并删除所有定时器 */
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
    
    /* 简单清零，不使用复杂的同步 */
    memset(g_timer_mgr.trigger_counts, 0, sizeof(g_timer_mgr.trigger_counts));
    memset(g_timer_mgr.last_trigger_times, 0, sizeof(g_timer_mgr.last_trigger_times));
    
    g_timer_mgr.initialized = false;
    
    return 0;
}

int screen_timer_start(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    /* 使用互斥锁保护非ISR API */
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    int ret = 0;
    if (g_timer_mgr.configs[type].enabled && g_timer_mgr.timers[type]) {
        rt_err_t result = rt_timer_start(g_timer_mgr.timers[type]);
        if (result == RT_EOK) {
        } else {
            ret = -RT_ERROR;
        }
    } else {
        ret = -RT_ERROR;
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return ret;
}

int screen_timer_stop(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    /* 使用互斥锁保护非ISR API */
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    int ret = 0;
    if (g_timer_mgr.timers[type]) {
        rt_err_t result = rt_timer_stop(g_timer_mgr.timers[type]);
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return ret;
}

int screen_timer_restart(screen_timer_type_t type)
{
    screen_timer_stop(type);
    rt_thread_mdelay(10);  /* 短暂延迟确保停止完成 */
    return screen_timer_start(type);
}

int screen_timer_start_group1_timers(void)
{
    int ret = 0;
    ret |= screen_timer_start(SCREEN_TIMER_CLOCK);
    ret |= screen_timer_start(SCREEN_TIMER_WEATHER);
    ret |= screen_timer_start(SCREEN_TIMER_STOCK);
    ret |= screen_timer_start(SCREEN_TIMER_SENSOR);
    
    return ret;
}

int screen_timer_start_group2_timers(void)
{
    int ret = 0;
    ret |= screen_timer_start(SCREEN_TIMER_SYSTEM);
    
    return ret;
}

int screen_timer_stop_all_group_timers(void)
{
    int ret = 0;
    ret |= screen_timer_stop(SCREEN_TIMER_CLOCK);
    ret |= screen_timer_stop(SCREEN_TIMER_WEATHER);
    ret |= screen_timer_stop(SCREEN_TIMER_STOCK);
    ret |= screen_timer_stop(SCREEN_TIMER_SYSTEM);
    ret |= screen_timer_stop(SCREEN_TIMER_SENSOR);
    /* 保持清理定时器运行 */
    
    return ret;
}

int screen_timer_set_interval(screen_timer_type_t type, uint32_t interval_ms)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    /* 使用互斥锁保护非ISR API */
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    g_timer_mgr.configs[type].interval_ms = interval_ms;
    
    /* 如果定时器正在运行，重启以应用新间隔 */
    if (g_timer_mgr.timers[type]) {
        rt_mutex_release(g_timer_mgr.lock);
        return screen_timer_restart(type);
    }
    
    rt_mutex_release(g_timer_mgr.lock);
    return 0;
}

int screen_timer_enable(screen_timer_type_t type, bool enabled)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return -RT_EINVAL;
    }
    
    /* 使用互斥锁保护非ISR API */
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    g_timer_mgr.configs[type].enabled = enabled;
    rt_mutex_release(g_timer_mgr.lock);
    return 0;
}

bool screen_timer_is_running(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX || !g_timer_mgr.timers[type]) {
        return false;
    }
    
    /* 使用互斥锁保护非ISR API，但简化逻辑避免复杂的状态检查 */
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    bool enabled = g_timer_mgr.configs[type].enabled;
    bool timer_exists = (g_timer_mgr.timers[type] != NULL);
    rt_mutex_release(g_timer_mgr.lock);
    
    return enabled && timer_exists;
}

uint32_t screen_timer_get_trigger_count(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return 0;
    }
    
    /* 简单读取，在32位系统上整数读取通常是原子的 */
    return g_timer_mgr.trigger_counts[type];
}

rt_tick_t screen_timer_get_last_trigger_time(screen_timer_type_t type)
{
    if (!g_timer_mgr.initialized || type >= SCREEN_TIMER_MAX) {
        return 0;
    }
    
    /* 简单读取，在32位系统上整数读取通常是原子的 */
    return g_timer_mgr.last_trigger_times[type];
}

int screen_timer_get_status_string(char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size < 200 || !g_timer_mgr.initialized) {
        return -RT_EINVAL;
    }
    
    int offset = 0;
    offset += rt_snprintf(buffer + offset, buffer_size - offset, "Timer Status:\n");
    
    /* 使用互斥锁保护配置访问 */
    rt_mutex_take(g_timer_mgr.lock, RT_WAITING_FOREVER);
    
    for (int i = 0; i < SCREEN_TIMER_MAX && offset < buffer_size - 50; i++) {
        bool running = g_timer_mgr.configs[i].enabled && (g_timer_mgr.timers[i] != NULL);
        uint32_t interval = g_timer_mgr.configs[i].interval_ms;
        
        /* 简单读取触发次数 */
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