/**
 * @file power_manager.c
 * @brief Power management implementation.
 *
 * Sleep: set state -> wait LVGL idle -> save ctx -> stop LEDs -> LCD off -> backlight off -> stop sensor
 * Wake:  set state -> LCD on -> wait 300ms -> restore LEDs -> resume sensor -> invalidate LVGL -> wait 200ms -> backlight on
 */

#include "power_manager.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "../middleware/event_bus.h"
#include "../manager/led_effects_manager.h"
#include "../device/sht30_controller.h"
#include "../screen/screen.h"
#include "lvgl.h"
#include "drv_io.h"
#include "bf0_hal.h"

#define LOG_TAG "pwr_mgr"
#include <rtdbg.h>

/* ---------- Backlight control (PA08, active-low) ---------- */

#define LCD_BACKLIGHT_PIN  8

static bool g_bl_gpio_inited = false;

static void backlight_set(int on)
{
    GPIO_TypeDef *gpio = hwp_gpio1;
    GPIO_PinState target = on ? GPIO_PIN_RESET : GPIO_PIN_SET;

    if (!g_bl_gpio_inited) {
        HAL_PIN_Set(PAD_PA08, GPIO_A8, PIN_NOPULL, 1);
        /* Pre-set output level before Init to avoid glitch */
        HAL_GPIO_WritePin(gpio, LCD_BACKLIGHT_PIN, target);
        GPIO_InitTypeDef cfg = { .Pin = LCD_BACKLIGHT_PIN,
                                 .Mode = GPIO_MODE_OUTPUT,
                                 .Pull = GPIO_NOPULL };
        HAL_GPIO_Init(gpio, &cfg);
        g_bl_gpio_inited = true;
        rt_kprintf("[PWR] Backlight GPIO configured\n");
    } else {
        HAL_GPIO_WritePin(gpio, LCD_BACKLIGHT_PIN, target);
    }
}

static void backlight_on(void)  { backlight_set(1); rt_kprintf("[PWR] Backlight ON\n"); }
static void backlight_off(void) { backlight_set(0); rt_kprintf("[PWR] Backlight OFF\n"); }

/* ---------- Internal state ---------- */

static power_state_t g_power_state = POWER_STATE_NORMAL;
static rt_device_t   g_lcd_device  = RT_NULL;

static struct {
    screen_group_t saved_group;
    uint8_t        saved_led_brightness;
} g_sleep_ctx = {0};

/* ---------- Event bus callback ---------- */

static int on_power_event(const event_t *event, void *user_data)
{
    (void)user_data;
    if (event->type == EVENT_SYSTEM_POWER_SLEEP)
        power_manager_enter_sleep();
    else if (event->type == EVENT_SYSTEM_POWER_WAKEUP)
        power_manager_exit_sleep();
    return 0;
}

/* ---------- Public API ---------- */

int power_manager_init(void)
{
    g_lcd_device = rt_device_find("lcd");
    if (!g_lcd_device)
        rt_kprintf("[PWR] Warning: LCD device not found\n");

    event_bus_subscribe(EVENT_SYSTEM_POWER_SLEEP,
                        on_power_event, RT_NULL, EVENT_PRIORITY_HIGH);
    event_bus_subscribe(EVENT_SYSTEM_POWER_WAKEUP,
                        on_power_event, RT_NULL, EVENT_PRIORITY_HIGH);

    g_power_state = POWER_STATE_NORMAL;
    rt_kprintf("[PWR] Power manager initialized\n");
    return 0;
}

int power_manager_enter_sleep(void)
{
    if (g_power_state == POWER_STATE_SLEEP)
        return 0;

    rt_kprintf("[PWR] Entering sleep mode...\n");

    /* Must set SLEEP first so main loop stops calling lv_timer_handler(),
     * otherwise GPU may be accessed after POWEROFF -> assert crash. */
    g_power_state = POWER_STATE_SLEEP;
    rt_thread_mdelay(100);  /* Let any in-progress LVGL flush finish */

    g_sleep_ctx.saved_group = screen_get_current_group();
    g_sleep_ctx.saved_led_brightness = led_effects_get_global_brightness();

    led_effects_stop_all_effects();
    led_effects_turn_off_all_leds();

    if (g_lcd_device)
        rt_device_control(g_lcd_device, RTGRAPHIC_CTRL_POWEROFF, RT_NULL);

    backlight_off();
    sht30_controller_stop_continuous();

    rt_kprintf("[PWR] Sleep mode entered\n");
    return 0;
}

int power_manager_exit_sleep(void)
{
    if (g_power_state == POWER_STATE_NORMAL)
        return 0;

    rt_kprintf("[PWR] Exiting sleep mode...\n");

    /* Restore state early so incoming serial commands are not dropped. */
    g_power_state = POWER_STATE_NORMAL;

    if (g_lcd_device) {
        rt_device_control(g_lcd_device, RTGRAPHIC_CTRL_POWERON, RT_NULL);
        rt_thread_mdelay(300);  /* Wait for GC9107 init sequence + LCDC ready */
    }

    led_effects_set_global_brightness(g_sleep_ctx.saved_led_brightness);
    sht30_controller_start_continuous(5000);

    /* Invalidate active screen to trigger a full LVGL redraw.
     * The first flush will make drv_lcd auto-call DisplayOn(0x29). */
    lv_obj_invalidate(lv_scr_act());

    rt_thread_mdelay(200);  /* Wait for first frame flush before turning on backlight */
    backlight_on();

    rt_kprintf("[PWR] Normal mode restored\n");
    return 0;
}

power_state_t power_manager_get_state(void)
{
    return g_power_state;
}