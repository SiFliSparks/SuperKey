#include "encoder_controller.h"
#include <drivers/rt_drv_encoder.h>
#include "bf0_hal.h"
#include <board.h>
#include <string.h>
#include "event_bus.h"

#define ENCODER_DEVICE_NAME1 "encoder1"
#define ENCODER_POLLING_PERIOD_MS 10 //机械采集去抖
#define ENCODER_MIN_EVENT_INTERVAL_MS 300 //事件去抖时间间隔
#define ENCODER_PULSE_THRESHOLD 4//每4个脉冲算1格

static struct {
    struct rt_device *device;
    encoder_mode_t mode;
    int32_t last_count;
    int32_t total_count;
    uint8_t sensitivity;
    rt_timer_t polling_timer;
    bool initialized;
    bool polling_enabled;
    rt_mutex_t lock;
    
    // 用于软件去抖的时间戳
    rt_tick_t last_event_time;
} g_encoder = {0};

static const char* encoder_mode_names[] = {
    "IDLE", "VOLUME", "SCROLL", "BRIGHTNESS", "MENU_NAV", "SCREEN_SWITCH", "CUSTOM"
};

static void encoder_polling_timer_cb(void *parameter)
{
    (void)parameter;
    
    if (!g_encoder.device || !g_encoder.polling_enabled) {
        return;
    }
    
    // 读取编码器当前计数
    rt_int16_t current_count;
    rt_size_t bytes_read = rt_device_read(g_encoder.device, 0, &current_count, sizeof(current_count));
    
    if (bytes_read == sizeof(current_count)) {
        rt_mutex_take(g_encoder.lock, RT_WAITING_FOREVER);
        
        // 计算原始脉冲变化量
        int32_t raw_delta = current_count - g_encoder.last_count;
        
        // ✅ 关键修改：阈值过滤
        // 只处理绝对值 >= 3 的变化
        if (raw_delta != 0) {
            int32_t abs_delta = (raw_delta > 0) ? raw_delta : -raw_delta;
            
            if (abs_delta >= ENCODER_PULSE_THRESHOLD) {
                rt_tick_t current_time = rt_tick_get();
                rt_tick_t time_since_last = current_time - g_encoder.last_event_time;
                
                // 最小50ms间隔，用于过滤机械抖动
                if (time_since_last >= rt_tick_from_millisecond(ENCODER_MIN_EVENT_INTERVAL_MS)) {
                    
                    // 计算格数：每3个脉冲算作1格
                    // 例如：raw_delta=7 → delta=2格（7/3=2余1）
                    //       raw_delta=-5 → delta=-1格（-5/3=-1余-2）
                    int32_t delta = raw_delta / ENCODER_PULSE_THRESHOLD;
                    
                    // 保留余数，避免丢失脉冲
                    int32_t remainder = raw_delta % ENCODER_PULSE_THRESHOLD;
                    g_encoder.last_count = current_count - remainder;
                    
                    g_encoder.total_count += delta;
                    g_encoder.last_event_time = current_time;
                    
                    // 发布事件：delta 是"格数"
                    event_data_encoder_t encoder_event = {
                        .delta = delta,
                        .total_count = g_encoder.total_count,
                        .user_data = NULL
                    };
                    
                    event_bus_publish(EVENT_ENCODER_ROTATED, &encoder_event, sizeof(encoder_event),
                                     EVENT_PRIORITY_HIGH, MODULE_ID_ENCODER);
                }
            } else {
            }
        }
        
        rt_mutex_release(g_encoder.lock);
    }
}

int encoder_controller_init(void)
{
    if (g_encoder.initialized) {
        return 0;
    }
    
    HAL_PIN_Set(PAD_PA43, GPTIM1_CH1, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA41, GPTIM1_CH2, PIN_NOPULL, 1);
    
    g_encoder.device = rt_device_find(ENCODER_DEVICE_NAME1);
    if (g_encoder.device == RT_NULL) {
        return -RT_ERROR;
    }
    
    rt_err_t result = rt_device_open(g_encoder.device, RT_DEVICE_OFLAG_RDWR);
    if (result != RT_EOK) {
        return result;
    }
    
    g_encoder.lock = rt_mutex_create("enc_lock", RT_IPC_FLAG_PRIO);
    if (!g_encoder.lock) {
        rt_device_close(g_encoder.device);
        return -RT_ENOMEM;
    }
    
    g_encoder.polling_timer = rt_timer_create("enc_timer",
                                             encoder_polling_timer_cb,
                                             RT_NULL,
                                             rt_tick_from_millisecond(ENCODER_POLLING_PERIOD_MS),
                                             RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    
    if (g_encoder.polling_timer == RT_NULL) {
        rt_mutex_delete(g_encoder.lock);
        rt_device_close(g_encoder.device);
        return -RT_ENOMEM;
    }
    
    g_encoder.mode = ENCODER_MODE_IDLE;
    g_encoder.sensitivity = 1;  // 不再使用 sensitivity，改用阈值
    g_encoder.last_count = 0;
    g_encoder.total_count = 0;
    g_encoder.polling_enabled = false;
    g_encoder.last_event_time = 0;
    
    g_encoder.initialized = true;
    return 0;
}

int encoder_controller_deinit(void)
{
    if (!g_encoder.initialized) {
        return 0;
    }
    
    encoder_controller_stop_polling();
    
    if (g_encoder.polling_timer) {
        rt_timer_delete(g_encoder.polling_timer);
        g_encoder.polling_timer = NULL;
    }
    
    if (g_encoder.device) {
        rt_device_close(g_encoder.device);
        g_encoder.device = NULL;
    }
    
    if (g_encoder.lock) {
        rt_mutex_delete(g_encoder.lock);
        g_encoder.lock = NULL;
    }
    
    memset(&g_encoder, 0, sizeof(g_encoder));
    return 0;
}

int encoder_controller_set_mode(encoder_mode_t mode)
{
    if (!g_encoder.initialized) {
        return -RT_ERROR;
    }
    
    if (mode >= ENCODER_MODE_MAX) {
        return -RT_EINVAL;
    }
    
    rt_mutex_take(g_encoder.lock, RT_WAITING_FOREVER);
    g_encoder.mode = mode;
    rt_mutex_release(g_encoder.lock);
    return 0;
}

encoder_mode_t encoder_controller_get_mode(void)
{
    return g_encoder.mode;
}

int32_t encoder_controller_get_count(void)
{
    if (!g_encoder.initialized || !g_encoder.device) {
        return 0;
    }
    
    rt_int16_t current_count;
    rt_size_t result = rt_device_read(g_encoder.device, 0, &current_count, sizeof(current_count));
    
    if (result == sizeof(current_count)) {
        return current_count;
    }
    
    return 0;
}

int encoder_controller_reset_count(void)
{
    if (!g_encoder.initialized || !g_encoder.device) {
        return -RT_ERROR;
    }
    
    rt_int16_t zero_count = 0;
    rt_size_t result = rt_device_write(g_encoder.device, 0, &zero_count, sizeof(zero_count));
    
    if (result == sizeof(zero_count)) {
        rt_mutex_take(g_encoder.lock, RT_WAITING_FOREVER);
        g_encoder.last_count = 0;
        g_encoder.total_count = 0;
        rt_mutex_release(g_encoder.lock);
        return 0;
    }
    
    return -RT_ERROR;
}

int32_t encoder_controller_get_delta(void)
{
    if (!g_encoder.initialized || !g_encoder.device) {
        return 0;
    }
    
    int32_t current_count = encoder_controller_get_count();
    int32_t delta;
    
    rt_mutex_take(g_encoder.lock, RT_WAITING_FOREVER);
    delta = current_count - g_encoder.last_count;
    g_encoder.last_count = current_count;
    rt_mutex_release(g_encoder.lock);
    
    return delta;
}

int encoder_controller_start_polling(void)
{
    if (!g_encoder.initialized) {
        return -RT_ERROR;
    }
    
    if (g_encoder.polling_enabled) {
        return 0;
    }
    
    uint32_t test_pub, test_proc, test_drop, test_queue;
    
    rt_err_t result = rt_timer_start(g_encoder.polling_timer);
    if (result != RT_EOK) {
        return result;
    }
    
    g_encoder.polling_enabled = true;
    
    rt_thread_mdelay(100);
    return 0;
}

int encoder_controller_stop_polling(void)
{
    if (!g_encoder.initialized || !g_encoder.polling_enabled) {
        return 0;
    }
    
    rt_timer_stop(g_encoder.polling_timer);
    g_encoder.polling_enabled = false;
    return 0;
}

int encoder_controller_set_sensitivity(uint8_t divider)
{
    if (!g_encoder.initialized) {
        return -RT_ERROR;
    }
    
    if (divider == 0) {
        divider = 1;
    }
    
    rt_mutex_take(g_encoder.lock, RT_WAITING_FOREVER);
    g_encoder.sensitivity = divider;
    rt_mutex_release(g_encoder.lock);
    return 0;
}

const char* encoder_controller_get_mode_name(encoder_mode_t mode)
{
    if (mode >= ENCODER_MODE_MAX) {
        return "UNKNOWN";
    }
    return encoder_mode_names[mode];
}

bool encoder_controller_is_ready(void)
{
    return g_encoder.initialized && (g_encoder.device != NULL);
}

int encoder_controller_enable_screen_switch(bool enable)
{
    return enable ? 0 : 0;
}

bool encoder_controller_is_screen_switch_enabled(void)
{
    return true;
}