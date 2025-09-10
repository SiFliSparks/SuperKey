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
#include "serial_data_handler.h"  // 添加头文件包含
#include <board.h>
#include <stdlib.h>

int main(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t ms;

    rt_kprintf("[1/10] 初始化显示系统...\n");  // 更新计数
    ret = littlevgl2rtt_init("lcd");
    if (ret != RT_EOK) {
        rt_kprintf("LVGL初始化失败: %d\n", ret);
        event_bus_deinit();
        return ret;
    }

    lv_ex_data_pool_init();
    
    rt_kprintf("[2/10] 初始化LED控制器...\n");
    if (led_controller_init() != 0) {
        rt_kprintf("LED控制器初始化失败\n");
    } else {
        led_controller_light_led(2, 0xFFFFFF, 800);
        rt_thread_mdelay(300);
        led_controller_light_led(1, 0xFFFFFF, 800);
        rt_thread_mdelay(300);
        led_controller_light_led(0, 0xFFFFFF, 800);
        rt_thread_mdelay(50);
    }
    
    rt_kprintf("[3/10] 初始化事件总线...\n");
    if (event_bus_init() != 0) {
        rt_kprintf("事件总线初始化失败\n");
        return -1;
    }
    
    rt_kprintf("[4/10] 初始化数据管理器...\n");
    if (data_manager_init() != 0) {
        rt_kprintf("数据管理器初始化失败\n");
        event_bus_deinit();
        return -1;
    }
    rt_kprintf("数据管理器初始化成功\n");
    
    rt_kprintf("[5/10] 初始化串口数据处理器...\n");
    // 关键修复：添加串口数据处理器初始化
    if (serial_data_handler_init() != RT_EOK) {
        rt_kprintf("串口数据处理器初始化失败\n");
        data_manager_deinit();
        event_bus_deinit();
        return -1;
    }
    rt_kprintf("串口数据处理器初始化成功 (uart1, 1000000 baud)\n");
    
    rt_kprintf("[6/10] 初始化HID系统...\n");
    if (app_controller_init() != 0) {
        rt_kprintf("HID系统初始化失败\n");
        serial_data_handler_deinit();
        data_manager_deinit();
        event_bus_deinit();
        return -1;
    }
    
    rt_kprintf("[7/10] 初始化SHT30传感器...\n");
    if (sht30_controller_init() == RT_EOK) {
        sht30_report_config_t config = {
            .enabled = false,
            .interval_ms = 5000,
            .format = SHT30_FORMAT_SI,
            .include_derived = true
        };
        sht30_controller_config_report(&config);
        sht30_controller_start_continuous(5000);
        rt_kprintf("SHT30传感器初始化成功\n");
    } else {
        rt_kprintf("SHT30传感器初始化失败\n");
    }

    rt_kprintf("[8/10] 创建三联屏显示系统...\n");
    create_triple_screen_display();
    rt_thread_mdelay(500);
    
    rt_kprintf("[9/10] 系统启动完成，进入主循环...\n");
    rt_kprintf("[10/10] 串口数据格式: sys_set <key> <value>\n");
    rt_kprintf("        支持的key: time, date, temp, weather_code, humidity, pressure, city_code\n");
    rt_kprintf("                   stock_name, stock_price, stock_change\n");
    rt_kprintf("                   cpu, cpu_temp, mem, gpu, gpu_temp, net_up, net_down\n");

    static rt_tick_t last_stats_report = 0;
    static rt_tick_t last_memory_check = 0;
    static rt_tick_t last_hid_check = 0;
    static uint32_t memory_warning_count = 0;

    while (1) {
        uint32_t ms = lv_timer_handler();
        rt_tick_t now = rt_tick_get();
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
            
            // 添加数据状态报告
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
    
    // 清理资源 - 添加串口数据处理器清理
    cleanup_triple_screen_display();
    app_controller_deinit();
    led_controller_deinit();
    sht30_controller_deinit();
    serial_data_handler_deinit();  // 添加清理调用
    data_manager_deinit();
    event_bus_deinit();
    
    return RT_EOK;
}