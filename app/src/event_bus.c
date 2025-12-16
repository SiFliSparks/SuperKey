#include "event_bus.h"
#include <string.h>
#include <stdlib.h>

#define EVENT_QUEUE_SIZE        64
#define MAX_SUBSCRIBERS         32
#define EVENT_THREAD_STACK_SIZE 4096
#define EVENT_THREAD_PRIORITY   8     // 高优先级确保及时处理

typedef struct {
    event_subscription_t subscription;
    bool active;
} subscriber_info_t;

static struct {
    rt_mq_t event_queue;
    subscriber_info_t subscribers[MAX_SUBSCRIBERS];
    rt_mutex_t subscribers_lock;
    rt_thread_t event_thread;
    rt_sem_t stop_sem;
    bool running;
    uint32_t published_count;
    uint32_t processed_count;
    uint32_t dropped_count;
    rt_mutex_t stats_lock;
    bool initialized;
    

    uint32_t error_count;
    rt_tick_t last_health_check;
    bool health_monitor_enabled;
} g_event_bus = {0};

static void event_processing_thread(void *parameter);
static int find_subscriber_slot(void);
static int find_subscriber(event_type_t event_type, event_handler_t handler);
static void update_stats(uint32_t *counter);
static bool is_in_interrupt_context(void);
static void event_bus_health_check(void);
static void event_bus_emergency_cleanup(void);


static bool is_in_interrupt_context(void)
{
    return (rt_interrupt_get_nest() > 0);
}


static void event_processing_thread(void *parameter)
{
    (void)parameter;
    event_t event;
    uint32_t consecutive_errors = 0;
    uint32_t processed_events = 0;
    
    while (g_event_bus.running) {
        
        rt_err_t result = rt_mq_recv(g_event_bus.event_queue, &event, sizeof(event_t), 100);
        
        if (result == RT_EOK) {
            consecutive_errors = 0;  // 重置错误计数
            processed_events++;
            

            if (event.type == EVENT_LED_FEEDBACK_REQUEST) {
                int retry_count = 3;
                bool handled = false;
                
                while (retry_count > 0 && !handled) {
                    if (rt_mutex_take(g_event_bus.subscribers_lock, 500) == RT_EOK) {
                        
                        for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
                            subscriber_info_t *sub = &g_event_bus.subscribers[i];
                            
                            if (!sub->active || !sub->subscription.enabled) {
                                continue;
                            }
                            
                            if (sub->subscription.event_type != event.type) {
                                continue;
                            }
                            
                            if (event.priority < sub->subscription.min_priority) {
                                continue;
                            }
                            
                            if (sub->subscription.handler) {
                                int ret = sub->subscription.handler(&event, sub->subscription.user_data);
                                if (ret == 0) {
                                    handled = true;
                                }
                            }
                        }
                        
                        rt_mutex_release(g_event_bus.subscribers_lock);
                        break; // 成功获取到锁，退出重试循环
                        
                    } else {
                        retry_count--;
                        
                        if (retry_count > 0) {
                            rt_thread_mdelay(50); // 短暂等待后重试
                        }
                    }
                }
                
                if (!handled && retry_count == 0) {

                    static uint8_t led_requeue_count = 0;
                    if (led_requeue_count < 2) {
                        rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
                        led_requeue_count++;
                    } else {
                        update_stats(&g_event_bus.dropped_count);
                        led_requeue_count = 0; // 重置计数器
                    }
                } else if (handled) {
                    update_stats(&g_event_bus.processed_count);
                }
                
            } else {

                if (rt_mutex_take(g_event_bus.subscribers_lock, 200) == RT_EOK) {
                    
                    bool handled = false;
                    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
                        subscriber_info_t *sub = &g_event_bus.subscribers[i];
                        
                        if (!sub->active || !sub->subscription.enabled) {
                            continue;
                        }
                        
                        if (sub->subscription.event_type != event.type) {
                            continue;
                        }
                        
                        if (event.priority < sub->subscription.min_priority) {
                            continue;
                        }
                        
                        if (sub->subscription.handler) {
                            int ret = sub->subscription.handler(&event, sub->subscription.user_data);
                            if (ret == 0) {
                                handled = true;
                            }
                        }
                    }
                    
                    rt_mutex_release(g_event_bus.subscribers_lock);
                    
                    if (handled) {
                        update_stats(&g_event_bus.processed_count);
                    }
                    
                } else {
                    update_stats(&g_event_bus.dropped_count);
                }
            }
            
        } else if (result == -RT_ETIMEOUT) {

            

            if (g_event_bus.health_monitor_enabled && (processed_events % 100) == 0) {
                event_bus_health_check();
            }
            
            continue;
            
        } else {
            consecutive_errors++;
            if (consecutive_errors > 10) {
                event_bus_emergency_cleanup();
                rt_thread_mdelay(1000);
                consecutive_errors = 0;
            } else {
                rt_thread_mdelay(10);
            }
        }
    }
}


static void event_bus_health_check(void)
{
    rt_tick_t now = rt_tick_get();

    if ((now - g_event_bus.last_health_check) < rt_tick_from_millisecond(30000)) {
        return;
    }
    
    g_event_bus.last_health_check = now;
    

    if (g_event_bus.event_queue) {
        rt_mq_t mq = g_event_bus.event_queue;
        uint32_t used = mq->max_msgs - mq->entry;
        uint32_t usage_percent = (used * 100) / mq->max_msgs;
        
        if (usage_percent > 80) {
            

            event_t dummy_event;
            int cleaned = 0;
            while (cleaned < 5 && rt_mq_recv(g_event_bus.event_queue, &dummy_event, 
                                            sizeof(event_t), 0) == RT_EOK) {
                cleaned++;
            }
            
            if (cleaned > 0) {
                update_stats(&g_event_bus.dropped_count);
            }
        }
    }
    

    if (g_event_bus.error_count > 0) {
        uint32_t total_events = g_event_bus.published_count + g_event_bus.processed_count;
        if (total_events > 0) {
            uint32_t error_rate = (g_event_bus.error_count * 100) / total_events;
            if (error_rate > 5) {
                g_event_bus.error_count = 0;  // 重置错误计数
            }
        }
    }
}


static void event_bus_emergency_cleanup(void)
{
    

    if (g_event_bus.event_queue) {
        event_t dummy_event;
        int cleaned = 0;
        while (rt_mq_recv(g_event_bus.event_queue, &dummy_event, sizeof(event_t), 0) == RT_EOK) {
            cleaned++;
            if (cleaned > 50) break;  // 避免无限循环
        }
        
        if (cleaned > 0) {
            update_stats(&g_event_bus.dropped_count);
        }
    }
    

    g_event_bus.error_count = 0;
}

static int find_subscriber_slot(void)
{
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        if (!g_event_bus.subscribers[i].active) {
            return i;
        }
    }
    return -1;
}

static int find_subscriber(event_type_t event_type, event_handler_t handler)
{
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        subscriber_info_t *sub = &g_event_bus.subscribers[i];
        if (sub->active && 
            sub->subscription.event_type == event_type &&
            sub->subscription.handler == handler) {
            return i;
        }
    }
    return -1;
}

static void update_stats(uint32_t *counter)
{
    if (rt_mutex_take(g_event_bus.stats_lock, 10) == RT_EOK) {
        (*counter)++;
        rt_mutex_release(g_event_bus.stats_lock);
    }
}


int event_bus_init(void)
{
    if (g_event_bus.initialized) {
        return 0;
    }
    
    memset(&g_event_bus, 0, sizeof(g_event_bus));
    

    g_event_bus.event_queue = rt_mq_create("event_queue", 
                                          sizeof(event_t),
                                          EVENT_QUEUE_SIZE,
                                          RT_IPC_FLAG_PRIO);
    if (!g_event_bus.event_queue) {
        return -RT_ENOMEM;
    }
    

    g_event_bus.subscribers_lock = rt_mutex_create("event_sub_lock", RT_IPC_FLAG_PRIO);
    if (!g_event_bus.subscribers_lock) {
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    

    g_event_bus.stats_lock = rt_mutex_create("event_stats_lock", RT_IPC_FLAG_PRIO);
    if (!g_event_bus.stats_lock) {
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    

    g_event_bus.stop_sem = rt_sem_create("event_stop", 0, RT_IPC_FLAG_PRIO);
    if (!g_event_bus.stop_sem) {
        rt_mutex_delete(g_event_bus.stats_lock);
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    

    g_event_bus.event_thread = rt_thread_create("event_proc",
                                               event_processing_thread,
                                               NULL,
                                               EVENT_THREAD_STACK_SIZE,
                                               EVENT_THREAD_PRIORITY,
                                               10);
    if (!g_event_bus.event_thread) {
        rt_sem_delete(g_event_bus.stop_sem);
        rt_mutex_delete(g_event_bus.stats_lock);
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    

    g_event_bus.running = true;
    g_event_bus.health_monitor_enabled = true;
    g_event_bus.last_health_check = rt_tick_get();
    

    rt_thread_startup(g_event_bus.event_thread);
    
    g_event_bus.initialized = true;
    
    return 0;
}


int event_bus_deinit(void)
{
    if (!g_event_bus.initialized) {
        return 0;
    }
    

    g_event_bus.running = false;
    if (g_event_bus.stop_sem) {
        rt_sem_release(g_event_bus.stop_sem);
    }
    

    rt_thread_mdelay(200);
    

    if (g_event_bus.event_thread) {
        g_event_bus.event_thread = NULL;
    }
    
    if (g_event_bus.stop_sem) {
        rt_sem_delete(g_event_bus.stop_sem);
        g_event_bus.stop_sem = NULL;
    }
    
    if (g_event_bus.stats_lock) {
        rt_mutex_delete(g_event_bus.stats_lock);
        g_event_bus.stats_lock = NULL;
    }
    
    if (g_event_bus.subscribers_lock) {
        rt_mutex_delete(g_event_bus.subscribers_lock);
        g_event_bus.subscribers_lock = NULL;
    }
    
    if (g_event_bus.event_queue) {
        rt_mq_delete(g_event_bus.event_queue);
        g_event_bus.event_queue = NULL;
    }
    g_event_bus.initialized = false;
    
    return 0;
}


int event_bus_publish(event_type_t type, const void *event_data, size_t data_size, 
                     event_priority_t priority, uint32_t source_module_id)
{
    if (!g_event_bus.initialized || !g_event_bus.running) {
        return -RT_ERROR;
    }
    
    if (data_size > sizeof(((event_t*)0)->data)) {
        return -RT_EINVAL;
    }
    
    event_t event = {0};
    event.type = type;
    event.priority = priority;
    event.timestamp = rt_tick_get();
    event.source_module_id = source_module_id;
    
    if (event_data && data_size > 0) {
        memcpy(&event.data, event_data, data_size);
    }
    
    rt_err_t result;
    

    if (is_in_interrupt_context()) {

        result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
        

        if (result == RT_EOK) {
            g_event_bus.published_count++;
        } else {
            g_event_bus.dropped_count++;
        }
    } else {

        result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
        
        if (result == RT_EOK) {
            update_stats(&g_event_bus.published_count);
        } else {
            update_stats(&g_event_bus.dropped_count);
        }
    }
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}


int event_bus_publish_sync(event_type_t type, const void *event_data, size_t data_size,
                          event_priority_t priority, uint32_t source_module_id)
{
    if (!g_event_bus.initialized) {
        return -RT_ERROR;
    }
    

    if (is_in_interrupt_context()) {
        return event_bus_publish(type, event_data, data_size, priority, source_module_id);
    }
    
    event_t event = {0};
    event.type = type;
    event.priority = priority;
    event.timestamp = rt_tick_get();
    event.source_module_id = source_module_id;
    
    if (event_data && data_size > 0) {
        memcpy(&event.data, event_data, data_size);
    }
    

    if (rt_mutex_take(g_event_bus.subscribers_lock, 1000) != RT_EOK) {
        return -RT_ETIMEOUT;
    }
    
    bool handled = false;
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        subscriber_info_t *sub = &g_event_bus.subscribers[i];
        
        if (!sub->active || !sub->subscription.enabled) {
            continue;
        }
        
        if (sub->subscription.event_type != event.type) {
            continue;
        }
        
        if (event.priority < sub->subscription.min_priority) {
            continue;
        }
        
        if (sub->subscription.handler) {
            int ret = sub->subscription.handler(&event, sub->subscription.user_data);
            if (ret == 0) {
                handled = true;
            }
        }
    }
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    
    update_stats(&g_event_bus.published_count);
    if (handled) {
        update_stats(&g_event_bus.processed_count);
    }
    
    return handled ? 0 : -RT_ERROR;
}


int event_bus_subscribe(event_type_t event_type, event_handler_t handler, 
                       void *user_data, event_priority_t min_priority)
{
    if (!g_event_bus.initialized || !handler) {
        return -RT_EINVAL;
    }
    

    if (rt_mutex_take(g_event_bus.subscribers_lock, 1000) != RT_EOK) {
        return -RT_ETIMEOUT;
    }
    
    int existing = find_subscriber(event_type, handler);
    if (existing >= 0) {
        rt_mutex_release(g_event_bus.subscribers_lock);
        return -RT_EBUSY;
    }
    
    int slot = find_subscriber_slot();
    if (slot < 0) {
        rt_mutex_release(g_event_bus.subscribers_lock);
        return -RT_EFULL;
    }
    
    subscriber_info_t *sub = &g_event_bus.subscribers[slot];
    sub->subscription.event_type = event_type;
    sub->subscription.handler = handler;
    sub->subscription.user_data = user_data;
    sub->subscription.min_priority = min_priority;
    sub->subscription.enabled = true;
    sub->active = true;
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    return 0;
}


int event_bus_unsubscribe(event_type_t event_type, event_handler_t handler)
{
    if (!g_event_bus.initialized || !handler) {
        return -RT_EINVAL;
    }
    
    if (rt_mutex_take(g_event_bus.subscribers_lock, 1000) != RT_EOK) {
        return -RT_ETIMEOUT;
    }
    
    int slot = find_subscriber(event_type, handler);
    if (slot >= 0) {
        g_event_bus.subscribers[slot].active = false;
        memset(&g_event_bus.subscribers[slot], 0, sizeof(subscriber_info_t));
    }
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    
    return (slot >= 0) ? 0 : -RT_ERROR;
}


int event_bus_enable_subscription(event_type_t event_type, event_handler_t handler, bool enable)
{
    if (!g_event_bus.initialized || !handler) {
        return -RT_EINVAL;
    }
    
    if (rt_mutex_take(g_event_bus.subscribers_lock, 1000) != RT_EOK) {
        return -RT_ETIMEOUT;
    }
    
    int slot = find_subscriber(event_type, handler);
    if (slot >= 0) {
        g_event_bus.subscribers[slot].subscription.enabled = enable;
    }
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    
    return (slot >= 0) ? 0 : -RT_ERROR;
}


int event_bus_get_stats(uint32_t *published_count, uint32_t *processed_count, 
                       uint32_t *dropped_count, uint32_t *queue_size)
{
    if (!g_event_bus.initialized) {
        return -RT_ERROR;
    }
    
    if (rt_mutex_take(g_event_bus.stats_lock, 100) == RT_EOK) {
        if (published_count) *published_count = g_event_bus.published_count;
        if (processed_count) *processed_count = g_event_bus.processed_count;
        if (dropped_count) *dropped_count = g_event_bus.dropped_count;
        rt_mutex_release(g_event_bus.stats_lock);
    } else {

        if (published_count) *published_count = g_event_bus.published_count;
        if (processed_count) *processed_count = g_event_bus.processed_count;
        if (dropped_count) *dropped_count = g_event_bus.dropped_count;
    }
    
    if (queue_size && g_event_bus.event_queue) {
        rt_mq_t mq = g_event_bus.event_queue;
        *queue_size = (mq->max_msgs - mq->entry);
    }
    
    return 0;
}


int event_bus_cleanup(void)
{
    if (!g_event_bus.initialized || !g_event_bus.event_queue) {
        return -RT_ERROR;
    }
    
    event_t dummy_event;
    int cleaned = 0;
    
    while (rt_mq_recv(g_event_bus.event_queue, &dummy_event, sizeof(event_t), 0) == RT_EOK) {
        cleaned++;
        if (cleaned > 20) break;  // 避免清理过多
    }
    
    if (cleaned > 0) {
        update_stats(&g_event_bus.dropped_count);
    }
    
    return cleaned;
}


int event_bus_publish_data_update(event_type_t data_type, const void *data)
{
    size_t data_size = 0;
    
    switch (data_type) {
    case EVENT_DATA_WEATHER_UPDATED:
        data_size = sizeof(event_data_weather_t);
        break;
    case EVENT_DATA_SYSTEM_UPDATED:
        data_size = sizeof(event_data_system_t);
        break;
    case EVENT_DATA_FORECAST_UPDATED:
        data_size = sizeof(event_data_forecast_t);
        break;
    default:
        data_size = sizeof(event_data_generic_t);
        break;
    }
    
    return event_bus_publish(data_type, data, data_size, 
                           EVENT_PRIORITY_NORMAL, MODULE_ID_DATA_MANAGER);
}


int event_bus_publish_screen_switch(screen_group_t target_group, bool force)
{
    event_data_screen_switch_t switch_data = {
        .target_group = target_group,
        .current_group = SCREEN_GROUP_MAX,
        .force_switch = force
    };
    
    return event_bus_publish(EVENT_SCREEN_SWITCH_REQUEST, &switch_data, 
                           sizeof(switch_data), EVENT_PRIORITY_HIGH, MODULE_ID_SCREEN);
}


int event_bus_publish_error(int error_code, const char *error_msg, const char *module_name)
{
    event_data_error_t error_data = {
        .error_code = error_code,
        .module_name = module_name
    };
    
    if (error_msg) {
        strncpy(error_data.error_msg, error_msg, sizeof(error_data.error_msg) - 1);
        error_data.error_msg[sizeof(error_data.error_msg) - 1] = '\0';
    }
    
    return event_bus_publish(EVENT_SYSTEM_ERROR, &error_data, sizeof(error_data),
                           EVENT_PRIORITY_HIGH, MODULE_ID_SYSTEM);
}


int event_bus_publish_led_feedback(int led_index, uint32_t color, uint32_t duration_ms)
{
    if (!g_event_bus.initialized || !g_event_bus.running) {
        return -RT_ERROR;
    }
    
    event_data_led_t led_data = {
        .led_index = led_index,
        .color = color,
        .duration_ms = duration_ms
    };
    
    event_t event = {0};
    event.type = EVENT_LED_FEEDBACK_REQUEST;
    event.priority = EVENT_PRIORITY_HIGH;  // 提高LED事件优先级
    event.timestamp = rt_tick_get();
    event.source_module_id = MODULE_ID_LED;
    event.data.led = led_data;
    

    rt_err_t result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
    
    if (result == RT_EOK) {

        if (!is_in_interrupt_context()) {
            update_stats(&g_event_bus.published_count);
        } else {
            g_event_bus.published_count++;
        }
    } else {
        if (!is_in_interrupt_context()) {
            update_stats(&g_event_bus.dropped_count);
        } else {
            g_event_bus.dropped_count++;
        }
    }
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}


int event_bus_enable_health_monitor(bool enable)
{
    g_event_bus.health_monitor_enabled = enable;
    return 0;
}


uint32_t event_bus_get_error_count(void)
{
    return g_event_bus.error_count;
}


int event_bus_reset_stats(void)
{
    if (!g_event_bus.initialized) {
        return -RT_ERROR;
    }
    
    if (rt_mutex_take(g_event_bus.stats_lock, 1000) == RT_EOK) {
        g_event_bus.published_count = 0;
        g_event_bus.processed_count = 0;
        g_event_bus.dropped_count = 0;
        g_event_bus.error_count = 0;
        rt_mutex_release(g_event_bus.stats_lock);
        return 0;
    }
    
    return -RT_ETIMEOUT;
}