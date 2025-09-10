#include "encoder_controller.h"
#include <drivers/rt_drv_encoder.h>
#include "bf0_hal.h"
#include <board.h>
#include <string.h>
#include "event_bus.h"

#define ENCODER_DEVICE_NAME1 "encoder1"
#define ENCODER_POLLING_PERIOD_MS 10

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
    
    rt_int16_t current_count;
    rt_size_t bytes_read = rt_device_read(g_encoder.device, 0, &current_count, sizeof(current_count));
    
    if (bytes_read == sizeof(current_count)) {
        rt_mutex_take(g_encoder.lock, RT_WAITING_FOREVER);
        
        int32_t delta = current_count - g_encoder.last_count;
        if (g_encoder.sensitivity > 1) {
            delta = delta / g_encoder.sensitivity;
        }
        
        if (delta != 0) {
            g_encoder.last_count = current_count;
            g_encoder.total_count += delta;
            
            event_data_encoder_t encoder_event = {
                .delta = delta,
                .total_count = g_encoder.total_count,
                .user_data = NULL
            };
            
            event_bus_publish(EVENT_ENCODER_ROTATED, &encoder_event, sizeof(encoder_event),
                             EVENT_PRIORITY_HIGH, MODULE_ID_ENCODER);
        }
        
        rt_mutex_release(g_encoder.lock);
    }
}

int encoder_controller_init(void)
{
    if (g_encoder.initialized) {
        rt_kprintf("[Encoder] Already initialized\n");
        return 0;
    }
    
    rt_kprintf("[Encoder] Initializing optimized encoder controller...\n");
    
    HAL_PIN_Set(PAD_PA43, GPTIM1_CH1, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA41, GPTIM1_CH2, PIN_NOPULL, 1);
    rt_kprintf("[Encoder] GPIO pins configured: PA37/PA38 -> GPTIM1_CH1/CH2\n");
    
    g_encoder.device = rt_device_find(ENCODER_DEVICE_NAME1);
    if (g_encoder.device == RT_NULL) {
        rt_kprintf("[Encoder] Failed to find %s device\n", ENCODER_DEVICE_NAME1);
        return -RT_ERROR;
    }
    
    rt_err_t result = rt_device_open(g_encoder.device, RT_DEVICE_OFLAG_RDWR);
    if (result != RT_EOK) {
        rt_kprintf("[Encoder] Failed to open device: %d\n", result);
        return result;
    }
    
    g_encoder.lock = rt_mutex_create("enc_lock", RT_IPC_FLAG_PRIO);
    if (!g_encoder.lock) {
        rt_kprintf("[Encoder] Failed to create mutex\n");
        rt_device_close(g_encoder.device);
        return -RT_ENOMEM;
    }
    
    g_encoder.polling_timer = rt_timer_create("enc_timer",
                                             encoder_polling_timer_cb,
                                             RT_NULL,
                                             rt_tick_from_millisecond(ENCODER_POLLING_PERIOD_MS),
                                             RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    
    if (g_encoder.polling_timer == RT_NULL) {
        rt_kprintf("[Encoder] Failed to create polling timer\n");
        rt_mutex_delete(g_encoder.lock);
        rt_device_close(g_encoder.device);
        return -RT_ENOMEM;
    }
    
    g_encoder.mode = ENCODER_MODE_IDLE;
    g_encoder.sensitivity = 1;
    g_encoder.last_count = 0;
    g_encoder.total_count = 0;
    g_encoder.polling_enabled = false;
    g_encoder.initialized = true;
    
    rt_kprintf("[Encoder] Optimized encoder controller initialized (timer-based)\n");
    return 0;
}

int encoder_controller_deinit(void)
{
    if (!g_encoder.initialized) {
        return 0;
    }
    
    rt_kprintf("[Encoder] Deinitializing optimized encoder controller...\n");
    
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
    
    rt_kprintf("[Encoder] Optimized encoder controller deinitialized\n");
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
    
    rt_kprintf("[Encoder] Mode set to: %s\n", encoder_mode_names[mode]);
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
        
        rt_kprintf("[Encoder] Count reset to 0\n");
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
        rt_kprintf("[Encoder] Not initialized\n");
        return -RT_ERROR;
    }
    
    if (g_encoder.polling_enabled) {
        rt_kprintf("[Encoder] Polling already enabled\n");
        return 0;
    }
    
    uint32_t test_pub, test_proc, test_drop, test_queue;
    if (event_bus_get_stats(&test_pub, &test_proc, &test_drop, &test_queue) == 0) {
        rt_kprintf("[Encoder] Event bus ready: pub=%u, proc=%u, queue=%u\n", 
                  test_pub, test_proc, test_queue);
    } else {
        rt_kprintf("[Encoder] WARNING: Event bus not ready, but starting polling anyway\n");
    }
    
    rt_err_t result = rt_timer_start(g_encoder.polling_timer);
    if (result != RT_EOK) {
        rt_kprintf("[Encoder] Failed to start polling timer: %d\n", result);
        return result;
    }
    
    g_encoder.polling_enabled = true;
    
    rt_thread_mdelay(100);
    
    rt_kprintf("[Encoder] Polling started with event bus integration (period=%dms)\n", ENCODER_POLLING_PERIOD_MS);
    return 0;
}

int encoder_controller_stop_polling(void)
{
    if (!g_encoder.initialized || !g_encoder.polling_enabled) {
        return 0;
    }
    
    rt_timer_stop(g_encoder.polling_timer);
    g_encoder.polling_enabled = false;
    
    rt_kprintf("[Encoder] Polling stopped\n");
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
    
    rt_kprintf("[Encoder] Sensitivity set to 1/%d\n", divider);
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
    rt_kprintf("[Encoder] Screen switch control managed by upper layer\n");
    return enable ? 0 : 0;
}

bool encoder_controller_is_screen_switch_enabled(void)
{
    return true;
}