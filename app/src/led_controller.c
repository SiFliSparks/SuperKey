#include "led_controller.h"
#include "event_bus.h"
#include <drivers/rt_drv_pwm.h>
#include <string.h>
#include "bf0_hal.h"
#include "drv_io.h"

#define RGBLED_DEVICE_NAME  "rgbled"
#define RGB_LED_COUNT       3

static struct rt_device *rgbled_device = RT_NULL;
static rt_mutex_t led_mutex = RT_NULL;
static rt_timer_t led_timers[RGB_LED_COUNT] = {RT_NULL};
static uint32_t current_colors[RGB_LED_COUNT] = {0};

static int led_feedback_event_handler(const event_t *event, void *user_data)
{
    (void)user_data;
    
    if (event->type == EVENT_LED_FEEDBACK_REQUEST) {
        const event_data_led_t *led_data = &event->data.led;
        
        rt_kprintf("[LED] Received LED feedback event: led=%d, color=0x%06X, duration=%dms\n",
                  led_data->led_index, led_data->color, led_data->duration_ms);
        
        if (led_controller_is_ready()) {
            led_controller_light_led(led_data->led_index, led_data->color, led_data->duration_ms);
            return 0;
        } else {
            rt_kprintf("[LED] LED controller not ready\n");
            return -1;
        }
    }
    
    return -1;
}

static void led_timer_callback(void *parameter)
{
    int led_index = (int)(rt_ubase_t)parameter;
    
    if (led_index >= 0 && led_index < RGB_LED_COUNT) {
        rt_mutex_take(led_mutex, RT_WAITING_FOREVER);
        current_colors[led_index] = LED_COLOR_OFF;
        
        led_controller_set_led_colors(current_colors, RGB_LED_COUNT);
        
        rt_mutex_release(led_mutex);
    }
}

static rt_err_t _set_single_led_color(uint32_t color)
{
    struct rt_rgbled_configuration cfg;
    rt_memset(&cfg, 0, sizeof(cfg));
    cfg.color_rgb = color;
    
    return rt_device_control(rgbled_device, PWM_CMD_SET_COLOR, &cfg);
}

static rt_err_t _set_led_colors_array(const uint32_t *colors, uint16_t count)
{
    if (count == 0) return RT_EOK;

    struct rt_rgbled_color_array arr;
    rt_memset(&arr, 0, sizeof(arr));
    arr.count = count;
    arr.colors = (rt_uint32_t *)colors;

    return rt_device_control(rgbled_device, PWM_CMD_SET_COLOR_ARRAY, &arr);
}

int led_controller_init(void)
{
    if (rgbled_device != RT_NULL) {
        rt_kprintf("[LED] Already initialized\n");
        return 0;
    }

    rt_kprintf("[LED] Configuring WS2812 hardware pins...\n");
    
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO3_3V3, true, true);
    HAL_PIN_Set(PAD_PA10, GPTIM2_CH1, PIN_NOPULL, 1);
    
    rt_kprintf("[LED] Hardware pins configured: PA10 -> GPTIM2_CH1\n");

    rgbled_device = rt_device_find(RGBLED_DEVICE_NAME);
    if (!rgbled_device) {
        rt_kprintf("[LED] ERROR: device '%s' not found!\n", RGBLED_DEVICE_NAME);
        return -RT_ERROR;
    }
    
    rt_err_t ret = rt_device_open(rgbled_device, RT_DEVICE_FLAG_WRONLY);
    if (ret != RT_EOK) {
        rt_kprintf("[LED] ERROR: Cannot open device: %d\n", ret);
        rgbled_device = RT_NULL;
        return -RT_ERROR;
    }
    
    led_mutex = rt_mutex_create("led_mutex", RT_IPC_FLAG_PRIO);
    if (!led_mutex) {
        rt_kprintf("[LED] Failed to create mutex\n");
        rt_device_close(rgbled_device);
        rgbled_device = RT_NULL;
        return -RT_ENOMEM;
    }

    for (int i = 0; i < RGB_LED_COUNT; i++) {
        char timer_name[16];
        rt_snprintf(timer_name, sizeof(timer_name), "led_timer%d", i);
        led_timers[i] = rt_timer_create(timer_name, led_timer_callback, 
                                       (void*)(rt_ubase_t)i, 
                                       rt_tick_from_millisecond(1000),
                                       RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
        if (!led_timers[i]) {
            rt_kprintf("[LED] Failed to create timer %d\n", i);
            led_controller_deinit();
            return -RT_ENOMEM;
        }
        current_colors[i] = LED_COLOR_OFF;
    }

    led_controller_turn_off_all();

    event_bus_subscribe(EVENT_LED_FEEDBACK_REQUEST, led_feedback_event_handler, 
                       NULL, EVENT_PRIORITY_NORMAL);

    rt_kprintf("[LED] LED controller initialized using SDK rgbled device\n");
    return 0;
}

int led_controller_deinit(void)
{
    if (!rgbled_device) {
        return 0;
    }

    event_bus_unsubscribe(EVENT_LED_FEEDBACK_REQUEST, led_feedback_event_handler);
    led_controller_turn_off_all();

    for (int i = 0; i < RGB_LED_COUNT; i++) {
        if (led_timers[i]) {
            rt_timer_stop(led_timers[i]);
            rt_timer_delete(led_timers[i]);
            led_timers[i] = RT_NULL;
        }
    }

    if (led_mutex) {
        rt_mutex_delete(led_mutex);
        led_mutex = RT_NULL;
    }

    rt_device_close(rgbled_device);
    rgbled_device = RT_NULL;
    
    rt_kprintf("[LED] LED controller deinitialized\n");
    return 0;
}

int led_controller_light_led(int led_index, uint32_t color, uint32_t duration_ms)
{
    if (!rgbled_device || !led_mutex) {
        rt_kprintf("[LED] Controller not initialized\n");
        return -RT_ERROR;
    }

    if (led_index < 0 || led_index >= RGB_LED_COUNT) {
        rt_kprintf("[LED] Invalid LED index: %d\n", led_index);
        return -RT_EINVAL;
    }

    rt_mutex_take(led_mutex, RT_WAITING_FOREVER);

    if (led_timers[led_index]) {
        rt_timer_stop(led_timers[led_index]);
    }

    current_colors[led_index] = color;

    rt_err_t ret = _set_led_colors_array(current_colors, RGB_LED_COUNT);
    if (ret != RT_EOK) {
        rt_kprintf("[LED] Failed to set LED array: %d\n", ret);
    }

    if (led_timers[led_index] && duration_ms > 0) {
        rt_timer_control(led_timers[led_index], RT_TIMER_CTRL_SET_TIME, 
                        &(rt_tick_t){rt_tick_from_millisecond(duration_ms)});
        rt_timer_start(led_timers[led_index]);
    }

    rt_mutex_release(led_mutex);
    return 0;
}

int led_controller_turn_off_all(void)
{
    if (!rgbled_device || !led_mutex) {
        return -RT_ERROR;
    }

    rt_mutex_take(led_mutex, RT_WAITING_FOREVER);

    for (int i = 0; i < RGB_LED_COUNT; i++) {
        if (led_timers[i]) {
            rt_timer_stop(led_timers[i]);
        }
        current_colors[i] = LED_COLOR_OFF;
    }

    rt_err_t ret = _set_led_colors_array(current_colors, RGB_LED_COUNT);
    if (ret != RT_EOK) {
        rt_kprintf("[LED] Failed to turn off all LEDs: %d\n", ret);
    }

    rt_mutex_release(led_mutex);
    return 0;
}

int led_controller_set_led_color(int led_index, uint32_t color)
{
    if (!rgbled_device || !led_mutex) {
        return -RT_ERROR;
    }

    if (led_index < 0 || led_index >= RGB_LED_COUNT) {
        return -RT_EINVAL;
    }

    rt_mutex_take(led_mutex, RT_WAITING_FOREVER);

    current_colors[led_index] = color;

    rt_err_t ret = _set_led_colors_array(current_colors, RGB_LED_COUNT);
    if (ret != RT_EOK) {
        rt_kprintf("[LED] Failed to set LED color: %d\n", ret);
    }

    rt_mutex_release(led_mutex);
    return 0;
}

int led_controller_set_led_colors(const uint32_t *colors, uint16_t count)
{
    if (!rgbled_device || !led_mutex || !colors) {
        return -RT_ERROR;
    }

    if (count > RGB_LED_COUNT) {
        count = RGB_LED_COUNT;
    }

    rt_mutex_take(led_mutex, RT_WAITING_FOREVER);

    for (uint16_t i = 0; i < count; i++) {
        current_colors[i] = colors[i];
    }

    rt_err_t ret = _set_led_colors_array(current_colors, RGB_LED_COUNT);
    if (ret != RT_EOK) {
        rt_kprintf("[LED] Failed to set LED colors: %d\n", ret);
    }

    rt_mutex_release(led_mutex);
    return 0;
}

bool led_controller_is_ready(void)
{
    return rgbled_device != RT_NULL && led_mutex != RT_NULL;
}