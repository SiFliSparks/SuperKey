#include "buttons_board.h"
#include "bf0_hal.h"
#include <board.h>
#include <string.h>

#ifndef BUTTON_ADV_ACTION_CHECK_DELAY
#define BUTTON_ADV_ACTION_CHECK_DELAY 800
#endif

// KEY4专用去抖配置
#define KEY4_DEBOUNCE_TIME_MS       50    // KEY4去抖时间50ms
#define KEY4_STABLE_COUNT          3      // 需要连续3次稳定读取
#define KEY4_FILTER_WINDOW         5      // 滤波窗口大小

// KEY4去抖状态结构
typedef struct {
    bool last_stable_state;              // 最后稳定状态
    bool current_raw_state;              // 当前原始状态
    rt_tick_t last_change_time;          // 最后变化时间
    uint8_t stable_count;                // 稳定计数
    bool filter_buffer[KEY4_FILTER_WINDOW]; // 数字滤波缓冲区
    uint8_t filter_index;                // 滤波索引
    bool debounce_in_progress;           // 是否正在去抖
} key4_debounce_t;

static const struct {
    int32_t pin;
    int pad;
    pin_function func;
} s_button_configs[BUTTON_COUNT] = {
    {26, PAD_PA26, GPIO_A26},
    {33, PAD_PA33, GPIO_A33},
    {32, PAD_PA32, GPIO_A32},
    {40, PAD_PA40, GPIO_A40}
};

static int32_t s_button_ids[BUTTON_COUNT] = {-1, -1, -1, -1};
static button_handler_t s_unified_callback = NULL;
static key4_debounce_t s_key4_debounce = {0};
static rt_timer_t s_key4_timer = RT_NULL;

// KEY4数字滤波函数
static bool key4_digital_filter(bool raw_state)
{
    // 更新滤波缓冲区
    s_key4_debounce.filter_buffer[s_key4_debounce.filter_index] = raw_state;
    s_key4_debounce.filter_index = (s_key4_debounce.filter_index + 1) % KEY4_FILTER_WINDOW;
    
    // 统计高电平数量
    uint8_t high_count = 0;
    for (int i = 0; i < KEY4_FILTER_WINDOW; i++) {
        if (s_key4_debounce.filter_buffer[i]) {
            high_count++;
        }
    }
    
    // 多数表决滤波
    return (high_count > KEY4_FILTER_WINDOW / 2);
}

// KEY4定时器回调函数
static void key4_timer_callback(void *parameter)
{
    // 读取当前GPIO状态 - 使用RT-Thread标准API
    rt_base_t level = rt_pin_read(s_button_configs[BUTTON_KEY4].pin);
    bool current_state = (level == PIN_LOW); // 按钮是低电平有效
    
    // 数字滤波
    bool filtered_state = key4_digital_filter(current_state);
    
    // 检查状态是否稳定
    if (filtered_state == s_key4_debounce.current_raw_state) {
        s_key4_debounce.stable_count++;
        
        // 达到稳定次数要求
        if (s_key4_debounce.stable_count >= KEY4_STABLE_COUNT) {
            // 状态真正改变了
            if (filtered_state != s_key4_debounce.last_stable_state) {
                s_key4_debounce.last_stable_state = filtered_state;
                s_key4_debounce.debounce_in_progress = false;
                
                // 发送按钮事件
                if (s_unified_callback) {
                    button_action_t action = filtered_state ? BUTTON_PRESSED : BUTTON_RELEASED;
                    s_unified_callback(s_button_configs[BUTTON_KEY4].pin, action);
                }
            } else {
                // 误触发，停止定时器
                s_key4_debounce.debounce_in_progress = false;
            }
            
            // 停止定时器
            rt_timer_stop(s_key4_timer);
        }
    } else {
        // 状态发生变化，重置计数
        s_key4_debounce.current_raw_state = filtered_state;
        s_key4_debounce.stable_count = 1;
        s_key4_debounce.last_change_time = rt_tick_get();
    }
}

// KEY4专用中断处理函数
static void key4_interrupt_handler(void *args)
{
    rt_tick_t current_time = rt_tick_get();
    
    // 如果正在去抖过程中，忽略新的中断
    if (s_key4_debounce.debounce_in_progress) {
        return;
    }
    
    // 启动去抖过程
    s_key4_debounce.debounce_in_progress = true;
    s_key4_debounce.last_change_time = current_time;
    s_key4_debounce.stable_count = 0;
    
    // 读取当前GPIO状态 - 使用更通用的方法
    rt_base_t level = rt_pin_read(s_button_configs[BUTTON_KEY4].pin);
    bool initial_state = (level == PIN_LOW); // 按钮是低电平有效
    
    // 初始化滤波缓冲区
    for (int i = 0; i < KEY4_FILTER_WINDOW; i++) {
        s_key4_debounce.filter_buffer[i] = initial_state;
    }
    s_key4_debounce.filter_index = 0;
    s_key4_debounce.current_raw_state = initial_state;
    
    // 启动定时器进行周期性检查
    rt_timer_start(s_key4_timer);
}

// 标准按钮适配器（KEY1-KEY3使用）
static void sdk_button_adapter(int32_t pin, button_action_t action)
{
    if (s_unified_callback) {
        s_unified_callback(pin, action);
    }
}

int buttons_board_init(button_handler_t unified_callback)
{
    if (!unified_callback) {
        return -RT_EINVAL;
    }
    
    s_unified_callback = unified_callback;
    
    // 初始化KEY4的定时器
    s_key4_timer = rt_timer_create("key4_debounce", 
                                   key4_timer_callback,
                                   RT_NULL,
                                   rt_tick_from_millisecond(10), // 10ms周期检查
                                   RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    
    if (s_key4_timer == RT_NULL) {
        return -RT_ERROR;
    }
    
    // 初始化KEY4去抖状态
    memset(&s_key4_debounce, 0, sizeof(s_key4_debounce));
    
    // 初始化按钮
    for (int i = 0; i < BUTTON_COUNT; i++) {
        HAL_PIN_Set(s_button_configs[i].pad, s_button_configs[i].func, PIN_PULLUP, 1);
        
        // KEY4使用特殊处理
        if (i == BUTTON_KEY4) {
            // 配置KEY4为中断模式 - 使用RT-Thread标准API
            rt_pin_mode(s_button_configs[i].pin, PIN_MODE_INPUT_PULLUP);
            rt_pin_attach_irq(s_button_configs[i].pin, PIN_IRQ_MODE_RISING_FALLING, 
                             key4_interrupt_handler, NULL);
            rt_pin_irq_enable(s_button_configs[i].pin, PIN_IRQ_ENABLE);
            
            // 初始化KEY4状态
            rt_base_t level = rt_pin_read(s_button_configs[i].pin);
            s_key4_debounce.last_stable_state = (level == PIN_LOW);
        } else {
            // KEY1-KEY3使用SDK驱动
            button_cfg_t cfg = {
                .pin = s_button_configs[i].pin,
                .active_state = BUTTON_ACTIVE_LOW,
                .mode = PIN_MODE_INPUT_PULLUP,
                .button_handler = sdk_button_adapter,
                .debounce_time = 2  // 标准去抖时间
            };
            
            s_button_ids[i] = button_init(&cfg);
            if (s_button_ids[i] < 0) {
                // 清理已初始化的资源
                for (int j = 0; j < i; j++) {
                    if (s_button_ids[j] >= 0) {
                        button_disable(s_button_ids[j]);
                        s_button_ids[j] = -1;
                    }
                }
                if (s_key4_timer) {
                    rt_timer_delete(s_key4_timer);
                    s_key4_timer = RT_NULL;
                }
                return -RT_ERROR;
            }
            
            if (button_enable(s_button_ids[i]) != SF_EOK) {
                return -RT_ERROR;
            }
        }
    }
    return RT_EOK;
}

int buttons_board_pin_to_idx(int32_t pin)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (s_button_configs[i].pin == pin) {
            return i;
        }
    }
    return -1;
}

int buttons_board_count(void)
{
    return BUTTON_COUNT;
}

int buttons_board_deinit(void)
{
    
    // 清理KEY4资源
    if (s_key4_timer) {
        rt_timer_stop(s_key4_timer);
        rt_timer_delete(s_key4_timer);
        s_key4_timer = RT_NULL;
    }
    
    rt_pin_irq_enable(s_button_configs[BUTTON_KEY4].pin, PIN_IRQ_DISABLE);
    rt_pin_detach_irq(s_button_configs[BUTTON_KEY4].pin);
    
    // 清理其他按钮
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (i != BUTTON_KEY4 && s_button_ids[i] >= 0) {
            button_disable(s_button_ids[i]);
            s_button_ids[i] = -1;
        }
    }
    
    s_unified_callback = NULL;
    return RT_EOK;
}

int buttons_board_enable(int key_idx)
{
    if (key_idx < 0 || key_idx >= BUTTON_COUNT) {
        return -RT_EINVAL;
    }
    
    if (key_idx == BUTTON_KEY4) {
        // KEY4启用中断
        rt_pin_irq_enable(s_button_configs[key_idx].pin, PIN_IRQ_ENABLE);
        return RT_EOK;
    } else {
        if (s_button_ids[key_idx] < 0) {
            return -RT_EINVAL;
        }
        return (button_enable(s_button_ids[key_idx]) == SF_EOK) ? RT_EOK : -RT_ERROR;
    }
}

int buttons_board_disable(int key_idx)
{
    if (key_idx < 0 || key_idx >= BUTTON_COUNT) {
        return -RT_EINVAL;
    }
    
    if (key_idx == BUTTON_KEY4) {
        // KEY4禁用中断和定时器
        rt_pin_irq_enable(s_button_configs[key_idx].pin, PIN_IRQ_DISABLE);
        if (s_key4_timer) {
            rt_timer_stop(s_key4_timer);
        }
        s_key4_debounce.debounce_in_progress = false;
        return RT_EOK;
    } else {
        if (s_button_ids[key_idx] < 0) {
            return -RT_EINVAL;
        }
        return (button_disable(s_button_ids[key_idx]) == SF_EOK) ? RT_EOK : -RT_ERROR;
    }
}

bool buttons_board_is_pressed(int key_idx)
{
    if (key_idx < 0 || key_idx >= BUTTON_COUNT) {
        return false;
    }
    
    if (key_idx == BUTTON_KEY4) {
        // KEY4返回去抖后的稳定状态
        return s_key4_debounce.last_stable_state;
    } else {
        if (s_button_ids[key_idx] < 0) {
            return false;
        }
        return button_is_pressed(s_button_ids[key_idx]);
    }
}