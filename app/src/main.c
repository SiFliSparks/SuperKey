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
#include "encoder_context.h"
#include "led_controller.h"
#include "key_manager.h"
#include "sht30_controller.h"
#include "data_manager.h" 
#include "serial_data_handler.h"
#include "light_effects.h"
#include <board.h>
#include <stdlib.h>

int main(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t ms;

    static rt_tick_t last_stats_report = 0;
    static rt_tick_t last_memory_check = 0;
    static rt_tick_t last_hid_check = 0;
    static uint32_t memory_warning_count = 0;
    
    rt_kprintf("[1/10] Initializing display system...\n");
    ret = littlevgl2rtt_init("lcd");
    if (ret != RT_EOK) {
        rt_kprintf("LVGL init failed: %d\n", ret);
        event_bus_deinit();
        return ret;
    }

    lv_ex_data_pool_init();
    
    rt_kprintf("[2/10] Initializing LED controller...\n");
    if (led_controller_init() != 0) {
        rt_kprintf("LED controller init failed\n");
    } else {
        rt_kprintf("LED controller init success\n");
        
        // 初始化灯光效果模块
        if (light_effects_init() != 0) {
            rt_kprintf("Light effects module init failed\n");
        } else {
            rt_kprintf("Light effects module init success\n");
        }
    }
    
    rt_kprintf("[3/10] Initializing event bus...\n");
    if (event_bus_init() != 0) {
        rt_kprintf("Event bus init failed\n");
        return -1;
    }
    
    rt_kprintf("[4/10] Initializing data manager...\n");
    if (data_manager_init() != 0) {
        rt_kprintf("Data manager init failed\n");
        event_bus_deinit();
        return -1;
    }
    rt_kprintf("Data manager init success\n");
    
    rt_kprintf("[5/10] Initializing serial data handler...\n");
    if (serial_data_handler_init() != RT_EOK) {
        rt_kprintf("Serial data handler init failed\n");
        data_manager_deinit();
        event_bus_deinit();
        return -1;
    }
    rt_kprintf("Serial data handler init success (uart1, 1000000 baud)\n");
    
    rt_kprintf("[6/10] Initializing HID system...\n");
    if (app_controller_init() != 0) {
        rt_kprintf("HID system init failed\n");
        serial_data_handler_deinit();
        data_manager_deinit();
        event_bus_deinit();
        return -1;
    }
    
    rt_kprintf("[7/10] Initializing SHT30 sensor...\n");
    if (sht30_controller_init() == RT_EOK) {
        sht30_report_config_t config = {
            .enabled = false,
            .interval_ms = 5000,
            .format = SHT30_FORMAT_SI,
            .include_derived = true
        };
        sht30_controller_config_report(&config);
        sht30_controller_start_continuous(5000);
        rt_kprintf("SHT30 sensor init success\n");
    } else {
        rt_kprintf("SHT30 sensor init failed\n");
    }

    rt_kprintf("[8/10] Creating triple screen display...\n");
    create_triple_screen_display();
    rt_thread_mdelay(500);
    
    rt_kprintf("[9/10] System startup complete, entering main loop...\n");
    rt_kprintf("[10/10] Serial data format: sys_set <key> <value>\n");
    rt_kprintf("        Supported keys: time, date, temp, weather_code, humidity, pressure, city_code\n");
    rt_kprintf("                       stock_name, stock_price, stock_change\n");
    rt_kprintf("                       cpu, cpu_temp, mem, gpu, gpu_temp, net_up, net_down\n");
    
    // === 选择你想要的呼吸灯效果 ===
    
    // 选项1：青色呼吸灯开机动画（执行1次，2秒周期）- 推荐
    //light_effects_breathing_once(LIGHT_COLOR_CYAN, 2000, 200);
    
    // 选项2：蓝色呼吸灯（执行3次）
     light_effects_breathing(LIGHT_COLOR_BLUE, 2000, 200, 1);
    
    // 选项3：白色慢速呼吸（执行1次，3秒周期）
    // light_effects_breathing_once(LIGHT_COLOR_WHITE, 3000, 180);
    
    // 选项4：绿色快速呼吸（执行1次，1.5秒周期）
    // light_effects_breathing_once(LIGHT_COLOR_GREEN, 1500, 220);
    
    // 选项5：红色警告呼吸（执行1次，1秒周期）
    // light_effects_breathing_once(LIGHT_COLOR_RED, 1000, 255);

    while (1) {
        uint32_t ms = lv_timer_handler();
        rt_tick_t now = rt_tick_get();
        
        // 更新灯光效果
        light_effects_update();
        
        screen_process_switch_request();        
        
        if ((now - last_stats_report) > rt_tick_from_millisecond(300000)) {
            uint32_t published, processed, dropped, queue_size;
            if (event_bus_get_stats(&published, &processed, &dropped, &queue_size) == 0) {
                rt_kprintf("[Main] EventBus Stats - Published: %u, Processed: %u, Dropped: %u, Queue: %u\n",
                          published, processed, dropped, queue_size);
                
                if (dropped > published / 10) {
                    rt_kprintf("[Main] High event drop rate detected, cleaning queue\n");
                    event_bus_cleanup();
                }
            }
            
            char data_status[256];
            if (data_manager_get_data_status(data_status, sizeof(data_status)) == 0) {
                rt_kprintf("[Main] Data Status: %s\n", data_status);
            }
            
            last_stats_report = now;
        }

        if ((now - last_memory_check) > rt_tick_from_millisecond(60000)) {
            rt_size_t total, used, max_used;
            rt_memory_info(&total, &used, &max_used);
            
            float usage_percent = (float)used * 100 / total;
            
            if (usage_percent > 90) {
                memory_warning_count++;
                rt_kprintf("[Main] CRITICAL MEMORY: %d/%d bytes (%.1f%%) - Warning #%u\n", 
                          used, total, usage_percent, memory_warning_count);
                
                data_manager_cleanup_expired_data();
                event_bus_cleanup();
            }
            
            last_memory_check = now;
        }

        if ((now - last_hid_check) > rt_tick_from_millisecond(20000)) {
            if (hid_device_ready()) {
                int sem_count = hid_get_semaphore_count();
                if (sem_count > 1) {
                    rt_kprintf("[Main] HID semaphore anomaly detected (%d), auto-fixing\n", sem_count);
                    hid_reset_semaphore();
                }
            }
            last_hid_check = now;
        }
        
        screen_process_switch_request();
        rt_thread_mdelay(ms);
    }
    
    // 清理资源
    cleanup_triple_screen_display();
    app_controller_deinit();
    light_effects_deinit();
    led_controller_deinit();
    sht30_controller_deinit();
    serial_data_handler_deinit();
    data_manager_deinit();
    event_bus_deinit();
    
    return RT_EOK;
}