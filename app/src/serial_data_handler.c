#include <rtthread.h>
#include <rtdevice.h>
#include "screen.h"
#include <string.h>
#include <stdlib.h>
#include "data_manager.h"
#include <time.h>
#include <stdio.h>
#include "hid_device.h"
#include "event_bus.h"

#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_DEVICE_NAME "uart1"
#define SERIAL_TIMEOUT_MS (30000)
#define WATCHDOG_CHECK_INTERVAL_MS (10000)

static rt_device_t serial_device = RT_NULL;
static rt_sem_t rx_sem = RT_NULL;
static rt_timer_t watchdog_timer = RT_NULL;

static struct {
    rt_tick_t last_received_tick;
    bool connection_alive;
    uint32_t total_commands_received;
    uint32_t invalid_commands_count;
    uint32_t timeout_count;
} g_serial_status = {0};

static const char* weather_code_map[] = {
    "晴",
    "多云",
    "阴",
    "小雨",
    "中雨",
    "大雨",
    "雷阵雨",
    "小雪",
    "中雪",
    "大雪",
    "雨夹雪",
    "雾",
    "霾",
    "沙尘",
    "晴转多云",
    "多云转阴",
    "阴转雨"
};
#define WEATHER_CODE_MAX 17

static const char* city_code_map[] = {
    "杭州",
    "上海",
    "北京",
    "广州",
    "深圳",
    "成都",
    "重庆",
    "武汉",
    "西安",
    "南京",
    "天津",
    "苏州",
    "青岛",
    "厦门",
    "长沙"
};
#define CITY_CODE_MAX 15

static struct {
    char time_str[16];
    char date_str[16];
    char weekday_str[16];
    bool time_valid;
    
    int temperature;
    int weather_code;
    int humidity;
    int pressure;
    int city_code;
    bool weather_valid;
    
    char stock_name[64];
    float stock_price;
    float stock_change;
    bool stock_valid;
    
    float cpu_usage;
    float cpu_temp;
    float mem_usage;
    float gpu_usage;
    float gpu_temp;
    float net_up;
    float net_down;
    bool system_valid;
} g_finsh_data = {0};

static void update_connection_status(void)
{
    g_serial_status.last_received_tick = rt_tick_get();
    if (!g_serial_status.connection_alive) {
        g_serial_status.connection_alive = true;
        rt_kprintf("[Finsh] Serial connection established\n");
    }
}

static void serial_watchdog_timer_cb(void *parameter)
{
    (void)parameter;
    
    rt_tick_t now = rt_tick_get();
    rt_tick_t timeout_ticks = rt_tick_from_millisecond(SERIAL_TIMEOUT_MS);
    
    if ((now - g_serial_status.last_received_tick) > timeout_ticks) {
        if (g_serial_status.connection_alive) {
            g_serial_status.connection_alive = false;
            g_serial_status.timeout_count++;
            
            rt_kprintf("[Finsh] Serial connection timeout #%u, invalidating data\n", 
                      g_serial_status.timeout_count);
            
            g_finsh_data.time_valid = false;
            g_finsh_data.weather_valid = false;
            g_finsh_data.stock_valid = false;
            g_finsh_data.system_valid = false;
            
            data_manager_reset_all_data();
        }
    }
}

static void safe_strcpy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) return;
    
    size_t src_len = strlen(src);
    size_t copy_len = (src_len < dest_size - 1) ? src_len : (dest_size - 1);
    
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
}

static void remove_quotes(char *str)
{
    if (!str) return;
    
    int len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        str[len-1] = '\0';
        memmove(str, str+1, len-1);
    }
}

static void handle_time_data(const char *key, const char *value)
{
    if (strcmp(key, "time") == 0) {
        safe_strcpy(g_finsh_data.time_str, value, sizeof(g_finsh_data.time_str));
        g_finsh_data.time_valid = true;
        rt_kprintf("[Finsh] Time updated: %s\n", value);
        
        int hour, min, sec;
        if (sscanf(value, "%d:%d:%d", &hour, &min, &sec) == 3) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                tm_info->tm_hour = hour;
                tm_info->tm_min = min;
                tm_info->tm_sec = sec;
                time_t new_time = mktime(tm_info);
                
                rt_device_t rtc = rt_device_find("rtc");
                if (rtc) {
                    rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &new_time);
                    rt_kprintf("[Finsh] System time synchronized\n");
                }
            }
        }
    }
    else if (strcmp(key, "date") == 0) {
        safe_strcpy(g_finsh_data.date_str, value, sizeof(g_finsh_data.date_str));
        rt_kprintf("[Finsh] Date updated: %s\n", value);
        
        int year, month, day;
        if (sscanf(value, "%d-%d-%d", &year, &month, &day) == 3) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                tm_info->tm_year = year - 1900;
                tm_info->tm_mon = month - 1;
                tm_info->tm_mday = day;
                time_t new_time = mktime(tm_info);
                
                rt_device_t rtc = rt_device_find("rtc");
                if (rtc) {
                    rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &new_time);
                    rt_kprintf("[Finsh] System date synchronized\n");
                }
            }
        }
    }
    else if (strcmp(key, "weekday") == 0) {
        safe_strcpy(g_finsh_data.weekday_str, value, sizeof(g_finsh_data.weekday_str));
        rt_kprintf("[Finsh] Weekday updated: %s\n", value);
    }
}

static void handle_weather_data(const char *key, const char *value)
{
    if (strcmp(key, "temp") == 0) {
        g_finsh_data.temperature = atoi(value);
        g_finsh_data.weather_valid = true;
        rt_kprintf("[Finsh] Temperature updated: %d°C\n", g_finsh_data.temperature);
    }
    else if (strcmp(key, "weather_code") == 0) {
        g_finsh_data.weather_code = atoi(value);
        rt_kprintf("[Finsh] Weather code updated: %d\n", g_finsh_data.weather_code);
    }
    else if (strcmp(key, "humidity") == 0) {
        g_finsh_data.humidity = atoi(value);
        rt_kprintf("[Finsh] Humidity updated: %d%%\n", g_finsh_data.humidity);
    }
    else if (strcmp(key, "pressure") == 0) {
        g_finsh_data.pressure = atoi(value);
        rt_kprintf("[Finsh] Pressure updated: %dhPa\n", g_finsh_data.pressure);
    }
    else if (strcmp(key, "city_code") == 0) {
        g_finsh_data.city_code = atoi(value);
        rt_kprintf("[Finsh] City code updated: %d\n", g_finsh_data.city_code);
    }
    
    if (g_finsh_data.weather_valid) {
        weather_data_t weather = {0};
        
        weather.temperature = (float)g_finsh_data.temperature;
        weather.humidity = (float)g_finsh_data.humidity;
        weather.pressure = g_finsh_data.pressure;
        weather.valid = true;
        
        if (g_finsh_data.weather_code >= 0 && g_finsh_data.weather_code < WEATHER_CODE_MAX) {
            safe_strcpy(weather.weather, weather_code_map[g_finsh_data.weather_code], sizeof(weather.weather));
        } else {
            safe_strcpy(weather.weather, "晴", sizeof(weather.weather));
        }
        
        if (g_finsh_data.city_code >= 0 && g_finsh_data.city_code < CITY_CODE_MAX) {
            safe_strcpy(weather.city, city_code_map[g_finsh_data.city_code], sizeof(weather.city));
        } else {
            safe_strcpy(weather.city, "杭州", sizeof(weather.city));
        }
        
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                snprintf(weather.update_time, sizeof(weather.update_time),
                        "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
        }
        
        event_data_weather_t weather_event = { .weather = weather };
        event_bus_publish(EVENT_DATA_WEATHER_UPDATED, &weather_event, sizeof(weather_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
        
        rt_kprintf("[Finsh] Weather event published: %s %.0f°C %s\n", 
                  weather.city, weather.temperature, weather.weather);
    }
}

static void handle_stock_data(const char *key, const char *value)
{
    if (strcmp(key, "stock_name") == 0) {
        safe_strcpy(g_finsh_data.stock_name, value, sizeof(g_finsh_data.stock_name));
        g_finsh_data.stock_valid = true;
        rt_kprintf("[Finsh] Stock name updated: %s\n", value);
    }
    else if (strcmp(key, "stock_price") == 0) {
        g_finsh_data.stock_price = atof(value);
        rt_kprintf("[Finsh] Stock price updated: %.2f\n", g_finsh_data.stock_price);
    }
    else if (strcmp(key, "stock_change") == 0) {
        g_finsh_data.stock_change = atof(value);
        rt_kprintf("[Finsh] Stock change updated: %.2f\n", g_finsh_data.stock_change);
    }
    
    if (g_finsh_data.stock_valid) {
        stock_data_t stock = {0};
        
        safe_strcpy(stock.name, g_finsh_data.stock_name, sizeof(stock.name));
        safe_strcpy(stock.symbol, "000001", sizeof(stock.symbol));
        stock.current_price = g_finsh_data.stock_price;
        stock.change_value = g_finsh_data.stock_change;
        
        if (stock.current_price > 0) {
            float prev_price = stock.current_price - stock.change_value;
            if (prev_price > 0) {
                stock.change_percent = (stock.change_value / prev_price) * 100.0f;
            }
        }
        
        stock.valid = true;
        
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                snprintf(stock.update_time, sizeof(stock.update_time),
                        "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
        }
        
        event_data_stock_t stock_event = { .stock = stock };
        event_bus_publish(EVENT_DATA_STOCK_UPDATED, &stock_event, sizeof(stock_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
        
        rt_kprintf("[Finsh] Stock event published: %s %.2f %+.2f(%.2f%%)\n",
                  stock.name, stock.current_price, stock.change_value, stock.change_percent);
    }
}

static void handle_system_data(const char *key, const char *value)
{
    float val = atof(value);
    
    if (strcmp(key, "cpu") == 0) {
        g_finsh_data.cpu_usage = val;
        g_finsh_data.system_valid = true;
        rt_kprintf("[Finsh] CPU usage updated: %.1f%%\n", val);
    }
    else if (strcmp(key, "cpu_temp") == 0) {
        g_finsh_data.cpu_temp = val;
        rt_kprintf("[Finsh] CPU temp updated: %.1f°C\n", val);
    }
    else if (strcmp(key, "mem") == 0) {
        g_finsh_data.mem_usage = val;
        rt_kprintf("[Finsh] Memory usage updated: %.1f%%\n", val);
    }
    else if (strcmp(key, "gpu") == 0) {
        g_finsh_data.gpu_usage = val;
        rt_kprintf("[Finsh] GPU usage updated: %.1f%%\n", val);
    }
    else if (strcmp(key, "gpu_temp") == 0) {
        g_finsh_data.gpu_temp = val;
        rt_kprintf("[Finsh] GPU temp updated: %.1f°C\n", val);
    }
    else if (strcmp(key, "net_up") == 0) {
        g_finsh_data.net_up = val;
        rt_kprintf("[Finsh] Network upload updated: %.2f MB/s\n", val);
    }
    else if (strcmp(key, "net_down") == 0) {
        g_finsh_data.net_down = val;
        rt_kprintf("[Finsh] Network download updated: %.2f MB/s\n", val);
    }
    
    if (g_finsh_data.system_valid) {
        system_monitor_data_t sys_data = {0};
        
        sys_data.cpu_usage = g_finsh_data.cpu_usage;
        sys_data.cpu_temp = g_finsh_data.cpu_temp;
        sys_data.gpu_usage = g_finsh_data.gpu_usage;
        sys_data.gpu_temp = g_finsh_data.gpu_temp;
        sys_data.ram_usage = g_finsh_data.mem_usage;
        sys_data.net_upload_speed = g_finsh_data.net_up;
        sys_data.net_download_speed = g_finsh_data.net_down;
        sys_data.valid = true;
        
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                        "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
        }
        
        event_data_system_t system_event = { .system = sys_data };
        event_bus_publish(EVENT_DATA_SYSTEM_UPDATED, &system_event, sizeof(system_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
        
        rt_kprintf("[Finsh] System monitor event published\n");
    }
}

static void handle_finsh_key_value(const char *key, const char *value)
{
    if (!key || !value) return;
    
    if (strcmp(key, "time") == 0 || strcmp(key, "date") == 0 || strcmp(key, "weekday") == 0) {
        handle_time_data(key, value);
    }
    else if (strcmp(key, "temp") == 0 || strcmp(key, "weather_code") == 0 || 
             strcmp(key, "humidity") == 0 || strcmp(key, "pressure") == 0 || 
             strcmp(key, "city_code") == 0) {
        handle_weather_data(key, value);
    }
    else if (strcmp(key, "stock_name") == 0 || strcmp(key, "stock_price") == 0 || 
             strcmp(key, "stock_change") == 0) {
        handle_stock_data(key, value);
    }
    else if (strcmp(key, "cpu") == 0 || strcmp(key, "cpu_temp") == 0 || 
             strcmp(key, "mem") == 0 || strcmp(key, "gpu") == 0 || 
             strcmp(key, "gpu_temp") == 0 || strcmp(key, "net_up") == 0 || 
             strcmp(key, "net_down") == 0) {
        handle_system_data(key, value);
    }
    else if (strcmp(key, "test") == 0) {
        rt_kprintf("[Finsh] Test message received: %s\n", value);
    }
    else {
        rt_kprintf("[Finsh] Unknown key: %s = %s\n", key, value);
    }
}

static void process_finsh_command(const char *cmd_str)
{
    if (!cmd_str) return;
    
    size_t cmd_len = strlen(cmd_str);
    if (cmd_len > SERIAL_RX_BUFFER_SIZE - 1) {
        rt_kprintf("[Finsh] Command too long (%d bytes), ignored\n", cmd_len);
        g_serial_status.invalid_commands_count++;
        return;
    }
    
    char command[16] = {0};
    char key[32] = {0};
    char value[128] = {0};
    
    int parsed = sscanf(cmd_str, "%15s %31s %127[^\r\n]", command, key, value);
    
    if (parsed >= 3 && strcmp(command, "sys_set") == 0) {
        command[15] = '\0';
        key[31] = '\0';
        value[127] = '\0';
        
        remove_quotes(value);
        
        rt_kprintf("[Finsh] Command: %s %s = %.50s%s\n", 
                  command, key, value, (strlen(value) > 50) ? "..." : "");
        
        handle_finsh_key_value(key, value);
        
        g_serial_status.total_commands_received++;
        update_connection_status();
        
    } else {
        rt_kprintf("[Finsh] Invalid command format (parsed %d fields): %.100s\n", 
                  parsed, cmd_str);
        rt_kprintf("[Finsh] Expected format: sys_set <key> <value>\n");
        g_serial_status.invalid_commands_count++;
    }
}

static rt_err_t serial_rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(rx_sem);
    return RT_EOK;
}

static void serial_rx_thread_entry(void *parameter)
{
    (void)parameter;
    
    char ch;
    static char line_buffer[SERIAL_RX_BUFFER_SIZE];
    static int line_index = 0;
    static uint32_t consecutive_errors = 0;
    static rt_tick_t last_hid_check = 0;
    
    rt_kprintf("[Finsh] RX thread started with system monitoring\n");
    rt_kprintf("[Finsh] Expected format: sys_set <key> <value>\\n\\n");
    
    g_serial_status.last_received_tick = rt_tick_get();
    g_serial_status.connection_alive = false;
    
    while (1) {
        rt_size_t read_bytes = 0;
        
        while (read_bytes != 1) {
            read_bytes = rt_device_read(serial_device, -1, &ch, 1);
            
            if (read_bytes != 1) {
                rt_err_t sem_result = rt_sem_take(rx_sem, RT_WAITING_FOREVER);
                if (sem_result != RT_EOK) {
                    rt_kprintf("[Finsh] Semaphore error: %d\n", sem_result);
                    consecutive_errors++;
                    
                    if (consecutive_errors > 50) {
                        rt_kprintf("[Finsh] Too many consecutive errors, taking a break\n");
                        rt_thread_mdelay(1000);
                        consecutive_errors = 0;
                    }
                    continue;
                }
            }
        }
        
        consecutive_errors = 0;
        
        rt_tick_t now = rt_tick_get();
        if ((now - last_hid_check) > rt_tick_from_millisecond(30000)) {
            if (hid_device_ready()) {
                int sem_count = hid_get_semaphore_count();
                if (sem_count > 1) {
                    rt_kprintf("[Finsh] Detected HID anomaly during RX (%d), fixing\n", sem_count);
                    hid_reset_semaphore();
                }
            }
            last_hid_check = now;
        }
        
        if (ch == '\n' || ch == '\r') {
            if (line_index > 0) {
                line_buffer[line_index] = '\0';
                
                if (line_index < SERIAL_RX_BUFFER_SIZE - 1) {
                    rt_kprintf("[Finsh] Received line (%d bytes): %s\n", line_index, line_buffer);
                    
                    if (consecutive_errors > 10) {
                        rt_kprintf("[Finsh] Too many errors, throttling processing\n");
                        rt_thread_mdelay(100);
                        consecutive_errors = 0;
                    }
                    
                    if (strlen(line_buffer) > 0) {
                        process_finsh_command(line_buffer);
                    }
                    
                    consecutive_errors = 0;
                } else {
                    rt_kprintf("[Finsh] Line too long (%d bytes), ignored\n", line_index);
                    g_serial_status.invalid_commands_count++;
                    consecutive_errors++;
                }
                
                line_index = 0;
                memset(line_buffer, 0, sizeof(line_buffer));
            }
        } else if (ch >= 0x20 || ch == 0x09) {
            if (line_index < SERIAL_RX_BUFFER_SIZE - 2) {
                line_buffer[line_index++] = ch;
            } else {
                rt_kprintf("[Finsh] Line buffer full (%d bytes), resetting\n", line_index);
                line_index = 0;
                memset(line_buffer, 0, sizeof(line_buffer));
                g_serial_status.invalid_commands_count++;
                consecutive_errors++;
            }
        }
    }
}

int serial_data_handler_init(void)
{
    rt_thread_t thread;
    
    serial_device = rt_device_find(SERIAL_DEVICE_NAME);
    if (!serial_device) {
        rt_kprintf("[Finsh] Device %s not found\n", SERIAL_DEVICE_NAME);
        return -RT_ERROR;
    }
    
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_1000000;
    rt_device_control(serial_device, RT_DEVICE_CTRL_CONFIG, &config);
    
    rt_err_t ret = rt_device_open(serial_device, RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        rt_kprintf("[Finsh] Failed to open device: %d\n", ret);
        return ret;
    }
    
    rx_sem = rt_sem_create("finsh_rx_sem", 0, RT_IPC_FLAG_PRIO);
    if (!rx_sem) {
        rt_kprintf("[Finsh] Failed to create semaphore\n");
        rt_device_close(serial_device);
        return -RT_ENOMEM;
    }
    
    watchdog_timer = rt_timer_create("finsh_watchdog",
                                     serial_watchdog_timer_cb,
                                     RT_NULL,
                                     rt_tick_from_millisecond(WATCHDOG_CHECK_INTERVAL_MS),
                                     RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (!watchdog_timer) {
        rt_kprintf("[Finsh] Failed to create watchdog timer\n");
        rt_sem_delete(rx_sem);
        rt_device_close(serial_device);
        return -RT_ENOMEM;
    }
    
    rt_timer_start(watchdog_timer);
    
    rt_device_set_rx_indicate(serial_device, serial_rx_callback);
    
    thread = rt_thread_create("finsh_rx",
                              serial_rx_thread_entry,
                              RT_NULL,
                              4096,
                              10, 10);
    if (thread) {
        rt_thread_startup(thread);
        rt_kprintf("[Finsh] Data handler initialized with connection monitoring\n");
        rt_kprintf("[Finsh] Baud rate: 1000000, Buffer: %d bytes, Stack: 4KB\n", SERIAL_RX_BUFFER_SIZE);
        rt_kprintf("[Finsh] Connection timeout: %ds, Watchdog interval: %ds\n", 
                  SERIAL_TIMEOUT_MS/1000, WATCHDOG_CHECK_INTERVAL_MS/1000);
        return RT_EOK;
    }
    
    rt_timer_stop(watchdog_timer);
    rt_timer_delete(watchdog_timer);
    rt_sem_delete(rx_sem);
    rt_device_close(serial_device);
    return -RT_ERROR;
}

int serial_data_handler_deinit(void)
{
    if (watchdog_timer) {
        rt_timer_stop(watchdog_timer);
        rt_timer_delete(watchdog_timer);
        watchdog_timer = RT_NULL;
    }
    
    if (serial_device) {
        rt_device_close(serial_device);
        serial_device = NULL;
    }
    
    if (rx_sem) {
        rt_sem_delete(rx_sem);
        rx_sem = NULL;
    }
    
    rt_kprintf("[Finsh] Data handler deinitialized\n");
    rt_kprintf("[Finsh] Statistics: Commands: %u, Invalid: %u, Timeouts: %u\n",
              g_serial_status.total_commands_received,
              g_serial_status.invalid_commands_count,
              g_serial_status.timeout_count);
    return RT_EOK;
}