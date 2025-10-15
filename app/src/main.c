// main.c - 修复编译错误版本

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
#include <string.h>  // 添加缺少的头文件
#include "led_effects_manager.h"
#include "screen_context.h"
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
static void system_health_monitor(void);
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
            .interval_ms = 5000,
            .format = SHT30_FORMAT_SI,
            .include_derived = true
        };
        sht30_controller_config_report(&config);
        sht30_controller_start_continuous(5000);
        return 0;
    }
    return -1;
}
static int init_screen_system(void) { 
    create_triple_screen_display(); 
    rt_thread_mdelay(500);
    return 0; 
}

/* 安全的组件初始化包装器 */
static int safe_component_init(const char *name, int (*init_func)(void))
{
    if (!init_func) {
        rt_kprintf("[INIT] ERROR: NULL init function for %s\n", name);
        return -RT_ERROR;
    }
    
    rt_kprintf("[INIT] Initializing %s...\n", name);
    
    int result = init_func();
    
    if (result == 0 || result == RT_EOK) {
        rt_kprintf("[INIT] %s initialized successfully\n", name);
        return 0;
    } else {
        rt_kprintf("[INIT] ERROR: %s initialization failed with code %d\n", name, result);
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
        rt_kprintf("[INIT] CRITICAL: Stage %d (%s) failed, aborting startup\n", 
                  stage, stage_name);
        g_system_state.in_error_state = true;
        g_system_state.error_count++;
        return result;
    }
    
    // 每个阶段完成后等待一小段时间，确保初始化稳定
    rt_thread_mdelay(100);
    return 0;
}

/* 系统健康监控 */
static void system_health_monitor(void)
{
    static uint32_t last_published = 0, last_processed = 0;
    rt_tick_t now = rt_tick_get();
    
    // 每30秒检查一次系统健康状态
    if ((now - g_system_state.last_health_check) < rt_tick_from_millisecond(30000)) {
        return;
    }
    
    g_system_state.last_health_check = now;
    
    // 检查事件总线状态
    uint32_t published, processed, dropped, queue_size;
    if (event_bus_get_stats(&published, &processed, &dropped, &queue_size) == 0) {
        
        // 检查事件处理是否停滞
        if (published == last_published && processed == last_processed && published > 0) {
            rt_kprintf("[HEALTH] WARNING: Event processing stalled\n");
            event_bus_cleanup();
        }
        
        // 检查丢弃率
        if (published > 0 && (dropped * 100 / published) > 10) {
            rt_kprintf("[HEALTH] WARNING: High event drop rate (%d%%)\n", 
                      dropped * 100 / published);
            event_bus_cleanup();
        }
        
        last_published = published;
        last_processed = processed;
    }
    
    // 检查内存使用情况
    rt_size_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    float usage_percent = (float)used * 100 / total;
    
    if (usage_percent > 85) {
        rt_kprintf("[HEALTH] WARNING: High memory usage (%.1f%%)\n", usage_percent);
        
        // 触发数据清理
        data_manager_cleanup_expired_data();
        event_bus_cleanup();
        
        // 如果内存使用仍然很高，进入错误恢复模式
        rt_memory_info(&total, &used, &max_used);
        usage_percent = (float)used * 100 / total;
        if (usage_percent > 90) {
            rt_kprintf("[HEALTH] CRITICAL: Memory usage critical, entering recovery mode\n");
            system_error_recovery();
        }
    }
    
    // 检查HID设备状态
    if (hid_device_ready()) {
        int sem_count = hid_get_semaphore_count();
        if (sem_count > 1) {
            rt_kprintf("[HEALTH] WARNING: HID semaphore anomaly (%d), fixing\n", sem_count);
            hid_reset_semaphore();
        }
    }
}

/* 系统错误恢复 */
static void system_error_recovery(void)
{
    rt_kprintf("[RECOVERY] Starting system recovery procedures...\n");
    
    // 停止所有LED效果
    led_effects_stop_all_effects();
    led_effects_turn_off_all_leds();
    
    // 清理数据和事件
    data_manager_cleanup_expired_data();
    data_manager_reset_all_data();
    event_bus_cleanup();
    
    // 重置屏幕到默认组 - 使用正确的函数名
    screen_switch_group(SCREEN_GROUP_1);
    
    // 闪烁红色LED表示恢复模式
    for (int i = 0; i < 3; i++) {
        led_effects_set_all_leds(RGB_COLOR_RED);
        rt_thread_mdelay(200);
        led_effects_set_all_leds(RGB_COLOR_BLACK);
        rt_thread_mdelay(200);
    }
    
    rt_kprintf("[RECOVERY] Recovery procedures completed\n");
}

/* 优雅关闭函数 */
static void system_graceful_shutdown(void)
{
    rt_kprintf("[SHUTDOWN] Starting graceful shutdown...\n");
    
    // 1. 停止所有效果和指示灯
    screen_context_cleanup_background_breathing();
    led_effects_stop_all_effects();
    led_effects_turn_off_all_leds();
    
    // 2. 清理各个组件（按初始化的逆序）
    cleanup_triple_screen_display();
    app_controller_deinit();
    sht30_controller_deinit();
    serial_data_handler_deinit();
    data_manager_deinit();
    led_effects_manager_deinit();
    event_bus_deinit();
    
    // 3. 清理系统状态
    if (g_system_state.system_lock) {
        rt_mutex_delete(g_system_state.system_lock);
        g_system_state.system_lock = RT_NULL;
    }
    
    rt_kprintf("[SHUTDOWN] Graceful shutdown completed\n");
}

/* 主函数重新设计 */
int main(void)
{
    rt_err_t ret = RT_EOK;
    HAL_PIN_Set(PAD_PA07, GPIO_A7, PIN_NOPULL, 1);//初始化LDO
    BSP_GPIO_Set(7, 1, 1);
    // 初始化系统状态
    memset(&g_system_state, 0, sizeof(g_system_state));
    g_system_state.system_lock = rt_mutex_create("sys_lock", RT_IPC_FLAG_PRIO);
    if (!g_system_state.system_lock) {
        rt_kprintf("[MAIN] CRITICAL: Failed to create system lock\n");
        return -RT_ENOMEM;
    }
    
    rt_kprintf("========================================\n");
    rt_kprintf("  SiFli Smart Display System Starting  \n");
    rt_kprintf("========================================\n");
    
    // 阶段1: 显示系统初始化
    ret = system_init_stage(1, "Display System", init_display_system);
    if (ret != 0) goto error_exit;
    
    // 阶段2: 数据池初始化
    ret = system_init_stage(2, "Data Pool", init_data_pool);
    if (ret != 0) goto error_exit;
    
    // 阶段3: 事件总线初始化 (高优先级)
    ret = system_init_stage(3, "Event Bus", event_bus_init);
    if (ret != 0) goto error_exit;
    
    // 阶段4: LED效果管理器初始化 (重新设计版本)
    ret = system_init_stage(4, "LED Effects Manager", led_effects_manager_init);
    if (ret != 0) {
        rt_kprintf("[INIT] WARNING: LED effects failed, continuing without LED support\n");
        // LED失败不是致命的，继续启动
    }
    
    // 阶段5: 数据管理器初始化
    ret = system_init_stage(5, "Data Manager", data_manager_init);
    if (ret != 0) goto error_exit;
    
    // 阶段6: 串口数据处理器初始化
    ret = system_init_stage(6, "Serial Data Handler", serial_data_handler_init);
    if (ret != 0) {
        rt_kprintf("[INIT] WARNING: Serial handler failed, continuing without serial support\n");
        // 串口失败不是致命的
    }
    
    // 阶段7: HID和应用控制器初始化
    ret = system_init_stage(7, "HID & App Controller", app_controller_init);
    if (ret != 0) {
        rt_kprintf("[INIT] WARNING: HID system failed, continuing without HID support\n");
        // HID失败不是致命的
    }
    
    // 阶段8: 传感器初始化 (可选)
    ret = system_init_stage(8, "SHT30 Sensor", init_sht30_sensor);
    if (ret != 0) {
        rt_kprintf("[INIT] WARNING: SHT30 sensor failed, continuing without sensor\n");
    }
    
    // 阶段9: 屏幕系统初始化
    ret = system_init_stage(9, "Screen System", init_screen_system);
    if (ret != 0) goto error_exit;
    
    // 阶段10: 启动LED欢迎效果
    system_show_startup_progress(10, 10, "Startup Effects & System Ready");
    
    // 启动LED欢迎序列 (如果LED管理器可用)
    rt_kprintf("=== 启动LED欢迎序列 ===\n");
    
    // 第一阶段：青色流水灯
    led_effect_handle_t flowing = led_effects_flowing(0xFFCCFF, 1000, 255, 2000);
    rt_thread_mdelay(2000);
    
    // 第二阶段：蓝色呼吸灯 (持续运行)
    led_effect_handle_t breathing = led_effects_breathing(RGB_COLOR_BLUE, 2000, 255, 0);
    
    rt_kprintf("=== 系统启动完成 ===\n");
    rt_kprintf("串口数据格式: sys_set <key> <value>\n");
    rt_kprintf("支持的键值:\n");
    rt_kprintf("  时间相关: time, date\n"); 
    rt_kprintf("  天气相关: temp, weather_code, humidity, pressure, city_code\n");
    rt_kprintf("  股票相关: stock_name, stock_price, stock_change\n");
    rt_kprintf("  系统相关: cpu, cpu_temp, mem, gpu, gpu_temp, net_up, net_down\n");
    
    g_system_state.system_ready = true;
    g_system_state.last_health_check = rt_tick_get();
    
    // 主循环 - 重新设计
    rt_kprintf("[MAIN] Entering main loop with health monitoring\n");
    
    while (g_system_state.system_ready) {
        // 1. 处理LVGL定时器
        uint32_t ms = lv_timer_handler();
        
        // 2. 处理屏幕切换请求
        screen_process_switch_request();
        screen_context_process_background_restore();
        // 3. 系统健康监控
        if (!g_system_state.in_error_state) {
            system_health_monitor();
        }
        
        // 4. 错误状态检查
        if (g_system_state.in_error_state && g_system_state.error_count > 5) {
            rt_kprintf("[MAIN] CRITICAL: Too many errors, initiating shutdown\n");
            break;
        }
        
        // 5. 控制主循环频率
        uint32_t sleep_time = (ms > 0 && ms < 100) ? ms : 50;
        rt_thread_mdelay(sleep_time);
    }
    
    // 正常退出清理
    system_graceful_shutdown();
    return 0;
    
error_exit:
    // 错误退出清理
    rt_kprintf("[MAIN] CRITICAL ERROR during initialization, performing emergency cleanup\n");
    g_system_state.in_error_state = true;
    // 清理背景呼吸灯系统（新增）
    screen_context_cleanup_background_breathing();    
    // 尝试显示错误LED指示
    for (int i = 0; i < 5; i++) {
        led_effects_set_all_leds(RGB_COLOR_RED);
        rt_thread_mdelay(100);
        led_effects_set_all_leds(RGB_COLOR_BLACK);
        rt_thread_mdelay(100);
    }
    
    system_graceful_shutdown();
    return ret;
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