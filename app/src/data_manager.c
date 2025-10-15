#include "data_manager.h"
#include <string.h>
#include <stdbool.h>
#include "event_bus.h"

static struct {
    weather_data_t weather;
    stock_data_t stock;
    system_monitor_data_t system;
    
    rt_tick_t weather_update_tick;
    rt_tick_t stock_update_tick;
    rt_tick_t system_update_tick;
    
    uint32_t cleanup_count;
    rt_tick_t last_cleanup_tick;
    
    rt_mutex_t lock;
    
    bool initialized;
} g_data_store = {0};

static bool is_data_expired(rt_tick_t last_update_tick)
{
    if (last_update_tick == 0) return true;
    
    rt_tick_t now = rt_tick_get();
    rt_tick_t timeout_ticks = rt_tick_from_millisecond(DATA_TIMEOUT_MS);
    return (now - last_update_tick) > timeout_ticks;
}

static uint32_t get_data_age_seconds(rt_tick_t last_update_tick)
{
    if (last_update_tick == 0) return UINT32_MAX;
    
    rt_tick_t now = rt_tick_get();
    rt_tick_t age_ticks = now - last_update_tick;
    return age_ticks / RT_TICK_PER_SECOND;
}

int data_manager_update_weather(const weather_data_t *data)
{
    if (!data || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    g_data_store.weather = *data;
    g_data_store.weather_update_tick = rt_tick_get();
    
    rt_mutex_release(g_data_store.lock);
    return 0;
}

int data_manager_update_stock(const stock_data_t *data)
{
    if (!data || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    g_data_store.stock = *data;
    g_data_store.stock_update_tick = rt_tick_get();
    
    rt_mutex_release(g_data_store.lock);
    return 0;
}

int data_manager_update_system(const system_monitor_data_t *data)
{
    if (!data || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    g_data_store.system = *data;
    g_data_store.system_update_tick = rt_tick_get();
    
    rt_mutex_release(g_data_store.lock);
    return 0;
}

int data_manager_get_weather(weather_data_t *data)
{
    if (!data || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    if (is_data_expired(g_data_store.weather_update_tick)) {
        g_data_store.weather.valid = false;
    }
    
    *data = g_data_store.weather;
    rt_mutex_release(g_data_store.lock);
    
    return data->valid ? 0 : -RT_EEMPTY;
}

int data_manager_get_stock(stock_data_t *data)
{
    if (!data || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    if (is_data_expired(g_data_store.stock_update_tick)) {
        g_data_store.stock.valid = false;
    }
    
    *data = g_data_store.stock;
    rt_mutex_release(g_data_store.lock);
    
    return data->valid ? 0 : -RT_EEMPTY;
}

int data_manager_get_system(system_monitor_data_t *data)
{
    if (!data || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    if (is_data_expired(g_data_store.system_update_tick)) {
        g_data_store.system.valid = false;
    }
    
    *data = g_data_store.system;
    rt_mutex_release(g_data_store.lock);
    
    return data->valid ? 0 : -RT_EEMPTY;
}

int data_manager_cleanup_expired_data(void)
{
    if (!g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    int cleaned = 0;
    rt_tick_t now = rt_tick_get();
    
    if (is_data_expired(g_data_store.weather_update_tick) && g_data_store.weather.valid) {
        g_data_store.weather.valid = false;
        cleaned++;
    }
    
    if (is_data_expired(g_data_store.stock_update_tick) && g_data_store.stock.valid) {
        g_data_store.stock.valid = false;
        cleaned++;
    }
    
    if (is_data_expired(g_data_store.system_update_tick) && g_data_store.system.valid) {
        g_data_store.system.valid = false;
        cleaned++;
    }
    
    if (cleaned > 0) {
        g_data_store.cleanup_count += cleaned;
    }
    
    g_data_store.last_cleanup_tick = now;
    rt_mutex_release(g_data_store.lock);
    
    return cleaned;
}

int data_manager_reset_all_data(void)
{
    if (!g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    memset(&g_data_store.weather, 0, sizeof(weather_data_t));
    memset(&g_data_store.stock, 0, sizeof(stock_data_t));
    memset(&g_data_store.system, 0, sizeof(system_monitor_data_t));
    
    g_data_store.weather.valid = false;
    g_data_store.stock.valid = false;
    g_data_store.system.valid = false;
    
    g_data_store.weather_update_tick = 0;
    g_data_store.stock_update_tick = 0;
    g_data_store.system_update_tick = 0;
    
    rt_mutex_release(g_data_store.lock);
    return 0;
}

int data_manager_get_data_status(char *status_buf, size_t buf_size)
{
    if (!status_buf || buf_size < 200 || !g_data_store.initialized) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    uint32_t weather_age = get_data_age_seconds(g_data_store.weather_update_tick);
    uint32_t stock_age = get_data_age_seconds(g_data_store.stock_update_tick);
    uint32_t system_age = get_data_age_seconds(g_data_store.system_update_tick);
    rt_mutex_release(g_data_store.lock);
    return 0;
}

bool data_manager_is_data_fresh(const char *type)
{
    if (!type || !g_data_store.initialized) {
        return false;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    bool fresh = false;
    if (strcmp(type, "weather") == 0) {
        fresh = !is_data_expired(g_data_store.weather_update_tick) && g_data_store.weather.valid;
    } else if (strcmp(type, "stock") == 0) {
        fresh = !is_data_expired(g_data_store.stock_update_tick) && g_data_store.stock.valid;
    } else if (strcmp(type, "system") == 0) {
        fresh = !is_data_expired(g_data_store.system_update_tick) && g_data_store.system.valid;
    }
    
    rt_mutex_release(g_data_store.lock);
    return fresh;
}

rt_tick_t data_manager_get_last_update(const char *type)
{
    if (!type || !g_data_store.initialized) {
        return 0;
    }
    
    rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
    
    rt_tick_t tick = 0;
    if (strcmp(type, "weather") == 0) {
        tick = g_data_store.weather_update_tick;
    } else if (strcmp(type, "stock") == 0) {
        tick = g_data_store.stock_update_tick;
    } else if (strcmp(type, "system") == 0) {
        tick = g_data_store.system_update_tick;
    }
    
    rt_mutex_release(g_data_store.lock);
    return tick;
}

static int data_manager_weather_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type == EVENT_DATA_WEATHER_UPDATED) {
        const weather_data_t *weather = &event->data.weather.weather;
        
        rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
        g_data_store.weather = *weather;
        g_data_store.weather_update_tick = rt_tick_get();
        rt_mutex_release(g_data_store.lock);
        
        return 0;
    }
    
    return -1;
}

static int data_manager_stock_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type == EVENT_DATA_STOCK_UPDATED) {
        const stock_data_t *stock = &event->data.stock.stock;
        
        rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
        g_data_store.stock = *stock;
        g_data_store.stock_update_tick = rt_tick_get();
        rt_mutex_release(g_data_store.lock);
        
        return 0;
    }
    
    return -1;
}

static int data_manager_system_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type == EVENT_DATA_SYSTEM_UPDATED) {
        const system_monitor_data_t *system = &event->data.system.system;
        rt_mutex_take(g_data_store.lock, RT_WAITING_FOREVER);
        g_data_store.system = *system;
        g_data_store.system_update_tick = rt_tick_get();
        rt_mutex_release(g_data_store.lock);
        return 0;
    }
    
    return -1;
}

int data_manager_init(void)
{
    if (g_data_store.initialized) {
        return 0;
    }
    
    g_data_store.lock = rt_mutex_create("data_mgr", RT_IPC_FLAG_PRIO);
    if (!g_data_store.lock) {
        return -RT_ENOMEM;
    }
    
    memset(&g_data_store.weather, 0, sizeof(weather_data_t));
    memset(&g_data_store.stock, 0, sizeof(stock_data_t));
    memset(&g_data_store.system, 0, sizeof(system_monitor_data_t));
    
    g_data_store.weather.valid = false;
    g_data_store.stock.valid = false;
    g_data_store.system.valid = false;
    
    g_data_store.weather_update_tick = 0;
    g_data_store.stock_update_tick = 0;
    g_data_store.system_update_tick = 0;
    g_data_store.last_cleanup_tick = rt_tick_get();
    g_data_store.cleanup_count = 0;
    
    event_bus_subscribe(EVENT_DATA_WEATHER_UPDATED, data_manager_weather_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    event_bus_subscribe(EVENT_DATA_STOCK_UPDATED, data_manager_stock_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    event_bus_subscribe(EVENT_DATA_SYSTEM_UPDATED, data_manager_system_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    
    g_data_store.initialized = true;
    return 0;
}

int data_manager_deinit(void)
{
    if (!g_data_store.initialized) {
        return 0;
    }
    
    event_bus_unsubscribe(EVENT_DATA_WEATHER_UPDATED, data_manager_weather_event_handler);
    event_bus_unsubscribe(EVENT_DATA_STOCK_UPDATED, data_manager_stock_event_handler);
    event_bus_unsubscribe(EVENT_DATA_SYSTEM_UPDATED, data_manager_system_event_handler);
    
    if (g_data_store.lock) {
        rt_mutex_delete(g_data_store.lock);
        g_data_store.lock = NULL;
    }
    
    g_data_store.initialized = false;
    return 0;
}