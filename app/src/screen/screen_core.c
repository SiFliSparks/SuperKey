#include "../screen/screen_core.h"
#include "../screen/screen_ui_manager.h"
#include "../screen/screen_timer_manager.h"
#include "../manager/data_manager.h"
#include "../device/sht30_controller.h"
#include "../mp3/mp3_screen_ui.h"
#include "../mp3/mp3_screen_context.h"
#include <time.h>
#include <string.h>
#include "../screen/screen_context.h"
#define MESSAGE_QUEUE_SIZE 32
#define MESSAGE_TIMEOUT_MS 1

static screen_core_t g_core = {0};

static int process_update_time_message(void);
static int process_update_weather_message(const weather_data_t *data);
static int process_update_system_message(const system_monitor_data_t *data);
static int process_switch_group_message(const screen_switch_msg_t *msg);
static int process_enter_l2_message(const screen_l2_enter_msg_t *msg);
static int process_return_l1_message(void);
static int process_cleanup_message(void);
static int process_update_forecast_message(const weather_forecast_data_t *data);
static int process_update_mp3_message(void);

int screen_core_init(void)
{
    if (g_core.message_queue) {
        return 0;
    }
    
    g_core.message_queue = rt_mq_create("screen_msgs", 
                                       sizeof(screen_message_t),
                                       MESSAGE_QUEUE_SIZE,
                                       RT_IPC_FLAG_PRIO);
    if (!g_core.message_queue) {
        return -RT_ENOMEM;
    }
    
    g_core.state_lock = rt_mutex_create("screen_state", RT_IPC_FLAG_PRIO);
    if (!g_core.state_lock) {
        rt_mq_delete(g_core.message_queue);
        g_core.message_queue = NULL;
        return -RT_ENOMEM;
    }
    
    g_core.current_group = SCREEN_GROUP_1;
    g_core.previous_group = SCREEN_GROUP_1;
    g_core.current_level = SCREEN_LEVEL_1;
    g_core.l2_current_group = SCREEN_L2_TIME_GROUP;
    g_core.l2_current_page = SCREEN_L2_TIME_DETAIL;
    g_core.ui_initialized = false;
    g_core.switching_in_progress = false;
    g_core.messages_processed = 0;
    g_core.switch_count = 0;
    g_core.last_cleanup_time = rt_tick_get();
    
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
    
    return 0;
}

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
    return (result == RT_EOK) ? 0 : -RT_ERROR;
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
    return (result == RT_EOK) ? 0 : -RT_ERROR;
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
    return (result == RT_EOK) ? 0 : -RT_ERROR;
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

int screen_core_post_update_forecast(const weather_forecast_data_t *data)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_UPDATE_FORECAST;
    msg.timestamp = rt_tick_get();
    
    if (data) {
        msg.data.forecast_data = *data;
    }
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int screen_core_post_update_mp3(void)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg = {0};
    msg.type = SCREEN_MSG_UPDATE_MP3;
    msg.timestamp = rt_tick_get();
    
    rt_err_t result = rt_mq_send(g_core.message_queue, &msg, sizeof(msg));
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

int screen_core_process_messages(void)
{
    if (!g_core.message_queue) {
        return -RT_ERROR;
    }
    
    screen_message_t msg;
    int processed_count = 0;
    
    while (processed_count < 10) {
        rt_err_t result = rt_mq_recv(g_core.message_queue, &msg, sizeof(msg), MESSAGE_TIMEOUT_MS);
        
        if (result != RT_EOK) {
            break;
        }
        
        switch (msg.type) {
            case SCREEN_MSG_UPDATE_TIME:
                process_update_time_message();
                break;
            case SCREEN_MSG_UPDATE_WEATHER:
                process_update_weather_message(&msg.data.weather_data);
                break;
            case SCREEN_MSG_UPDATE_SYSTEM:
                process_update_system_message(&msg.data.system_data);
                break;
            case SCREEN_MSG_UPDATE_FORECAST:  
                process_update_forecast_message(&msg.data.forecast_data);
                break;
            case SCREEN_MSG_UPDATE_MP3:
                process_update_mp3_message();
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
                break;
        }
        
        processed_count++;
        g_core.messages_processed++;
    }
    
    return processed_count;
}

static int process_update_time_message(void)
{
    screen_level_t current_level;
    screen_l2_group_t l2_group;
    screen_group_t current_group;
    
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    current_level = g_core.current_level;
    l2_group = g_core.l2_current_group;
    current_group = g_core.current_group;
    rt_mutex_release(g_core.state_lock);
    
    if (current_level == SCREEN_LEVEL_1) {
        switch (current_group) {
            case SCREEN_GROUP_1:
                return screen_ui_update_time_display();
            case SCREEN_GROUP_6:
                return mp3_screen_ui_update();
            default:
                return 0;
        }
    }
    
    if (current_level == SCREEN_LEVEL_2) {
        if (l2_group == SCREEN_L2_TIME_GROUP || 
            l2_group == SCREEN_L2_MUYU_GROUP ||
            l2_group == SCREEN_L2_TOMATO_GROUP ||
            l2_group == SCREEN_L2_WIDGET_SELECTOR_GROUP) {
            return screen_ui_update_time_display();
        }
        
        if (l2_group == SCREEN_L2_STOPWATCH_GROUP) {
            return screen_ui_update_stopwatch_display();
        }
    }
    
    return 0;
}

static int process_update_weather_message(const weather_data_t *data)
{
    if (g_core.current_group != SCREEN_GROUP_1 || g_core.current_level != SCREEN_LEVEL_1) {
        return 0;
    }
    
    weather_data_t weather_data = {0};
    
    if (!data || !data->valid) {
        if (data_manager_get_weather(&weather_data) == 0 && weather_data.valid) {
            data = &weather_data;
        } else {
            return 0;
        }
    }
    
    int ret = screen_ui_update_weather_display(data);
    screen_ui_update_sensor_display();
    
    return ret;
}

static int process_update_system_message(const system_monitor_data_t *data)
{
    if (g_core.current_group != SCREEN_GROUP_2 || g_core.current_level != SCREEN_LEVEL_1) {
        return 0;
    }
    
    system_monitor_data_t system_data = {0};
    
    if (!data || !data->valid) {
        if (data_manager_get_system(&system_data) == 0 && system_data.valid) {
            data = &system_data;
        } else {
            return 0;
        }
    }
    
    return screen_ui_update_system_display(data);
}

static int process_update_forecast_message(const weather_forecast_data_t *data)
{
    if (g_core.current_level != SCREEN_LEVEL_2 || 
        g_core.l2_current_group != SCREEN_L2_WEATHER_GROUP) {
        return 0;
    }
    
    weather_forecast_data_t forecast_data = {0};
    
    if (!data || !data->valid) {
        if (data_manager_get_forecast(&forecast_data) == 0 && forecast_data.valid) {
            data = &forecast_data;
        } else {
            return 0;
        }
    }
    
    return screen_ui_update_l2_weather_forecast(data);
}

static int process_update_mp3_message(void)
{
    if (g_core.current_group != SCREEN_GROUP_6 || g_core.current_level != SCREEN_LEVEL_1) {
        return 0;
    }
    
    return mp3_screen_ui_update();
}

static int get_max_pages_in_l2_group(screen_l2_group_t l2_group)
{
    switch (l2_group) {
        case SCREEN_L2_TIME_GROUP:
        case SCREEN_L2_MEDIA_GROUP:
        case SCREEN_L2_WEB_GROUP:
        case SCREEN_L2_SHORTCUT_GROUP:
            return 1;
        default:
            return 0;
    }
}

static int process_switch_group_message(const screen_switch_msg_t *msg)
{
    if (!msg) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    
    if (g_core.switching_in_progress) {
        rt_mutex_release(g_core.state_lock);
        return -RT_EBUSY;
    }
    
    if (g_core.current_group == msg->target_group && !msg->force_switch) {
        rt_mutex_release(g_core.state_lock);
        return 0;
    }
    
    screen_group_t previous = g_core.current_group;
    g_core.switching_in_progress = true;
    rt_mutex_release(g_core.state_lock);
    
    screen_timer_stop(SCREEN_TIMER_WEATHER);
    screen_timer_stop(SCREEN_TIMER_SENSOR);
    screen_timer_stop(SCREEN_TIMER_SYSTEM);
    screen_timer_stop(SCREEN_TIMER_MP3);
    
    if (previous == SCREEN_GROUP_6) {
        mp3_screen_context_deactivate();
    }
    
    int ret = screen_ui_switch_to_group(msg->target_group);
    
    if (ret == 0) {
        rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
        g_core.previous_group = previous;
        g_core.current_group = msg->target_group;
        g_core.current_level = SCREEN_LEVEL_1;
        g_core.switch_count++;
        rt_mutex_release(g_core.state_lock);
        
        switch (msg->target_group) {
            case SCREEN_GROUP_1:
                screen_timer_start(SCREEN_TIMER_WEATHER);
                screen_timer_start(SCREEN_TIMER_SENSOR);
                screen_core_post_update_weather(NULL);
                {
                    tomato_data_t tomato_data;
                    if (screen_context_get_tomato_data(&tomato_data) == 0 &&
                        tomato_data.current_state == TOMATO_STATE_RUNNING) {
                        screen_ui_start_tomato_background_display();
                    }
                }
                break;
            case SCREEN_GROUP_2:
                screen_timer_start_group2_timers();
                screen_core_post_update_system(NULL);
                break;
            case SCREEN_GROUP_6:
                screen_timer_start(SCREEN_TIMER_MP3);
                mp3_screen_ui_update();
                break;
            default:
                break;
        }
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
    
    screen_timer_stop(SCREEN_TIMER_WEATHER);
    screen_timer_stop(SCREEN_TIMER_SENSOR);
    screen_timer_stop(SCREEN_TIMER_SYSTEM);
    screen_timer_stop(SCREEN_TIMER_MP3);
    
    int ret = screen_ui_switch_to_l2(msg->l2_group, msg->l2_page);
    
    if (ret == 0) {
        rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
        g_core.current_level = SCREEN_LEVEL_2;
        g_core.l2_current_group = msg->l2_group;
        g_core.l2_current_page = msg->l2_page;
        rt_mutex_release(g_core.state_lock);
        
        screen_context_activate_for_level2(msg->l2_group);
        
        if (msg->l2_group == SCREEN_L2_MUYU_GROUP) {
            screen_timer_start_l2_muyu_timers();
        }
        else if (msg->l2_group == SCREEN_L2_STOPWATCH_GROUP) {
            screen_timer_start(SCREEN_TIMER_STOPWATCH);
        }
    }
    
    return ret;
}

static int process_return_l1_message(void)
{
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_group_t l1_group = g_core.current_group;
    screen_l2_group_t previous_l2_group = g_core.l2_current_group;
    rt_mutex_release(g_core.state_lock);
    
    int ret = screen_ui_return_to_l1(l1_group);
    
    if (ret == 0) {
        rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
        g_core.current_level = SCREEN_LEVEL_1;
        rt_mutex_release(g_core.state_lock);
        
        screen_context_deactivate_level2();
        screen_context_activate_for_group(l1_group);
        
        if (previous_l2_group == SCREEN_L2_MUYU_GROUP) {
            screen_timer_stop(SCREEN_TIMER_MUYU);
        }
        else if (previous_l2_group == SCREEN_L2_STOPWATCH_GROUP) {
            screen_timer_stop(SCREEN_TIMER_STOPWATCH);
        }
        
        if (l1_group == SCREEN_GROUP_1) {
            screen_timer_start(SCREEN_TIMER_WEATHER);
            screen_timer_start(SCREEN_TIMER_SENSOR);
            screen_core_post_update_time();
            screen_core_post_update_weather(NULL);
        } else if (l1_group == SCREEN_GROUP_2) {
            screen_timer_start_group2_timers();
            screen_core_post_update_system(NULL);
        } else if (l1_group == SCREEN_GROUP_6) {
            screen_timer_start(SCREEN_TIMER_MP3);
            mp3_screen_ui_update();
        }
    }
    
    return ret;
}

static int process_cleanup_message(void)
{
    data_manager_cleanup_expired_data();
    g_core.last_cleanup_time = rt_tick_get();
    return 0;
}

screen_group_t screen_core_get_current_group(void)
{
    if (!g_core.state_lock) {
        return SCREEN_GROUP_1;
    }
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_group_t group = g_core.current_group;
    rt_mutex_release(g_core.state_lock);
    return group;
}

screen_group_t screen_core_get_previous_group(void)
{
    if (!g_core.state_lock) {
        return SCREEN_GROUP_1;
    }
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_group_t group = g_core.previous_group;
    rt_mutex_release(g_core.state_lock);
    return group;
}

screen_level_t screen_core_get_current_level(void)
{
    if (!g_core.state_lock) {
        return SCREEN_LEVEL_1;
    }
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_level_t level = g_core.current_level;
    rt_mutex_release(g_core.state_lock);
    return level;
}

screen_l2_group_t screen_core_get_current_l2_group(void)
{
    if (!g_core.state_lock) {
        return SCREEN_L2_TIME_GROUP;
    }
    
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    screen_l2_group_t l2_group = g_core.l2_current_group;
    rt_mutex_release(g_core.state_lock);
    return l2_group;
}

bool screen_core_is_switching(void)
{
    if (!g_core.state_lock) {
        return false;
    }
    rt_mutex_take(g_core.state_lock, RT_WAITING_FOREVER);
    bool switching = g_core.switching_in_progress;
    rt_mutex_release(g_core.state_lock);
    return switching;
}