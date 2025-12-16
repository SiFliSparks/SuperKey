#include "rtthread.h"
#include "lvgl.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "lv_ex_data.h"
#include "screen.h"
#include "littlevgl2rtt.h"
#include "event_bus.h"
#include "app_controller.h"
#include "hid_device.h"
#include "encoder_controller.h"
#include "key_manager.h"
#include "sht30_controller.h"
#include "data_manager.h" 
#include "serial_data_handler.h"
#include "drv_rgbled.h"
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include "led_effects_manager.h"
#include "screen_context.h"
#include "widget_storage.h"
#include "widget_manager.h"
/* 系统线程优先级定义 */
#define MAIN_THREAD_PRIORITY        20  // 主线程优先级最低
#define EVENT_BUS_THREAD_PRIORITY   8   // 事件总线高优先级
#define LED_THREAD_PRIORITY         12  // LED效果线程
#define KEY_THREAD_PRIORITY         10  // 按键处理线程
#define SERIAL_THREAD_PRIORITY      15  // 串口数据处理
#define SCREEN_THREAD_PRIORITY      18  // 屏幕更新线程

/* 系统状态结构体 */
typedef struct {
    bool system_ready;
    bool in_error_state;
    uint32_t error_count;
    rt_tick_t last_health_check;
    rt_mutex_t system_lock;
} system_state_t;

static system_state_t g_system_state = {0};

/* 前向声明 */
static int system_init_stage(int stage, const char *stage_name, int (*init_func)(void));
static void system_error_recovery(void);
static int safe_component_init(const char *name, int (*init_func)(void));
static void system_show_startup_progress(int stage, int total_stages, const char *message);

/* 初始化函数包装器 */
static int init_display_system(void) { return littlevgl2rtt_init("lcd"); }
static int init_data_pool(void) { lv_ex_data_pool_init(); return 0; }
static int init_sht30_sensor(void) {
    if (sht30_controller_init() == RT_EOK) {
        sht30_report_config_t config = {
            .enabled = false,
            .interval_ms = 4000,
            .format = SHT30_FORMAT_SI,
            .include_derived = true
        };
        sht30_controller_config_report(&config);
        sht30_controller_start_continuous(5000);
        return 0;
    }
    return -1;
}
static int init_widget_system(void)
{
    return widget_manager_init(); 
}
static int init_screen_system(void) { 
    create_triple_screen_display(); 
    return 0; 
}

/* 安全的组件初始化包装器 */
static int safe_component_init(const char *name, int (*init_func)(void))
{
    if (!init_func) {
        return -RT_ERROR;
    }
    
    int result = init_func();
    
    if (result == 0 || result == RT_EOK) {
        return 0;
    } else {
        return result;
    }
}

/* 显示启动进度 */
static void system_show_startup_progress(int stage, int total_stages, const char *message)
{
    int progress = (stage * 100) / total_stages;
    rt_kprintf("[%d/%d] (%d%%) %s\n", stage, total_stages, progress, message);
}

/* 系统初始化阶段管理 */
static int system_init_stage(int stage, const char *stage_name, int (*init_func)(void))
{
    system_show_startup_progress(stage, 10, stage_name);
    
    int result = safe_component_init(stage_name, init_func);
    
    if (result != 0) {
        g_system_state.in_error_state = true;
        g_system_state.error_count++;
        return result;
    }
    

    rt_thread_mdelay(1);
    return 0;
}


/* 系统错误恢复 */
static void system_error_recovery(void)
{
    

    led_effects_stop_all_effects();
    led_effects_turn_off_all_leds();
    

    data_manager_cleanup_expired_data();
    data_manager_reset_all_data();
    event_bus_cleanup();
    

    screen_switch_group(SCREEN_GROUP_1);
    

    for (int i = 0; i < 3; i++) {
        led_effects_set_all_leds(RGB_COLOR_RED);
        rt_thread_mdelay(200);
        led_effects_set_all_leds(RGB_COLOR_BLACK);
        rt_thread_mdelay(200);
    }
}

static void system_graceful_shutdown(void)
{
    

    screen_context_cleanup_background_breathing();
    led_effects_stop_all_effects();
    led_effects_turn_off_all_leds();
    

    cleanup_triple_screen_display();
    app_controller_deinit();
    sht30_controller_deinit();
    serial_data_handler_deinit();
    data_manager_deinit();
    widget_manager_deinit();
    led_effects_manager_deinit();
    event_bus_deinit();
    

    if (g_system_state.system_lock) {
        rt_mutex_delete(g_system_state.system_lock);
        g_system_state.system_lock = RT_NULL;
    }
    
}

/* 主函数重新设计 */
int main(void)
{
    rt_err_t ret = RT_EOK;

    memset(&g_system_state, 0, sizeof(g_system_state));
    g_system_state.system_lock = rt_mutex_create("sys_lock", RT_IPC_FLAG_PRIO);
    

    ret = system_init_stage(1, "Display System", init_display_system);
    
    ret = system_init_stage(2, "Data Pool", init_data_pool);
    
    ret = system_init_stage(3, "Event Bus", event_bus_init);
    
    ret = system_init_stage(4, "HID & App Controller", app_controller_init);

    ret = system_init_stage(5, "LED Effects Manager (Init)",  led_effects_manager_init);
    
    ret = system_init_stage(6, "Data Manager", data_manager_init);
    
    ret = system_init_stage(7, "Serial Data Handler", serial_data_handler_init);

    ret = system_init_stage(8, "LED Effects Manager (Start)",  led_effects_manager_start); 


    ret = system_init_stage(9, "SHT30 Sensor", init_sht30_sensor);
    

    ret = system_init_stage(10, "Widget System", init_widget_system);

    ret = system_init_stage(11, "Screen System", init_screen_system);

    g_system_state.system_ready = true;
    g_system_state.last_health_check = rt_tick_get();

    while (g_system_state.system_ready) {

        uint32_t ms = lv_timer_handler();
        screen_process_switch_request();
        screen_context_process_background_restore();
        uint32_t sleep_time = (ms > 0 && ms < 100) ? ms : 50;
        rt_thread_mdelay(sleep_time);
    }
    

    system_graceful_shutdown();
    return 0;
}

/* 导出给其他模块的系统状态查询函数 */
bool system_is_ready(void)
{
    return g_system_state.system_ready && !g_system_state.in_error_state;
}

bool system_is_in_error_state(void)
{
    return g_system_state.in_error_state;
}

uint32_t system_get_error_count(void)
{
    return g_system_state.error_count;
}