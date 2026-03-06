#include "rtthread.h"
#include "lvgl.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "lv_ex_data.h"
#include "screen/screen.h"
#include "littlevgl2rtt.h"
#include "middleware/event_bus.h"
#include "middleware/app_controller.h"
#include "device/hid_device.h"
#include "device/encoder_controller.h"
#include "manager/key_manager.h"
#include "device/sht30_controller.h"
#include "manager/data_manager.h" 
#include "device/serial_data_handler.h"
#include "drv_rgbled.h"
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include "manager/led_effects_manager.h"
#include "screen/screen_context.h"
#include "widget/widget_storage.h"
#include "widget/widget_manager.h"
#include "mp3/mp3_player_controller.h"
#include "device/sdcard_monitor.h"
#include "manager/power_manager.h"          /* 低功耗管理 */

static bool g_system_ready = false;

int main(void)
{
    /* 显示系统初始化 */
    littlevgl2rtt_init("lcd");
    lv_ex_data_pool_init();
    
    /* 核心模块初始化 */
    event_bus_init();
    app_controller_init();
    led_effects_manager_init();
    data_manager_init();
    serial_data_handler_init();
    led_effects_manager_start();
    
    /* 电源管理初始化 (需要在 event_bus 和 lcd 之后) */
    power_manager_init();
    
    /* 传感器初始化 */
    sht30_controller_init();
    sht30_controller_start_continuous(5000);
    
    /* MP3和SD卡初始化 */
    mp3_player_init();
    sdcard_monitor_init();
    
    /* 界面初始化 */
    widget_manager_init();
    create_triple_screen_display();

    g_system_ready = true;

    /* 主动请求上位机下发数据（固件启动时间不确定，由固件决定何时准备好） */
    rt_thread_mdelay(500);  /* 等待 USB CDC 枚举完成 */
    serial_send_response("REQ:weather");
    serial_send_response("REQ:time");

    /* 主循环 */
    while (g_system_ready) {
        if (power_manager_get_state() == POWER_STATE_NORMAL) {
            /* 正常模式: LVGL刷新 + 屏幕状态处理 */
            uint32_t ms = lv_timer_handler();
            screen_process_switch_request();
            screen_context_process_background_restore();
            uint32_t sleep_time = (ms > 0 && ms < 100) ? ms : 50;
            rt_thread_mdelay(sleep_time);
        } else {
            /* 休眠模式: 仍需处理消息队列（唤醒/旋转命令），但不刷LVGL */
            screen_process_switch_request();
            rt_thread_mdelay(200);
        }
    }

    /* 关闭清理 */
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

    return 0;
}

bool system_is_ready(void)
{
    return g_system_ready;
}