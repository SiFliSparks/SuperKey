// led_effects_manager.c - 完整重新设计版本（修复编译错误）

#include "led_effects_manager.h"
#include "drv_rgbled.h"
#include "event_bus.h"
#include "bf0_hal.h"
#include <string.h>
#include <math.h>

#define MAX_CONCURRENT_EFFECTS  4
#define MAX_CUSTOM_EFFECTS      8
#define LED_UPDATE_INTERVAL_MS  20
#define LED_THREAD_STACK_SIZE   2048
#define LED_THREAD_PRIORITY     12

/* LED更新消息类型 */
typedef enum {
    LED_MSG_UPDATE_TICK,        // 定时更新
    LED_MSG_SET_LED,           // 设置单个LED
    LED_MSG_SET_ALL_LEDS,      // 设置所有LED
    LED_MSG_START_EFFECT,      // 启动效果
    LED_MSG_STOP_EFFECT,       // 停止效果
    LED_MSG_SET_BRIGHTNESS,    // 设置亮度
    LED_MSG_LED_FEEDBACK,      // LED反馈闪烁
    LED_MSG_SHUTDOWN           // 关闭管理器
} led_msg_type_t;

/* LED消息结构体 */
typedef struct {
    led_msg_type_t type;
    union {
        struct {
            uint8_t led_index;
            uint32_t color;
        } set_led;
        struct {
            uint32_t color;
        } set_all;
        struct {
            led_effect_config_t config;
            int *effect_id_out;  // 用于返回效果ID
            rt_sem_t done_sem;   // 用于同步（直接使用信号量）
        } start_effect;
        struct {
            int effect_id;
        } stop_effect;
        struct {
            uint8_t brightness;
        } set_brightness;
        struct {
            int led_index;
            uint32_t color;
            uint32_t duration_ms;
        } led_feedback;
    } data;
} led_message_t;

/* 内部效果句柄结构体 */
typedef struct led_effect_handle {
    led_effect_config_t config;
    led_effect_state_t state;
    uint32_t start_tick;
    uint32_t last_update_tick;
    uint32_t effect_tick;
    bool active;
    int id;
} led_effect_handle_internal_t;

/* LED效果管理器全局状态 */
static struct {
    rt_device_t rgb_device;
    led_effect_handle_internal_t effects[MAX_CONCURRENT_EFFECTS];
    uint32_t *led_buffer;
    uint32_t *manual_led_buffer;
    bool *manual_led_mask;
    uint32_t actual_led_count;
    uint8_t global_brightness;
    
    // 线程和通信
    rt_thread_t led_thread;
    rt_mq_t led_msg_queue;
    rt_timer_t update_timer;
    rt_sem_t shutdown_sem;
    
    int next_effect_id;
    bool initialized;
    bool running;
} g_led_mgr = {0};

/* 完整的前向声明 - 确保所有函数都在使用前声明 */
static void led_update_timer_callback(void *parameter);
static void led_effects_thread_entry(void *parameter);
static int led_send_message(const led_message_t *msg, bool sync);
static void led_process_message(const led_message_t *msg);
static void led_do_update_effects(void);
static void led_do_update_hardware(void);
static int led_effects_hardware_init(void);
static void led_effects_configure_pins(void);
static int apply_effect_static(const led_effect_handle_internal_t *effect, uint32_t *buffer);
static int apply_effect_breathing(const led_effect_handle_internal_t *effect, uint32_t *buffer);
static int apply_effect_flowing(const led_effect_handle_internal_t *effect, uint32_t *buffer);
static int apply_effect_blink(const led_effect_handle_internal_t *effect, uint32_t *buffer);
static int led_feedback_event_handler(const event_t *event, void *user_data);

/* 硬件初始化函数实现 */
static int led_effects_hardware_init(void)
{
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO3_3V3, true, true);
    
    // 配置PWM引脚
    led_effects_configure_pins();
    
    // 等待硬件稳定
    rt_thread_mdelay(100);
    return 0;
}

/* LED反馈事件处理函数 */
static int led_feedback_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type != EVENT_LED_FEEDBACK_REQUEST) {
        return -1;
    }
    
    const event_data_led_t *led_data = &event->data.led;
    
    led_message_t msg = {
        .type = LED_MSG_LED_FEEDBACK,
        .data.led_feedback = {
            .led_index = led_data->led_index,
            .color = led_data->color,
            .duration_ms = led_data->duration_ms
        }
    };
    
    return led_send_message(&msg, false);
}


/* 配置PWM引脚 */
static void led_effects_configure_pins(void)
{
    HAL_PIN_Set(PAD_PA10, GPTIM2_CH1, PIN_NOPULL, 1);
}

/* 定时器回调 - 只在中断上下文中发送消息 */
static void led_update_timer_callback(void *parameter)
{
    (void)parameter;
    
    // 在中断上下文中只发送非阻塞消息
    led_message_t msg = {.type = LED_MSG_UPDATE_TICK};
    rt_mq_send(g_led_mgr.led_msg_queue, &msg, sizeof(msg));
}

/* LED效果处理线程 */
static void led_effects_thread_entry(void *parameter)
{
    (void)parameter;
    led_message_t msg;
    rt_err_t result;
    
    while (g_led_mgr.running) {
        // 等待消息，超时时间为100ms
        result = rt_mq_recv(g_led_mgr.led_msg_queue, &msg, sizeof(msg), 100);
        
        if (result == RT_EOK) {
            led_process_message(&msg);
        } else if (result == -RT_ETIMEOUT) {
            // 超时是正常的，继续循环
            continue;
        } else {
            rt_thread_mdelay(10);
        }
    }
    rt_sem_release(g_led_mgr.shutdown_sem);
}

/* 处理LED消息 */
static void led_process_message(const led_message_t *msg)
{
    switch (msg->type) {
        case LED_MSG_UPDATE_TICK:
            led_do_update_effects();
            led_do_update_hardware();
            break;
            
        case LED_MSG_SET_LED:
            if (msg->data.set_led.led_index < g_led_mgr.actual_led_count) {
                g_led_mgr.manual_led_buffer[msg->data.set_led.led_index] = msg->data.set_led.color;
                g_led_mgr.manual_led_mask[msg->data.set_led.led_index] = true;
                led_do_update_hardware();
            }
            break;
            
        case LED_MSG_SET_ALL_LEDS:
            for (uint32_t i = 0; i < g_led_mgr.actual_led_count; i++) {
                g_led_mgr.manual_led_buffer[i] = msg->data.set_all.color;
                g_led_mgr.manual_led_mask[i] = true;
            }
            led_do_update_hardware();
            break;
            
        case LED_MSG_START_EFFECT:
            {
                // 查找空闲的效果槽
                int slot = -1;
                for (int i = 0; i < MAX_CONCURRENT_EFFECTS; i++) {
                    if (!g_led_mgr.effects[i].active) {
                        slot = i;
                        break;
                    }
                }
                
                int effect_id = -1;
                if (slot >= 0) {
                    led_effect_handle_internal_t *effect = &g_led_mgr.effects[slot];
                    effect->config = msg->data.start_effect.config;
                    effect->state = LED_EFFECT_STATE_RUNNING;
                    effect->start_tick = rt_tick_get();
                    effect->last_update_tick = effect->start_tick;
                    effect->effect_tick = 0;
                    effect->active = true;
                    effect->id = g_led_mgr.next_effect_id++;
                    effect_id = effect->id;
                    
                    // 参数检查和修正
                    if (effect->config.led_start >= g_led_mgr.actual_led_count) {
                        effect->config.led_start = 0;
                    }
                    if (effect->config.led_start + effect->config.led_count > g_led_mgr.actual_led_count) {
                        effect->config.led_count = g_led_mgr.actual_led_count - effect->config.led_start;
                    }
                    if (effect->config.led_count == 0) {
                        effect->config.led_count = g_led_mgr.actual_led_count;
                    }
                }
                
                // 返回效果ID
                if (msg->data.start_effect.effect_id_out) {
                    *msg->data.start_effect.effect_id_out = effect_id;
                }
                
                // 发送完成信号
                if (msg->data.start_effect.done_sem) {
                    rt_sem_release(msg->data.start_effect.done_sem);
                }
            }
            break;
            
        case LED_MSG_STOP_EFFECT:
            for (int i = 0; i < MAX_CONCURRENT_EFFECTS; i++) {
                if (g_led_mgr.effects[i].active && 
                    g_led_mgr.effects[i].id == msg->data.stop_effect.effect_id) {
                    g_led_mgr.effects[i].state = LED_EFFECT_STATE_STOPPED;
                    g_led_mgr.effects[i].active = false;
                    break;
                }
            }
            break;
            
        case LED_MSG_SET_BRIGHTNESS:
            g_led_mgr.global_brightness = msg->data.set_brightness.brightness;
            led_do_update_hardware();
            break;
            
        case LED_MSG_LED_FEEDBACK:
            {
                int led_index = msg->data.led_feedback.led_index;
                uint32_t color = msg->data.led_feedback.color;
                uint32_t duration_ms = msg->data.led_feedback.duration_ms;
                
                if (led_index >= 0 && led_index < (int)g_led_mgr.actual_led_count) {
                    rt_kprintf("[LED] High-priority feedback: LED%d=0x%06X for %ums\n", 
                            led_index, color, duration_ms);
                    g_led_mgr.manual_led_mask[led_index] = false;
                    for (int i = 0; i < MAX_CONCURRENT_EFFECTS; i++) {
                        if (!g_led_mgr.effects[i].active) {
                            led_effect_handle_internal_t *effect = &g_led_mgr.effects[i];
                            effect->config.type = LED_EFFECT_STATIC;
                            effect->config.duration_ms = duration_ms;
                            effect->config.period_ms = 100;
                            effect->config.brightness = 255;
                            effect->config.colors[0] = color;
                            effect->config.color_count = 1;
                            effect->config.led_start = led_index;
                            effect->config.led_count = 1;
                            
                            effect->state = LED_EFFECT_STATE_RUNNING;
                            effect->start_tick = rt_tick_get();
                            effect->last_update_tick = effect->start_tick;
                            effect->effect_tick = 0;
                            effect->active = true;
                            effect->id = g_led_mgr.next_effect_id++;
                            break;
                        }
                    }
                }
            }
            break;
                    
                case LED_MSG_SHUTDOWN:
                    g_led_mgr.running = false;
                    break;
                    
                default:
                    break;
            }
        }

/* 发送LED消息 */
static int led_send_message(const led_message_t *msg, bool sync)
{
    if (!g_led_mgr.led_msg_queue) {
        return -RT_ERROR;
    }
    
    rt_err_t result;
    if (sync) {
        // RT-Thread中没有rt_mq_send_wait，使用循环尝试
        int retry_count = 100; // 最多重试100次，总共1秒
        do {
            result = rt_mq_send(g_led_mgr.led_msg_queue, (void*)msg, sizeof(*msg));
            if (result == RT_EOK) {
                break;
            }
            rt_thread_mdelay(10);
            retry_count--;
        } while (retry_count > 0);
    } else {
        result = rt_mq_send(g_led_mgr.led_msg_queue, (void*)msg, sizeof(*msg));
    }
    
    return (result == RT_EOK) ? 0 : -RT_ERROR;
}

static void led_do_update_effects(void)
{
    // 清空LED缓冲区
    memset(g_led_mgr.led_buffer, 0, g_led_mgr.actual_led_count * sizeof(uint32_t));
    
    uint32_t current_tick = rt_tick_get();
    
    // 处理所有活动效果
    for (int i = 0; i < MAX_CONCURRENT_EFFECTS; i++) {
        led_effect_handle_internal_t *effect = &g_led_mgr.effects[i];
        
        if (!effect->active || effect->state != LED_EFFECT_STATE_RUNNING) {
            continue;
        }
        
        // 检查效果是否超时
        if (effect->config.duration_ms > 0) {
            uint32_t elapsed_ms = (current_tick - effect->start_tick) * 1000 / RT_TICK_PER_SECOND;
            if (elapsed_ms >= effect->config.duration_ms) {
                effect->state = LED_EFFECT_STATE_FINISHED;
                effect->active = false;
                continue;
            }
        }
        
        // 更新效果内部计时器
        uint32_t delta_ms = (current_tick - effect->last_update_tick) * 1000 / RT_TICK_PER_SECOND;
        effect->effect_tick += delta_ms;
        effect->last_update_tick = current_tick;
        
        // 应用效果
        switch (effect->config.type) {
            case LED_EFFECT_STATIC:
                apply_effect_static(effect, g_led_mgr.led_buffer);
                break;
            case LED_EFFECT_BREATHING:
                apply_effect_breathing(effect, g_led_mgr.led_buffer);
                break;
            case LED_EFFECT_FLOWING:
                apply_effect_flowing(effect, g_led_mgr.led_buffer);
                break;
            case LED_EFFECT_BLINK:
                apply_effect_blink(effect, g_led_mgr.led_buffer);
                break;
            default:
                break;
        }
    }
    
    // 应用手动设置的LED（但允许LED反馈效果覆盖）
    for (uint32_t i = 0; i < g_led_mgr.actual_led_count; i++) {
        if (g_led_mgr.manual_led_mask[i]) {
            // 检查是否有LED反馈效果正在运行
            bool has_feedback_effect = false;
            for (int j = 0; j < MAX_CONCURRENT_EFFECTS; j++) {
                led_effect_handle_internal_t *effect = &g_led_mgr.effects[j];
                if (effect->active && 
                    effect->config.led_start <= i && 
                    effect->config.led_start + effect->config.led_count > i &&
                    effect->config.duration_ms > 0 && 
                    effect->config.duration_ms <= 1000) { // 短时效果优先
                    has_feedback_effect = true;
                    break;
                }
            }
            
            // 只有没有LED反馈效果时才应用手动设置
            if (!has_feedback_effect) {
                g_led_mgr.led_buffer[i] = g_led_mgr.manual_led_buffer[i];
            }
        }
    }
}

/* 更新硬件 - 在线程上下文中执行 */
static void led_do_update_hardware(void)
{
    if (!g_led_mgr.rgb_device) {
        return;
    }
    
    // 应用全局亮度
    uint32_t *brightness_buffer = (uint32_t *)rt_malloc(g_led_mgr.actual_led_count * sizeof(uint32_t));
    if (!brightness_buffer) {
        return;
    }
    
    for (uint32_t i = 0; i < g_led_mgr.actual_led_count; i++) {
        brightness_buffer[i] = led_effects_apply_brightness(g_led_mgr.led_buffer[i], g_led_mgr.global_brightness);
    }
    
    // 使用drv_rgbled的多LED控制API
    struct rt_rgbled_multi_configuration multi_config = {
        .led_count = g_led_mgr.actual_led_count,
        .color_array = brightness_buffer
    };
    
    rt_device_control(g_led_mgr.rgb_device, RGB_CMD_SET_MULTI_COLOR, &multi_config);
    rt_free(brightness_buffer);
}

/* LED效果管理器初始化 */
int led_effects_manager_init(void)
{
    if (g_led_mgr.initialized) {
        return 0;
    }
    
    // 1. 硬件初始化
    led_effects_hardware_init();
    rt_thread_mdelay(500);
    
    // 2. 查找RGB设备
    g_led_mgr.rgb_device = rgb_find_device(NULL);
    if (!g_led_mgr.rgb_device) {
        return -RT_ERROR;
    }
    
    // 3. 获取LED数量
    uint32_t max_led_count = 0;
    rt_err_t result = rt_device_control(g_led_mgr.rgb_device, RGB_CMD_GET_CAPABILITY, &max_led_count);
    g_led_mgr.actual_led_count = (result == RT_EOK) ? max_led_count : 3;
    
    // 4. 分配缓冲区
    g_led_mgr.led_buffer = (uint32_t *)rt_malloc(g_led_mgr.actual_led_count * sizeof(uint32_t));
    g_led_mgr.manual_led_buffer = (uint32_t *)rt_malloc(g_led_mgr.actual_led_count * sizeof(uint32_t));
    g_led_mgr.manual_led_mask = (bool *)rt_malloc(g_led_mgr.actual_led_count * sizeof(bool));
    
    if (!g_led_mgr.led_buffer || !g_led_mgr.manual_led_buffer || !g_led_mgr.manual_led_mask) {
        return -RT_ENOMEM;
    }
    
    // 5. 创建消息队列
    g_led_mgr.led_msg_queue = rt_mq_create("led_mq", sizeof(led_message_t), 16, RT_IPC_FLAG_PRIO);
    if (!g_led_mgr.led_msg_queue) {
        return -RT_ENOMEM;
    }
    
    // 6. 创建关闭信号量
    g_led_mgr.shutdown_sem = rt_sem_create("led_shutdown", 0, RT_IPC_FLAG_PRIO);
    if (!g_led_mgr.shutdown_sem) {
        rt_mq_delete(g_led_mgr.led_msg_queue);
        return -RT_ENOMEM;
    }
    
    // 7. 创建LED处理线程
    g_led_mgr.led_thread = rt_thread_create("led_effects",
                                           led_effects_thread_entry,
                                           RT_NULL,
                                           LED_THREAD_STACK_SIZE,
                                           LED_THREAD_PRIORITY,
                                           10);
    if (!g_led_mgr.led_thread) {
        rt_sem_delete(g_led_mgr.shutdown_sem);
        rt_mq_delete(g_led_mgr.led_msg_queue);
        return -RT_ENOMEM;
    }
    
    // 8. 创建定时器
    g_led_mgr.update_timer = rt_timer_create("led_timer",
                                             led_update_timer_callback,
                                             RT_NULL,
                                             rt_tick_from_millisecond(LED_UPDATE_INTERVAL_MS),
                                             RT_TIMER_FLAG_PERIODIC);
    if (!g_led_mgr.update_timer) {
        return -RT_ENOMEM;
    }
    
    // 9. 初始化状态
    memset(g_led_mgr.effects, 0, sizeof(g_led_mgr.effects));
    memset(g_led_mgr.led_buffer, 0, g_led_mgr.actual_led_count * sizeof(uint32_t));
    memset(g_led_mgr.manual_led_buffer, 0, g_led_mgr.actual_led_count * sizeof(uint32_t));
    memset(g_led_mgr.manual_led_mask, false, g_led_mgr.actual_led_count * sizeof(bool));
    
    g_led_mgr.global_brightness = 255;
    g_led_mgr.next_effect_id = 1;
    g_led_mgr.running = true;
    g_led_mgr.initialized = true;
    
    // 10. 启动线程和定时器
    rt_thread_startup(g_led_mgr.led_thread);
    rt_timer_start(g_led_mgr.update_timer);
    
    // 11. 订阅LED反馈事件
    event_bus_subscribe(EVENT_LED_FEEDBACK_REQUEST, led_feedback_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);
    return 0;
}

/* LED效果管理器去初始化 */
int led_effects_manager_deinit(void)
{
    if (!g_led_mgr.initialized) {
        return 0;
    }
    
    // 1. 取消事件订阅
    event_bus_unsubscribe(EVENT_LED_FEEDBACK_REQUEST, led_feedback_event_handler);
    
    // 2. 停止定时器
    if (g_led_mgr.update_timer) {
        rt_timer_stop(g_led_mgr.update_timer);
        rt_timer_delete(g_led_mgr.update_timer);
        g_led_mgr.update_timer = RT_NULL;
    }
    
    // 2. 发送关闭消息给线程
    led_message_t shutdown_msg = {.type = LED_MSG_SHUTDOWN};
    led_send_message(&shutdown_msg, false);
    
    // 3. 等待线程结束
    rt_sem_take(g_led_mgr.shutdown_sem, 5000);
    
    // 4. 清理资源
    if (g_led_mgr.shutdown_sem) {
        rt_sem_delete(g_led_mgr.shutdown_sem);
        g_led_mgr.shutdown_sem = RT_NULL;
    }
    
    if (g_led_mgr.led_msg_queue) {
        rt_mq_delete(g_led_mgr.led_msg_queue);
        g_led_mgr.led_msg_queue = RT_NULL;
    }
    
    if (g_led_mgr.led_buffer) {
        rt_free(g_led_mgr.led_buffer);
        g_led_mgr.led_buffer = RT_NULL;
    }
    
    if (g_led_mgr.manual_led_buffer) {
        rt_free(g_led_mgr.manual_led_buffer);
        g_led_mgr.manual_led_buffer = RT_NULL;
    }
    
    if (g_led_mgr.manual_led_mask) {
        rt_free(g_led_mgr.manual_led_mask);
        g_led_mgr.manual_led_mask = RT_NULL;
    }
    
    g_led_mgr.initialized = false;
    return 0;
}

/* 公共API函数 - 通过消息队列与线程通信 */

int led_effects_set_led(uint8_t led_index, uint32_t color)
{
    if (!g_led_mgr.initialized) {
        return -RT_ERROR;
    }
    
    led_message_t msg = {
        .type = LED_MSG_SET_LED,
        .data.set_led = {
            .led_index = led_index,
            .color = color
        }
    };
    
    return led_send_message(&msg, false);
}

int led_effects_set_all_leds(uint32_t color)
{
    if (!g_led_mgr.initialized) {
        return -RT_ERROR;
    }
    
    led_message_t msg = {
        .type = LED_MSG_SET_ALL_LEDS,
        .data.set_all = {
            .color = color
        }
    };
    
    return led_send_message(&msg, false);
}

int led_effects_set_global_brightness(uint8_t brightness)
{
    if (!g_led_mgr.initialized) {
        return -RT_ERROR;
    }
    
    led_message_t msg = {
        .type = LED_MSG_SET_BRIGHTNESS,
        .data.set_brightness = {
            .brightness = brightness
        }
    };
    
    return led_send_message(&msg, false);
}

led_effect_handle_t led_effects_start_effect(const led_effect_config_t *config)
{
    if (!g_led_mgr.initialized || !config) {
        return RT_NULL;
    }
    
    int effect_id = -1;
    rt_sem_t done_sem = rt_sem_create("led_sync", 0, RT_IPC_FLAG_PRIO);
    if (!done_sem) {
        return RT_NULL;
    }
    
    led_message_t msg = {
        .type = LED_MSG_START_EFFECT,
        .data.start_effect = {
            .config = *config,
            .effect_id_out = &effect_id,
            .done_sem = done_sem
        }
    };
    
    if (led_send_message(&msg, true) == 0) {
        // 等待处理完成
        rt_sem_take(done_sem, 1000);
    }
    
    rt_sem_delete(done_sem);
    
    return (effect_id >= 0) ? (led_effect_handle_t)(uintptr_t)effect_id : RT_NULL;
}

/* LED效果渲染函数实现 */

/* 静态效果 */
static int apply_effect_static(const led_effect_handle_internal_t *effect, uint32_t *buffer)
{
    uint32_t color = effect->config.color_count > 0 ? effect->config.colors[0] : RGB_COLOR_WHITE;
    color = led_effects_apply_brightness(color, effect->config.brightness);

    for (int i = effect->config.led_start; i < effect->config.led_start + effect->config.led_count; i++) {
        if (i < (int)g_led_mgr.actual_led_count) {
            buffer[i] = color;
        }
    }
    return 0;
}

/* 呼吸灯效果 */
static int apply_effect_breathing(const led_effect_handle_internal_t *effect, uint32_t *buffer)
{
    if (effect->config.color_count < 1) return -1;

    uint32_t cycle_pos = effect->effect_tick % effect->config.period_ms;
    float phase = (float)cycle_pos / effect->config.period_ms * 2.0f * 3.14159f;
    uint8_t intensity = (uint8_t)((sin(phase) + 1.0f) * 127.5f);
    
    uint32_t color = led_effects_apply_brightness(effect->config.colors[0], intensity);
    color = led_effects_apply_brightness(color, effect->config.brightness);

    for (int i = effect->config.led_start; i < effect->config.led_start + effect->config.led_count; i++) {
        if (i < (int)g_led_mgr.actual_led_count) {
            buffer[i] = color;
        }
    }
    return 0;
}

/* 流水灯效果 */
static int apply_effect_flowing(const led_effect_handle_internal_t *effect, uint32_t *buffer)
{
    if (effect->config.color_count < 1) return -1;

    uint32_t cycle_pos = effect->effect_tick % effect->config.period_ms;
    float progress = (float)cycle_pos / effect->config.period_ms;
    
    if (effect->config.reverse) {
        progress = 1.0f - progress;
    }

    int active_led = (int)(progress * effect->config.led_count);
    uint32_t color = led_effects_apply_brightness(effect->config.colors[0], effect->config.brightness);

    // 清除范围内的LED
    for (int i = effect->config.led_start; i < effect->config.led_start + effect->config.led_count; i++) {
        if (i < (int)g_led_mgr.actual_led_count) {
            buffer[i] = RGB_COLOR_BLACK;
        }
    }

    // 点亮当前LED
    int led_index = effect->config.led_start + active_led;
    if (led_index < (int)g_led_mgr.actual_led_count && led_index >= effect->config.led_start) {
        buffer[led_index] = color;
    }
    return 0;
}

/* 闪烁效果 */
static int apply_effect_blink(const led_effect_handle_internal_t *effect, uint32_t *buffer)
{
    if (effect->config.color_count < 1) return -1;

    uint32_t cycle_pos = effect->effect_tick % effect->config.period_ms;
    bool is_on = cycle_pos < (effect->config.period_ms / 2);
    
    uint32_t color = is_on ? effect->config.colors[0] : RGB_COLOR_BLACK;
    color = led_effects_apply_brightness(color, effect->config.brightness);

    for (int i = effect->config.led_start; i < effect->config.led_start + effect->config.led_count; i++) {
        if (i < (int)g_led_mgr.actual_led_count) {
            buffer[i] = color;
        }
    }
    return 0;
}

int led_effects_stop_effect(led_effect_handle_t handle)
{
    if (!g_led_mgr.initialized || !handle) {
        return -RT_EINVAL;
    }
    
    int effect_id = (int)(uintptr_t)handle;
    
    led_message_t msg = {
        .type = LED_MSG_STOP_EFFECT,
        .data.stop_effect = {
            .effect_id = effect_id
        }
    };
    
    return led_send_message(&msg, false);
}

int led_effects_stop_all_effects(void)
{
    // 停止所有效果的简单实现
    for (int i = 1; i <= 10; i++) {
        led_effects_stop_effect((led_effect_handle_t)(uintptr_t)i);
    }
    return 0;
}

int led_effects_turn_off_all_leds(void)
{
    return led_effects_set_all_leds(RGB_COLOR_BLACK);
}

/* 预设效果函数 */
led_effect_handle_t led_effects_breathing(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t duration_ms)
{
    if (!g_led_mgr.initialized) {
        return RT_NULL;
    }

    led_effect_config_t config = {
        .type = LED_EFFECT_BREATHING,
        .duration_ms = duration_ms,
        .period_ms = period_ms,
        .brightness = brightness,
        .colors = {color, RGB_COLOR_BLACK},
        .color_count = 2,
        .reverse = false,
        .led_start = 0,
        .led_count = g_led_mgr.actual_led_count,
        .custom_data = RT_NULL
    };
    
    return led_effects_start_effect(&config);
}

led_effect_handle_t led_effects_flowing(uint32_t color, uint32_t period_ms, uint8_t brightness, uint32_t duration_ms)
{
    if (!g_led_mgr.initialized) {
        return RT_NULL;
    }

    led_effect_config_t config = {
        .type = LED_EFFECT_FLOWING,
        .duration_ms = duration_ms,
        .period_ms = period_ms,
        .brightness = brightness,
        .colors = {color, RGB_COLOR_BLACK},
        .color_count = 2,
        .reverse = false,
        .led_start = 0,
        .led_count = g_led_mgr.actual_led_count,
        .custom_data = RT_NULL
    };
    
    return led_effects_start_effect(&config);
}

/* 工具函数 */
uint32_t led_effects_apply_brightness(uint32_t color, uint8_t brightness)
{
    if (brightness == 0) return 0;
    if (brightness == 255) return color;
    
    uint8_t r = RGB_GET_RED(color);
    uint8_t g = RGB_GET_GREEN(color);
    uint8_t b = RGB_GET_BLUE(color);
    
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    return RGB_MAKE_COLOR(r, g, b);
}