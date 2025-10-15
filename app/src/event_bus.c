// event_bus.c - 完整重新设计版本

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
    
    // 新增：错误恢复相关
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

/* 检查是否在中断上下文中 */
static bool is_in_interrupt_context(void)
{
    return (rt_interrupt_get_nest() > 0);
}

/* 事件处理线程 - 增强版本，LED事件优先处理 */
static void event_processing_thread(void *parameter)
{
    (void)parameter;
    event_t event;
    uint32_t consecutive_errors = 0;
    uint32_t processed_events = 0;
    
    rt_kprintf("[EventBus] Processing thread started (priority %d, LED-optimized)\n", EVENT_THREAD_PRIORITY);
    
    while (g_event_bus.running) {
        // 检查停止信号
        if (rt_sem_take(g_event_bus.stop_sem, RT_WAITING_NO) == RT_EOK) {
            rt_kprintf("[EventBus] Stop signal received\n");
            break;
        }
        
        rt_err_t result = rt_mq_recv(g_event_bus.event_queue, &event, sizeof(event_t), 100);
        
        if (result == RT_EOK) {
            consecutive_errors = 0;  // 重置错误计数
            processed_events++;
            
            // 为LED反馈事件创建特殊处理路径
            if (event.type == EVENT_LED_FEEDBACK_REQUEST) {
                rt_kprintf("[EventBus] Processing LED feedback event (high priority)\n");
                
                // LED事件使用更长的超时时间和重试机制
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
                                rt_kprintf("[EventBus] Calling LED handler\n");
                                int ret = sub->subscription.handler(&event, sub->subscription.user_data);
                                if (ret == 0) {
                                    handled = true;
                                    rt_kprintf("[EventBus] LED event handled successfully\n");
                                }
                            }
                        }
                        
                        rt_mutex_release(g_event_bus.subscribers_lock);
                        break; // 成功获取到锁，退出重试循环
                        
                    } else {
                        retry_count--;
                        rt_kprintf("[EventBus] LED event lock failed, retrying (%d attempts left)\n", retry_count);
                        
                        if (retry_count > 0) {
                            rt_thread_mdelay(50); // 短暂等待后重试
                        }
                    }
                }
                
                if (!handled && retry_count == 0) {
                    // 所有重试都失败，将事件重新入队一次
                    static uint8_t led_requeue_count = 0;
                    if (led_requeue_count < 2) {
                        rt_kprintf("[EventBus] LED event failed all retries, re-queuing (attempt %d)\n", led_requeue_count + 1);
                        rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
                        led_requeue_count++;
                    } else {
                        rt_kprintf("[EventBus] LED event dropped after max retries\n");
                        update_stats(&g_event_bus.dropped_count);
                        led_requeue_count = 0; // 重置计数器
                    }
                } else if (handled) {
                    update_stats(&g_event_bus.processed_count);
                }
                
            } else {
                // 其他事件的正常处理逻辑 - 使用较短的超时时间
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
                    rt_kprintf("[EventBus] Failed to acquire subscribers lock for event 0x%04X, dropping\n", event.type);
                    update_stats(&g_event_bus.dropped_count);
                }
            }
            
        } else if (result == -RT_ETIMEOUT) {
            // 超时是正常的，用于检查停止信号
            
            // 每100次超时执行一次健康检查
            if (g_event_bus.health_monitor_enabled && (processed_events % 100) == 0) {
                event_bus_health_check();
            }
            
            continue;
            
        } else {
            consecutive_errors++;
            rt_kprintf("[EventBus] Message queue error: %d (consecutive: %u)\n", result, consecutive_errors);
            
            // 如果连续错误过多，进入恢复模式
            if (consecutive_errors > 10) {
                rt_kprintf("[EventBus] Too many consecutive errors, entering recovery mode\n");
                event_bus_emergency_cleanup();
                rt_thread_mdelay(1000);  // 等待1秒再继续
                consecutive_errors = 0;
            } else {
                rt_thread_mdelay(10);
            }
        }
    }
    
    rt_kprintf("[EventBus] Processing thread stopped (processed %u events)\n", processed_events);
}

/* 健康检查函数 */
static void event_bus_health_check(void)
{
    rt_tick_t now = rt_tick_get();
    
    // 每30秒执行一次完整健康检查
    if ((now - g_event_bus.last_health_check) < rt_tick_from_millisecond(30000)) {
        return;
    }
    
    g_event_bus.last_health_check = now;
    
    // 检查队列使用率
    if (g_event_bus.event_queue) {
        rt_mq_t mq = g_event_bus.event_queue;
        uint32_t used = mq->max_msgs - mq->entry;
        uint32_t usage_percent = (used * 100) / mq->max_msgs;
        
        if (usage_percent > 80) {
            rt_kprintf("[EventBus] Queue usage high: %u%%, cleaning old events\n", usage_percent);
            
            // 清理一些旧事件
            event_t dummy_event;
            int cleaned = 0;
            while (cleaned < 5 && rt_mq_recv(g_event_bus.event_queue, &dummy_event, 
                                            sizeof(event_t), 0) == RT_EOK) {
                cleaned++;
            }
            
            if (cleaned > 0) {
                rt_kprintf("[EventBus] Cleaned %d old events\n", cleaned);
                update_stats(&g_event_bus.dropped_count);
            }
        }
    }
    
    // 检查错误率
    if (g_event_bus.error_count > 0) {
        uint32_t total_events = g_event_bus.published_count + g_event_bus.processed_count;
        if (total_events > 0) {
            uint32_t error_rate = (g_event_bus.error_count * 100) / total_events;
            if (error_rate > 5) {
                rt_kprintf("[EventBus] High error rate: %u%%, resetting error count\n", error_rate);
                g_event_bus.error_count = 0;  // 重置错误计数
            }
        }
    }
}

/* 紧急清理函数 */
static void event_bus_emergency_cleanup(void)
{
    rt_kprintf("[EventBus] Performing emergency cleanup...\n");
    
    // 清空消息队列
    if (g_event_bus.event_queue) {
        event_t dummy_event;
        int cleaned = 0;
        while (rt_mq_recv(g_event_bus.event_queue, &dummy_event, sizeof(event_t), 0) == RT_EOK) {
            cleaned++;
            if (cleaned > 50) break;  // 避免无限循环
        }
        
        if (cleaned > 0) {
            rt_kprintf("[EventBus] Emergency: cleaned %d events from queue\n", cleaned);
            update_stats(&g_event_bus.dropped_count);
        }
    }
    
    // 重置错误计数
    g_event_bus.error_count = 0;
    
    rt_kprintf("[EventBus] Emergency cleanup completed\n");
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

/* 事件总线初始化 */
int event_bus_init(void)
{
    if (g_event_bus.initialized) {
        rt_kprintf("[EventBus] Already initialized\n");
        return 0;
    }
    
    rt_kprintf("[EventBus] Initializing event bus system (enhanced)...\n");
    
    memset(&g_event_bus, 0, sizeof(g_event_bus));
    
    // 创建事件队列
    g_event_bus.event_queue = rt_mq_create("event_queue", 
                                          sizeof(event_t),
                                          EVENT_QUEUE_SIZE,
                                          RT_IPC_FLAG_PRIO);
    if (!g_event_bus.event_queue) {
        rt_kprintf("[EventBus] Failed to create event queue\n");
        return -RT_ENOMEM;
    }
    
    // 创建订阅者锁
    g_event_bus.subscribers_lock = rt_mutex_create("event_sub_lock", RT_IPC_FLAG_PRIO);
    if (!g_event_bus.subscribers_lock) {
        rt_kprintf("[EventBus] Failed to create subscribers lock\n");
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    
    // 创建统计锁
    g_event_bus.stats_lock = rt_mutex_create("event_stats_lock", RT_IPC_FLAG_PRIO);
    if (!g_event_bus.stats_lock) {
        rt_kprintf("[EventBus] Failed to create stats lock\n");
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    
    // 创建停止信号量
    g_event_bus.stop_sem = rt_sem_create("event_stop", 0, RT_IPC_FLAG_PRIO);
    if (!g_event_bus.stop_sem) {
        rt_kprintf("[EventBus] Failed to create stop semaphore\n");
        rt_mutex_delete(g_event_bus.stats_lock);
        rt_mutex_delete(g_event_bus.subscribers_lock);
        rt_mq_delete(g_event_bus.event_queue);
        return -RT_ENOMEM;
    }
    
    // 创建事件处理线程
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
    
    // 初始化状态
    g_event_bus.running = true;
    g_event_bus.health_monitor_enabled = true;
    g_event_bus.last_health_check = rt_tick_get();
    
    // 启动事件处理线程
    rt_thread_startup(g_event_bus.event_thread);
    
    g_event_bus.initialized = true;
    
    rt_kprintf("[EventBus] Event bus initialized successfully (enhanced)\n");
    rt_kprintf("[EventBus] Queue size: %d, Max subscribers: %d, Thread priority: %d\n", 
              EVENT_QUEUE_SIZE, MAX_SUBSCRIBERS, EVENT_THREAD_PRIORITY);
    
    return 0;
}

/* 事件总线去初始化 */
int event_bus_deinit(void)
{
    if (!g_event_bus.initialized) {
        return 0;
    }
    
    rt_kprintf("[EventBus] Deinitializing event bus...\n");
    
    // 停止线程
    g_event_bus.running = false;
    if (g_event_bus.stop_sem) {
        rt_sem_release(g_event_bus.stop_sem);
    }
    
    // 等待线程结束
    rt_thread_mdelay(200);
    
    // 清理资源
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
    
    rt_kprintf("[EventBus] Final stats - Published: %u, Processed: %u, Dropped: %u, Errors: %u\n",
              g_event_bus.published_count, g_event_bus.processed_count, 
              g_event_bus.dropped_count, g_event_bus.error_count);
    
    g_event_bus.initialized = false;
    rt_kprintf("[EventBus] Event bus deinitialized\n");
    
    return 0;
}

/* 发布事件 - 增强版本，区分中断和线程上下文 */
int event_bus_publish(event_type_t type, const void *event_data, size_t data_size, 
                     event_priority_t priority, uint32_t source_module_id)
{
    if (!g_event_bus.initialized || !g_event_bus.running) {
        return -RT_ERROR;
    }
    
    if (data_size > sizeof(((event_t*)0)->data)) {
        if (!is_in_interrupt_context()) {
            rt_kprintf("[EventBus] Event data too large: %zu bytes\n", data_size);
        }
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
    
    // 区分中断和线程上下文
    if (is_in_interrupt_context()) {
        // 在中断上下文中，只使用非阻塞发送
        result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
        
        // 在中断中不能安全更新统计，只能粗略计数
        if (result == RT_EOK) {
            g_event_bus.published_count++;
        } else {
            g_event_bus.dropped_count++;
        }
    } else {
        // 在线程上下文中，可以使用阻塞发送
        result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
        
        if (result == RT_EOK) {
            update_stats(&g_event_bus.published_count);
        } else {
            update_stats(&g_event_bus.dropped_count);
            rt_kprintf("[EventBus] Failed to publish event 0x%04X: %d\n", type, result);
        }
    }
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

/* 同步发布事件 */
int event_bus_publish_sync(event_type_t type, const void *event_data, size_t data_size,
                          event_priority_t priority, uint32_t source_module_id)
{
    if (!g_event_bus.initialized) {
        return -RT_ERROR;
    }
    
    // 在中断上下文中不支持同步发布
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
    
    // 使用短超时获取锁
    if (rt_mutex_take(g_event_bus.subscribers_lock, 1000) != RT_EOK) {
        rt_kprintf("[EventBus] Failed to acquire lock for sync publish\n");
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

/* 订阅事件 */
int event_bus_subscribe(event_type_t event_type, event_handler_t handler, 
                       void *user_data, event_priority_t min_priority)
{
    if (!g_event_bus.initialized || !handler) {
        return -RT_EINVAL;
    }
    
    // 使用短超时避免死锁
    if (rt_mutex_take(g_event_bus.subscribers_lock, 1000) != RT_EOK) {
        rt_kprintf("[EventBus] Failed to acquire lock for subscription\n");
        return -RT_ETIMEOUT;
    }
    
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

/* 取消订阅事件 */
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
        rt_kprintf("[EventBus] Unsubscribed from event 0x%04X (slot %d)\n", event_type, slot);
    }
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    
    return (slot >= 0) ? 0 : -RT_ERROR;
}

/* 启用/禁用订阅 */
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
        rt_kprintf("[EventBus] %s subscription for event 0x%04X\n", 
                  enable ? "Enabled" : "Disabled", event_type);
    }
    
    rt_mutex_release(g_event_bus.subscribers_lock);
    
    return (slot >= 0) ? 0 : -RT_ERROR;
}

/* 获取统计信息 */
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
        // 如果无法获取锁，返回缓存值
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

/* 清理队列 */
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
        rt_kprintf("[EventBus] Cleaned %d pending events\n", cleaned);
        update_stats(&g_event_bus.dropped_count);
    }
    
    return cleaned;
}

/* 便捷函数：发布数据更新事件 */
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

/* 便捷函数：发布屏幕切换事件 */
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

/* 便捷函数：发布错误事件 */
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

/* 便利函数：发布LED反馈事件 - 优化版本 */
int event_bus_publish_led_feedback(int led_index, uint32_t color, uint32_t duration_ms)
{
    if (!g_event_bus.initialized || !g_event_bus.running) {
        rt_kprintf("[EventBus] LED feedback failed: bus not initialized\n");
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
    
    rt_kprintf("[EventBus] Publishing LED feedback: led=%d, color=0x%06X, duration=%ums\n", 
               led_index, color, duration_ms);
    
    // 使用非阻塞发送，但检查队列状态
    rt_err_t result = rt_mq_send(g_event_bus.event_queue, &event, sizeof(event_t));
    
    if (result == RT_EOK) {
        rt_kprintf("[EventBus] LED event queued successfully\n");
        // 在中断中不能安全更新统计，粗略计数
        if (!is_in_interrupt_context()) {
            update_stats(&g_event_bus.published_count);
        } else {
            g_event_bus.published_count++;
        }
    } else {
        rt_kprintf("[EventBus] LED event queue failed: %d\n", result);
        if (!is_in_interrupt_context()) {
            update_stats(&g_event_bus.dropped_count);
        } else {
            g_event_bus.dropped_count++;
        }
    }
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

/* 新增：启用/禁用健康监控 */
int event_bus_enable_health_monitor(bool enable)
{
    g_event_bus.health_monitor_enabled = enable;
    rt_kprintf("[EventBus] Health monitor %s\n", enable ? "enabled" : "disabled");
    return 0;
}

/* 新增：获取错误统计 */
uint32_t event_bus_get_error_count(void)
{
    return g_event_bus.error_count;
}

/* 新增：重置统计 */
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
        
        rt_kprintf("[EventBus] Statistics reset\n");
        return 0;
    }
    
    return -RT_ETIMEOUT;
}