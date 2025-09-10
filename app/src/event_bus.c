#include "event_bus.h"
#include <string.h>
#include <stdlib.h>

#define EVENT_QUEUE_SIZE        64
#define MAX_SUBSCRIBERS         32
#define EVENT_THREAD_STACK_SIZE 4096
#define EVENT_THREAD_PRIORITY   15

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
} g_event_bus = {0};

static void event_processing_thread(void *parameter);
static int find_subscriber_slot(void);
static int find_subscriber(event_type_t event_type, event_handler_t handler);
static void update_stats(uint32_t *counter);

static void event_processing_thread(void *parameter)
{
    (void)parameter;
    event_t event;
    
    rt_kprintf("[EventBus] Processing thread started\n");
    
    while (g_event_bus.running) {
        if (rt_sem_take(g_event_bus.stop_sem, RT_WAITING_NO) == RT_EOK) {
            rt_kprintf("[EventBus] Stop signal received\n");
            break;
        }
        
        rt_err_t result = rt_mq_recv(g_event_bus.event_queue, &event, sizeof(event_t), 100);
        
        if (result == RT_EOK) {
            rt_mutex_take(g_event_bus.subscribers_lock, RT_WAITING_FOREVER);
            
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
            
        } else if (result == -RT_ETIMEOUT) {
            continue;
        } else {
            rt_kprintf("[EventBus] Message queue error: %d\n", result);
            rt_thread_mdelay(10);
        }
    }
    
    rt_kprintf("[EventBus] Processing thread stopped\n");
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
    rt_mutex_take(g_event_bus.stats_lock, RT_WAITING_FOREVER);
    (*counter)++;
    rt_mutex_release(g_event_bus.stats_lock);
}

int event_bus_init(void)
{
    if (g_event_bus.initialized) {
        rt_kprintf("[EventBus] Already initialized\n");
        return 0;
    }
    
    rt_kprintf("[EventBus] Initializing event bus system...\n");
    
    memset(&g_event_bus, 0, sizeof(g_event_bus));
    
    g_event_bus.event_queue = rt_mq_create("event_queue", 
                                          sizeof(event_t),
                                          EVENT_QUEUE_SIZE,
                                          RT_IPC_FLAG_PRIO);
    if (!g_event_bus.event_queue) {
        rt_kprintf("[EventBus] Failed to create event queue\n");
        return -RT_ENOMEM;
    }
    
    g_event_bus.subscribers_lock = rt_mutex_create("event_sub_lock", RT_IPC_FLAG_PRIO);
    if (!g_event_bus.subscribers_lock) {
        rt_kprintf("[EventBus] Failed to create subscribers lock\n");
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    
    g_event_bus.stats_lock = rt_mutex_create("event_stats_lock", RT_IPC_FLAG_PRIO);
    if (!g_event_bus.stats_lock) {
        rt_kprintf("[EventBus] Failed to create stats lock\n");
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    
    g_event_bus.stop_sem = rt_sem_create("event_stop", 0, RT_IPC_FLAG_PRIO);
    if (!g_event_bus.stop_sem) {
        rt_kprintf("[EventBus] Failed to create stop semaphore\n");
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
        rt_kprintf("[EventBus] Failed to create event processing thread\n");
        rt_sem_delete(g_event_bus.stop_sem);
        rt_mutex_delete(g_event_bus.stats_lock);
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    
    g_event_bus.running = true;
    rt_thread_startup(g_event_bus.event_thread);
    
    g_event_bus.initialized = true;
    
    rt_kprintf("[EventBus] Event bus initialized successfully\n");
    rt_kprintf("[EventBus] Queue size: %d, Max subscribers: %d\n", 
              EVENT_QUEUE_SIZE, MAX_SUBSCRIBERS);
    
    return 0;
}

int event_bus_deinit(void)
{
    if (!g_event_bus.initialized) {
        return 0;
    }
    
    rt_kprintf("[EventBus] Deinitializing event bus...\n");
    
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
    
    rt_kprintf("[EventBus] Final stats - Published: %u, Processed: %u, Dropped: %u\n",
              g_event_bus.published_count, g_event_bus.processed_count, g_event_bus.dropped_count);
    
    g_event_bus.initialized = false;
    rt_kprintf("[EventBus] Event bus deinitialized\n");
    
    return 0;
}

int event_bus_publish(event_type_t type, const void *event_data, size_t data_size, 
                     event_priority_t priority, uint32_t source_module_id)
{
    if (!g_event_bus.initialized || !g_event_bus.running) {
        return -RT_ERROR;
    }
    
    if (data_size > sizeof(((event_t*)0)->data)) {
        rt_kprintf("[EventBus] Event data too large: %d bytes\n", data_size);
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
    
    rt_err_t result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
    
    if (result == RT_EOK) {
        update_stats(&g_event_bus.published_count);
        return 0;
    } else {
        update_stats(&g_event_bus.dropped_count);
        rt_kprintf("[EventBus] Failed to publish event 0x%04X: %d\n", type, result);
        return -RT_ERROR;
    }
}

int event_bus_publish_sync(event_type_t type, const void *event_data, size_t data_size,
                          event_priority_t priority, uint32_t source_module_id)
{
    if (!g_event_bus.initialized) {
        return -RT_ERROR;
    }
    
    event_t event = {0};
    event.type = type;
    event.priority = priority;
    event.timestamp = rt_tick_get();
    event.source_module_id = source_module_id;
    
    if (event_data && data_size > 0) {
        memcpy(&event.data, event_data, data_size);
    }
    
    rt_mutex_take(g_event_bus.subscribers_lock, RT_WAITING_FOREVER);
    
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
    
    rt_mutex_take(g_event_bus.subscribers_lock, RT_WAITING_FOREVER);
    
    int existing = find_subscriber(event_type, handler);
    if (existing >= 0) {
        rt_mutex_release(g_event_bus.subscribers_lock);
        rt_kprintf("[EventBus] Already subscribed to event 0x%04X\n", event_type);
        return -RT_EBUSY;
    }
    
    int slot = find_subscriber_slot();
    if (slot < 0) {
        rt_mutex_release(g_event_bus.subscribers_lock);
        rt_kprintf("[EventBus] No free subscriber slots\n");
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
    
    rt_kprintf("[EventBus] Subscribed to event 0x%04X (slot %d)\n", event_type, slot);
    return 0;
}

int event_bus_unsubscribe(event_type_t event_type, event_handler_t handler)
{
    if (!g_event_bus.initialized || !handler) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_event_bus.subscribers_lock, RT_WAITING_FOREVER);
    
    int slot = find_subscriber(event_type, handler);
    if (slot >= 0) {
        g_event_bus.subscribers[slot].active = false;
        memset(&g_event_bus.subscribers[slot], 0, sizeof(subscriber_info_t));
        rt_kprintf("[EventBus] Unsubscribed from event 0x%04X (slot %d)\n", event_type, slot);
    }
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    
    return (slot >= 0) ? 0 : -RT_ERROR;
}

int event_bus_enable_subscription(event_type_t event_type, event_handler_t handler, bool enable)
{
    if (!g_event_bus.initialized || !handler) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_event_bus.subscribers_lock, RT_WAITING_FOREVER);
    
    int slot = find_subscriber(event_type, handler);
    if (slot >= 0) {
        g_event_bus.subscribers[slot].subscription.enabled = enable;
        rt_kprintf("[EventBus] %s subscription for event 0x%04X\n", 
                  enable ? "Enabled" : "Disabled", event_type);
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
    
    rt_mutex_take(g_event_bus.stats_lock, RT_WAITING_FOREVER);
    
    if (published_count) *published_count = g_event_bus.published_count;
    if (processed_count) *processed_count = g_event_bus.processed_count;
    if (dropped_count) *dropped_count = g_event_bus.dropped_count;
    
    rt_mutex_release(g_event_bus.stats_lock);
    
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
    }
    
    if (cleaned > 0) {
        rt_kprintf("[EventBus] Cleaned %d pending events\n", cleaned);
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
    case EVENT_DATA_STOCK_UPDATED:
        data_size = sizeof(event_data_stock_t);
        break;
    case EVENT_DATA_SYSTEM_UPDATED:
        data_size = sizeof(event_data_system_t);
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
    event_data_led_t led_data = {
        .led_index = led_index,
        .color = color,
        .duration_ms = duration_ms
    };
    
    return event_bus_publish(EVENT_LED_FEEDBACK_REQUEST, &led_data, sizeof(led_data),
                           EVENT_PRIORITY_NORMAL, MODULE_ID_LED);
}