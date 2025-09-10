#include "screen_core.h"
#include "screen_ui_manager.h"
#include "screen_timer_manager.h"
#include "data_manager.h"
#include "sht30_controller.h"
#include <time.h>
#include <string.h>

#define MESSAGE_QUEUE_SIZE 32
#define MESSAGE_TIMEOUT_MS 100

/* 静态核心管理器实例 */
static screen_core_t g_core = {0};

/* 消息处理函数声明 */
static int process_update_time_message(void);
static int process_update_weather_message(const weather_data_t *data);
static int process_update_stock_message(const stock_data_t *data);
static int process_update_system_message(const system_monitor_data_t *data);
static int process_switch_group_message(const screen_switch_msg_t *msg);
static int process_enter_l2_message(const screen_l2_enter_msg_t *msg);
static int process_return_l1_message(void);
static int process_cleanup_message(void);

int screen_core_init(void)
{
    if (g_core.message_queue) {
        return 0; /* Already initialized */
    }
    
    /* 创建消息队列 */
    g_core.message_queue = rt_mq_create("screen_msgs", 
                                       sizeof(screen_message_t),
                                       MESSAGE_QUEUE_SIZE,
                                       RT_IPC_FLAG_PRIO);
    if (!g_core.message_queue) {
        rt_kprintf("[ScreenCore] Failed to create message queue\n");
        return -RT_ENOMEM;
    }
    
    /* 创建状态锁 */
    g_core.state_lock = rt_mutex_create("screen_state", RT_IPC_FLAG_PRIO);
    if (!g_core.state_lock) {
        rt_mq_delete(g_core.message_queue);
        g_core.message_queue = NULL;
        rt_kprintf("[ScreenCore] Failed to create state lock\n");
        return -RT_ENOMEM;
    }
    
    /* 初始化状态 */
    g_core.current_group = SCREEN_GROUP_1;
    g_core.current_level = SCREEN_LEVEL_1;
    g_core.l2_current_group = SCREEN_L2_TIME_GROUP;
    g_core.l2_current_page = SCREEN_L2_TIME_DETAIL;
    g_core.ui_initialized = false;
    g_core.switching_in_progress = false;
    g_core.messages_processed = 0;
    g_core.switch_count = 0;
    g_core.last_cleanup_time = rt_tick_get();
    
    rt_kprintf("[ScreenCore] Core initialized with %d-message queue\n", MESSAGE_QUEUE_SIZE);
    rt_kprintf("[ScreenCore] Thread-safe message-based UI updates enabled\n");
    
    return 0;
}

int screen_core_deinit(void)
{
    if (g_core.message_queue) {
        rt_mq_delete(g_core.message_queue);
        g_core.message_queue = NULL;
    }
    
    if (g_core.state_lock) {
        rt_mutex_delete(g_core.state_lock);
        g_core.state_lock = NULL;
    }
    
    rt_kprintf("[ScreenCore] Core deinitialized. Stats: %u messages, %u switches\n",
              g_core.messages_processed, g_core.switch_count);
    
    return 0;
}

/* 线程安全的消息发送函数 */
int screen_core_post_switch_group(screen_group_t target_group, bool force)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_SWITCH_GROUP;
    msg.timestamp = rt_tick_get();
    msg.data.switch_msg.target_group = target_group;
    msg.data.switch_msg.force_switch = force;
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    if (result != RT_EOK) {
        rt_kprintf("[ScreenCore] Failed to post switch message: %d\n", result);
        return -RT_ERROR;
    }
    
    return 0;
}

int screen_core_post_enter_l2(screen_l2_group_t l2_group, screen_l2_page_t l2_page)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_ENTER_L2;
    msg.timestamp = rt_tick_get();
    msg.data.l2_enter_msg.l2_group = l2_group;
    msg.data.l2_enter_msg.l2_page = l2_page;
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    if (result != RT_EOK) {
        rt_kprintf("[ScreenCore] Failed to post L2 enter message: %d\n", result);
        return -RT_ERROR;
    }
    
    return 0;
}

int screen_core_post_return_l1(void)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_RETURN_L1;
    msg.timestamp = rt_tick_get();
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    if (result != RT_EOK) {
        rt_kprintf("[ScreenCore] Failed to post L1 return message: %d\n", result);
        return -RT_ERROR;
    }
    
    return 0;
}

int screen_core_post_update_time(void)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_UPDATE_TIME;
    msg.timestamp = rt_tick_get();
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int screen_core_post_update_weather(const weather_data_t *data)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_UPDATE_WEATHER;
    msg.timestamp = rt_tick_get();
    
    if (data) {
        msg.data.weather_data = *data;
    }
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int screen_core_post_update_stock(const stock_data_t *data)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_UPDATE_STOCK;
    msg.timestamp = rt_tick_get();
    
    if (data) {
        msg.data.stock_data = *data;
    }
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int screen_core_post_update_system(const system_monitor_data_t *data)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_UPDATE_SYSTEM;
    msg.timestamp = rt_tick_get();
    
    if (data) {
        msg.data.system_data = *data;
    }
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int screen_core_post_cleanup_request(void)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_CLEANUP_REQUEST;
    msg.timestamp = rt_tick_get();
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

/* GUI线程消息处理 - 这是唯一可以调用LVGL的地方 */
int screen_core_process_messages(void)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg;
    int processed_count = 0;
    
    /* 处理所有待处理消息，但限制处理数量防止阻塞 */
    while (processed_count < 10) {
        rt_err_t result = rt_mq_recv(g_core.message_queue, &msg, sizeof(msg), MESSAGE_TIMEOUT_MS);
        
        if (result != RT_EOK) {
            break; /* 没有更多消息或超时 */
        }
        
        /* 处理消息 */
        switch (msg.type) {
            case SCREEN_MSG_UPDATE_TIME:
                process_update_time_message();
                break;
                
            case SCREEN_MSG_UPDATE_WEATHER:
                process_update_weather_message(&msg.data.weather_data);
                break;
                
            case SCREEN_MSG_UPDATE_STOCK:
                process_update_stock_message(&msg.data.stock_data);
                break;
                
            case SCREEN_MSG_UPDATE_SYSTEM:
                process_update_system_message(&msg.data.system_data);
                break;
                
            case SCREEN_MSG_SWITCH_GROUP:
                process_switch_group_message(&msg.data.switch_msg);
                break;
                
            case SCREEN_MSG_ENTER_L2:
                process_enter_l2_message(&msg.data.l2_enter_msg);
                break;
                
            case SCREEN_MSG_RETURN_L1:
                process_return_l1_message();
                break;
                
            case SCREEN_MSG_CLEANUP_REQUEST:
                process_cleanup_message();
                break;
                
            default:
                rt_kprintf("[ScreenCore] Unknown message type: %d\n", msg.type);
                break;
        }
        
        processed_count++;
        g_core.messages_processed++;
    }
    
    return processed_count;
}

/* 消息处理函数实现 */
static int process_update_time_message(void)
{
    /* 只在Group 1显示时更新时间 */
    if (g_core.current_group == SCREEN_GROUP_1 && g_core.current_level == SCREEN_LEVEL_1) {
        return screen_ui_update_time_display();
    }
    return 0;
}

static int process_update_weather_message(const weather_data_t *data)
{
    /* 只在Group 1显示时更新天气 */
    if (g_core.current_group != SCREEN_GROUP_1 || g_core.current_level != SCREEN_LEVEL_1) {
        return 0;
    }
    
    weather_data_t weather_data = {0};
    
    /* 如果消息中没有数据，从数据管理器获取 */
    if (!data || !data->valid) {
        if (data_manager_get_weather(&weather_data) == 0 && weather_data.valid) {
            data = &weather_data;
        } else {
            return 0; /* 没有有效数据 */
        }
    }
    
    int ret = screen_ui_update_weather_display(data);
    
    /* 同时更新传感器数据 */
    screen_ui_update_sensor_display();
    
    return ret;
}

static int process_update_stock_message(const stock_data_t *data)
{
    /* 只在Group 1显示时更新股票 */
    if (g_core.current_group != SCREEN_GROUP_1 || g_core.current_level != SCREEN_LEVEL_1) {
        return 0;
    }
    
    stock_data_t stock_data = {0};
    
    /* 如果消息中没有数据，从数据管理器获取 */
    if (!data || !data->valid) {
        if (data_manager_get_stock(&stock_data) == 0 && stock_data.valid) {
            data = &stock_data;
        } else {
            return 0; /* 没有有效数据 */
        }
    }
    
    return screen_ui_update_stock_display(data);
}

static int process_update_system_message(const system_monitor_data_t *data)
{
    /* 只在Group 2显示时更新系统监控 */
    if (g_core.current_group != SCREEN_GROUP_2 || g_core.current_level != SCREEN_LEVEL_1) {
        return 0;
    }
    
    system_monitor_data_t system_data = {0};
    
    /* 如果消息中没有数据，从数据管理器获取 */
    if (!data || !data->valid) {
        if (data_manager_get_system(&system_data) == 0 && system_data.valid) {
            data = &system_data;
        } else {
            return 0; /* 没有有效数据 */
        }
    }
    
    return screen_ui_update_system_display(data);
}

static int process_switch_group_message(const screen_switch_msg_t *msg)
{
    if (!msg) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    
    /* 防止重复切换 */
    if (g_core.switching_in_progress) {
        rt_mutex_release(g_core.state_lock);
        return -RT_EBUSY;
    }
    
    if (g_core.current_group == msg->target_group && !msg->force_switch) {
        rt_mutex_release(g_core.state_lock);
        return 0; /* 已经是目标组 */
    }
    
    g_core.switching_in_progress = true;
    rt_mutex_release(g_core.state_lock);
    
    rt_kprintf("[ScreenCore] Processing switch to group %d\n", msg->target_group);
    
    /* 停止当前组的定时器 */
    screen_timer_stop_all_group_timers();
    
    /* 执行UI切换 - 这里才是真正的LVGL操作 */
    int ret = screen_ui_switch_to_group(msg->target_group);
    
    if (ret == 0) {
        rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
        g_core.current_group = msg->target_group;
        g_core.current_level = SCREEN_LEVEL_1;
        g_core.switch_count++;
        rt_mutex_release(g_core.state_lock);
        
        /* 启动新组的定时器 */
        if (msg->target_group == SCREEN_GROUP_1) {
            screen_timer_start_group1_timers();
        } else if (msg->target_group == SCREEN_GROUP_2) {
            screen_timer_start_group2_timers();
        }
        
        rt_kprintf("[ScreenCore] Successfully switched to group %d\n", msg->target_group);
    } else {
        rt_kprintf("[ScreenCore] Failed to switch to group %d: %d\n", msg->target_group, ret);
    }
    
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    g_core.switching_in_progress = false;
    rt_mutex_release(g_core.state_lock);
    
    return ret;
}

static int process_enter_l2_message(const screen_l2_enter_msg_t *msg)
{
    if (!msg) {
        return -RT_EINVAL;
    }
    
    rt_kprintf("[ScreenCore] Processing enter L2: group %d, page %d\n", 
              msg->l2_group, msg->l2_page);
    
    /* 停止所有组定时器 */
    screen_timer_stop_all_group_timers();
    
    /* 执行L2切换 */
    int ret = screen_ui_switch_to_l2(msg->l2_group, msg->l2_page);
    
    if (ret == 0) {
        rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
        g_core.current_level = SCREEN_LEVEL_2;
        g_core.l2_current_group = msg->l2_group;
        g_core.l2_current_page = msg->l2_page;
        rt_mutex_release(g_core.state_lock);
        
        rt_kprintf("[ScreenCore] Successfully entered L2\n");
    }
    
    return ret;
}

static int process_return_l1_message(void)
{
    rt_kprintf("[ScreenCore] Processing return to L1\n");
    
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_group_t l1_group = g_core.current_group;
    rt_mutex_release(g_core.state_lock);
    
    /* 执行返回L1 */
    int ret = screen_ui_return_to_l1(l1_group);
    
    if (ret == 0) {
        rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
        g_core.current_level = SCREEN_LEVEL_1;
        rt_mutex_release(g_core.state_lock);
        
        /* 重启对应组的定时器 */
        if (l1_group == SCREEN_GROUP_1) {
            screen_timer_start_group1_timers();
        } else if (l1_group == SCREEN_GROUP_2) {
            screen_timer_start_group2_timers();
        }
        
        rt_kprintf("[ScreenCore] Successfully returned to L1\n");
    }
    
    return ret;
}

static int process_cleanup_message(void)
{
    rt_kprintf("[ScreenCore] Processing cleanup request\n");
    
    /* 清理过期数据 */
    data_manager_cleanup_expired_data();
    
    /* 更新清理时间 */
    g_core.last_cleanup_time = rt_tick_get();
    
    return 0;
}

/* 线程安全的状态查询函数 */
screen_group_t screen_core_get_current_group(void)
{
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_group_t group = g_core.current_group;
    rt_mutex_release(g_core.state_lock);
    return group;
}

screen_level_t screen_core_get_current_level(void)
{
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_level_t level = g_core.current_level;
    rt_mutex_release(g_core.state_lock);
    return level;
}

bool screen_core_is_switching(void)
{
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    bool switching = g_core.switching_in_progress;
    rt_mutex_release(g_core.state_lock);
    return switching;
}