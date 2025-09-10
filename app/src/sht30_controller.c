#include "sht30_controller.h"
#include <rtdevice.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bf0_hal.h"
#include "event_bus.h"

#define SHT30_I2C_ADDR    0x44
#define SHT30_I2C_BUS     "i2c2"
#define SHT30_SCL_PIN     39    
#define SHT30_SDA_PIN     37     
#define SHT30_SCL_PAD     PAD_PA39
#define SHT30_SDA_PAD     PAD_PA37

#define MAX_SAMPLES       50    
#define DEFAULT_INTERVAL  1000    
#define STATS_RESET_INTERVAL_MS (3600000) 

static struct {
    struct rt_i2c_bus_device *i2c_bus;
    sht30_data_t latest_data; 
    float temp_offset;
    float humi_offset;
    sht30_report_config_t report_config;
    rt_thread_t sampling_thread;
    rt_sem_t stop_sem;
    uint32_t sampling_interval;
    bool sampling_enabled;
    bool initialized;
    uint32_t error_count;
    uint32_t success_count;
    rt_mutex_t lock;
} g_sht30 = {0};

static uint8_t sht30_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static float calculate_dew_point(float temp_c, float rh)
{
    float a = 17.271;
    float b = 237.7;
    float gamma = (a * temp_c) / (b + temp_c) + log(rh / 100.0);
    return (b * gamma) / (a - gamma);
}

static float calculate_absolute_humidity(float temp_c, float rh)
{
    float es = 6.112 * exp((17.67 * temp_c) / (temp_c + 243.5));
    float e = es * (rh / 100.0);
    return (e * 216.7) / (temp_c + 273.15);
}

static float calculate_vapor_pressure(float temp_c, float rh)
{
    float es = 610.78 * exp((17.269 * temp_c) / (temp_c + 237.3));
    return es * (rh / 100.0);
}

static float calculate_enthalpy(float temp_c, float rh)
{
    float x = calculate_absolute_humidity(temp_c, rh) / 1000.0; /* kg/kg */
    return temp_c * (1.006 + 1.86 * x) + 2500 * x;
}

static rt_err_t sht30_configure_pins(void)
{
    rt_thread_mdelay(100);
    HAL_PIN_Set(SHT30_SCL_PAD, I2C2_SCL, PIN_PULLUP, 1);
    HAL_PIN_Set(SHT30_SDA_PAD, I2C2_SDA, PIN_PULLUP, 1);
    rt_thread_mdelay(50);
    return RT_EOK;
}

static rt_err_t sht30_read_raw(uint8_t *data)
{
    if (!g_sht30.i2c_bus || !data) {
        return -RT_ERROR;
    }

    uint8_t cmd[2] = {0x2C, 0x06};
    
    struct rt_i2c_msg msg;
    msg.addr = SHT30_I2C_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf = cmd;
    msg.len = 2;

    
    if (rt_i2c_transfer(g_sht30.i2c_bus, &msg, 1) != 1) {
        g_sht30.error_count++;
        return -RT_ERROR;
    }

    rt_thread_mdelay(50);
    
    msg.flags = RT_I2C_RD;
    msg.buf = data;
    msg.len = 6;

    
    if (rt_i2c_transfer(g_sht30.i2c_bus, &msg, 1) != 1) {
        g_sht30.error_count++;
        return -RT_ERROR;
    }
    
    rt_kprintf("[SHT30] Data read: %02X %02X %02X %02X %02X %02X\n", 
               data[0], data[1], data[2], data[3], data[4], data[5]);

    uint8_t temp_crc = sht30_crc8(&data[0], 2);
    uint8_t humi_crc = sht30_crc8(&data[3], 2);

    uint16_t temp_raw = (data[0] << 8) | data[1];
    uint16_t humi_raw = (data[3] << 8) | data[4];
    
    rt_kprintf("[SHT30] Raw: T=0x%04X, H=0x%04X\n", temp_raw, humi_raw);
    
    g_sht30.success_count++;
    return RT_EOK;
}

static void format_output(const sht30_data_t *data, sht30_format_t format)
{
    char buffer[512];
    
    switch (format) {
    case SHT30_FORMAT_TEXT:
        rt_kprintf("=== SHT30 Sensor Data ===\n");
        rt_kprintf("Temperature: %.2fÂ°C (%.2fK, %.2fÂ°F)\n", 
                  data->temperature_c, data->temperature_k, data->temperature_f);
        rt_kprintf("Humidity: %.2f%%RH\n", data->humidity_rh);
        if (g_sht30.report_config.include_derived) {
            rt_kprintf("Dew Point: %.2fÂ°C\n", data->dew_point_c);
            rt_kprintf("Absolute Humidity: %.2f g/mÂ³\n", data->humidity_abs);
            rt_kprintf("Vapor Pressure: %.2f Pa\n", data->vapor_pressure);
            rt_kprintf("Enthalpy: %.2f kJ/kg\n", data->enthalpy);
        }
        rt_kprintf("Sample #%d @ tick %d\n", data->sample_count, data->timestamp);
        break;
        
    case SHT30_FORMAT_CSV:
        rt_snprintf(buffer, sizeof(buffer),
                   "%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                   data->sample_count, data->timestamp,
                   data->temperature_c, data->temperature_k, data->humidity_rh,
                   data->dew_point_c, data->humidity_abs, 
                   data->vapor_pressure, data->enthalpy,
                   data->temperature_f);
        rt_kprintf("%s", buffer);
        break;
        
    case SHT30_FORMAT_JSON:
        rt_snprintf(buffer, sizeof(buffer),
                   "{\"n\":%d,\"t\":%d,\"temp_c\":%.2f,\"temp_k\":%.2f,"
                   "\"temp_f\":%.2f,\"rh\":%.2f,\"dp\":%.2f,\"ah\":%.2f,"
                   "\"vp\":%.2f,\"h\":%.2f}\n",
                   data->sample_count, data->timestamp,
                   data->temperature_c, data->temperature_k, data->temperature_f,
                   data->humidity_rh, data->dew_point_c, data->humidity_abs,
                   data->vapor_pressure, data->enthalpy);
        rt_kprintf("%s", buffer);
        break;
        
    case SHT30_FORMAT_SI:
        rt_snprintf(buffer, sizeof(buffer),
                   "SI:%d:%.3f:%.3f:%.3f:%.3f:%.3f:%.3f:%.3f:%d\n",
                   data->sample_count,    
                   data->temperature_k,      
                   data->humidity_rh / 100.0,  
                   data->dew_point_c + 273.15, 
                   data->humidity_abs / 1000.0,
                   data->vapor_pressure,    
                   data->enthalpy * 1000.0,   
                   data->temperature_c,     
                   data->timestamp);        
        rt_kprintf("%s", buffer);
        break;
        
    case SHT30_FORMAT_BINARY:
        rt_kprintf("BIN:");
        rt_device_write(rt_console_get_device(), 0, data, sizeof(sht30_data_t));
        rt_kprintf("\n");
        break;
    }
}

static void sampling_thread_entry(void *parameter)
{
    sht30_data_t data;
    uint32_t error_streak = 0;
    
    while (1) {
        if (rt_sem_take(g_sht30.stop_sem, RT_WAITING_NO) == RT_EOK) {
            break;
        }

        if (sht30_controller_read(&data) == RT_EOK) {
            error_streak = 0;

            event_data_generic_t sensor_event = {
                .float_value = data.temperature_c, 
                .extra_data = {
                    *(uint32_t*)&data.humidity_rh, 
                    *(uint32_t*)&data.dew_point_c, 
                    data.timestamp,  
                    data.valid ? 1 : 0  
                }
            };
            
            event_bus_publish(EVENT_DATA_SENSOR_UPDATED, &sensor_event, sizeof(sensor_event),
                             EVENT_PRIORITY_NORMAL, MODULE_ID_SENSOR);

            if (g_sht30.report_config.enabled) {
                format_output(&data, g_sht30.report_config.format);
            }
        } else {
            error_streak++;
            
            if (error_streak >= 10) {
                rt_kprintf("[SHT30] Too many consecutive errors (%u), attempting soft reset\n", 
                          error_streak);
                sht30_controller_soft_reset();
                error_streak = 0;
            }
        }
        
        rt_thread_mdelay(g_sht30.sampling_interval);
    }
    
    rt_kprintf("[SHT30] Sampling thread stopped\n");
}

int sht30_controller_init(void)
{
    if (g_sht30.initialized) {
        rt_kprintf("[SHT30] Already initialized\n");
        return RT_EOK;
    }
    

    if (sht30_configure_pins() != RT_EOK) {
        rt_kprintf("[SHT30] ERROR: Pin configuration failed\n");
        return -RT_ERROR;
    }

    g_sht30.i2c_bus = rt_i2c_bus_device_find(SHT30_I2C_BUS);
    rt_kprintf("[SHT30] i2c_bus:0x%p\n", g_sht30.i2c_bus);
    
    if (!g_sht30.i2c_bus) {
        rt_kprintf("[SHT30] ERROR: Can not found i2c bus %s, init fail\n", SHT30_I2C_BUS);
        return -RT_ERROR;
    }
    
    rt_kprintf("[SHT30] Find i2c bus device %s\n", SHT30_I2C_BUS);

    rt_device_open((rt_device_t)g_sht30.i2c_bus, RT_DEVICE_FLAG_RDWR);

    struct rt_i2c_configuration config = {
        .mode = 0,
        .addr = 0,
        .timeout = 1000,
        .max_hz = 50000,
    };
    
    rt_err_t config_result = rt_i2c_configure(g_sht30.i2c_bus, &config);
    if (config_result != RT_EOK) {
        rt_kprintf("[SHT30] I2C configuration failed: %d\n", config_result);
        return -RT_ERROR;
    }
    
    rt_kprintf("[SHT30] I2C配置: 速率=%dHz, 超时=%dms\n", config.max_hz, config.timeout);

    g_sht30.lock = rt_mutex_create("sht30_lock", RT_IPC_FLAG_PRIO);
    if (!g_sht30.lock) {
        rt_kprintf("[SHT30] Failed to create mutex\n");
        return -RT_ENOMEM;
    }

    g_sht30.stop_sem = rt_sem_create("sht30_stop", 0, RT_IPC_FLAG_PRIO);
    if (!g_sht30.stop_sem) {
        rt_kprintf("[SHT30] Failed to create semaphore\n");
        rt_mutex_delete(g_sht30.lock);
        return -RT_ENOMEM;
    }

    g_sht30.sampling_interval = DEFAULT_INTERVAL;
    g_sht30.temp_offset = 0.0f;
    g_sht30.humi_offset = 0.0f;
    g_sht30.error_count = 0;
    g_sht30.success_count = 0;

    g_sht30.initialized = true;
    
    rt_kprintf("[SHT30] SHT30初始化成功！\n");
    return RT_EOK;
}

int sht30_controller_deinit(void)
{
    if (!g_sht30.initialized) {
        return 0;
    }

    sht30_controller_stop_continuous();

    if (g_sht30.stop_sem) {
        rt_sem_delete(g_sht30.stop_sem);
    }
    if (g_sht30.lock) {
        rt_mutex_delete(g_sht30.lock);
    }

    memset(&g_sht30, 0, sizeof(g_sht30));
    
    rt_kprintf("[SHT30] Controller deinitialized (auto-reset count: %u)\n", 
              g_sht30.success_count);
    return 0;
}

int sht30_controller_read(sht30_data_t *data)
{
    if (!g_sht30.initialized || !data) {
        rt_kprintf("[SHT30] Not initialized or data is null\n");
        return -RT_ERROR;
    }
    
    uint8_t raw[6];
    
    if (sht30_read_raw(raw) != RT_EOK) {
        data->valid = false;
        return -RT_ERROR;
    }

    uint16_t temp_raw = (raw[0] << 8) | raw[1];
    uint16_t humi_raw = (raw[3] << 8) | raw[4];

    float temp_c = -45.0f + 175.0f * ((float)temp_raw / 65535.0f);
    float humi_rh = 100.0f * ((float)humi_raw / 65535.0f);

    temp_c += g_sht30.temp_offset;
    humi_rh += g_sht30.humi_offset;
    
    if (humi_rh > 100.0f) humi_rh = 100.0f;
    if (humi_rh < 0.0f) humi_rh = 0.0f;

    data->temperature_c = temp_c;
    data->temperature_k = temp_c + 273.15f;
    data->temperature_f = temp_c * 9.0f / 5.0f + 32.0f;
    data->humidity_rh = humi_rh;

    data->dew_point_c = calculate_dew_point(temp_c, humi_rh);
    data->humidity_abs = calculate_absolute_humidity(temp_c, humi_rh);
    data->vapor_pressure = calculate_vapor_pressure(temp_c, humi_rh);
    data->enthalpy = calculate_enthalpy(temp_c, humi_rh);

    data->timestamp = rt_tick_get();
    data->sample_count = g_sht30.success_count;
    data->valid = true;
    data->crc_status = 0;

    rt_mutex_take(g_sht30.lock, RT_WAITING_FOREVER);
    g_sht30.latest_data = *data;
    rt_mutex_release(g_sht30.lock);
    
    rt_kprintf("[SHT30] 读取成功！\n");
    rt_kprintf("[SHT30] 温度: %.2f°C\n", data->temperature_c);
    rt_kprintf("[SHT30] 湿度: %.2f%%\n", data->humidity_rh);
    
    return RT_EOK;
}

int sht30_controller_start_continuous(uint32_t interval_ms)
{
    if (!g_sht30.initialized) {
        return -RT_ERROR;
    }
    
    if (g_sht30.sampling_enabled) {
        rt_kprintf("[SHT30] Continuous sampling already running\n");
        return 0;
    }
    
    g_sht30.sampling_interval = interval_ms;

    g_sht30.sampling_thread = rt_thread_create("sht30_sample",
                                               sampling_thread_entry,
                                               RT_NULL,
                                               3072, 
                                               15, 10);
    if (!g_sht30.sampling_thread) {
        return -RT_ENOMEM;
    }
    
    g_sht30.sampling_enabled = true;
    rt_thread_startup(g_sht30.sampling_thread);
    
    rt_kprintf("[SHT30] Continuous sampling started (interval=%dms, stack=3KB)\n", interval_ms);
    return 0;
}

int sht30_controller_stop_continuous(void)
{
    if (!g_sht30.initialized || !g_sht30.sampling_enabled) {
        return 0;
    }

    rt_sem_release(g_sht30.stop_sem);

    rt_thread_mdelay(100);
    
    g_sht30.sampling_enabled = false;
    g_sht30.sampling_thread = RT_NULL;

    while (rt_sem_take(g_sht30.stop_sem, RT_WAITING_NO) == RT_EOK);
    
    rt_kprintf("[SHT30] Continuous sampling stopped\n");
    return 0;
}

int sht30_controller_config_report(const sht30_report_config_t *config)
{
    if (!g_sht30.initialized || !config) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_sht30.lock, RT_WAITING_FOREVER);
    g_sht30.report_config = *config;
    rt_mutex_release(g_sht30.lock);
    
    rt_kprintf("[SHT30] Report configured: enabled=%d, interval=%dms, format=%d\n",
              config->enabled, config->interval_ms, config->format);
    return 0;
}

int sht30_controller_send_data(sht30_format_t format)
{
    if (!g_sht30.initialized) {
        return -RT_ERROR;
    }
    
    sht30_data_t data;

    if (sht30_controller_read(&data) != RT_EOK) {
        rt_kprintf("[SHT30] Failed to read data\n");
        return -RT_ERROR;
    }

    format_output(&data, format);
    
    return RT_EOK;
}

int sht30_controller_get_latest(sht30_data_t *data)
{
    if (!g_sht30.initialized || !data) {
        return -RT_ERROR;
    }
    
    rt_mutex_take(g_sht30.lock, RT_WAITING_FOREVER);
    *data = g_sht30.latest_data;
    rt_mutex_release(g_sht30.lock);
    
    return data->valid ? RT_EOK : -RT_ERROR;
}

int sht30_controller_set_temp_offset(float offset)
{
    g_sht30.temp_offset = offset;
    rt_kprintf("[SHT30] Temperature offset set to %.2fÂ°C\n", offset);
    return 0;
}

int sht30_controller_set_humi_offset(float offset)
{
    g_sht30.humi_offset = offset;
    rt_kprintf("[SHT30] Humidity offset set to %.2f%%\n", offset);
    return 0;
}

int sht30_controller_soft_reset(void)
{
    if (!g_sht30.i2c_bus) {
        return -RT_ERROR;
    }

    uint8_t cmd[2] = {0x30, 0xA2};
    struct rt_i2c_msg msg;
    
    msg.addr = SHT30_I2C_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf = cmd;
    msg.len = 2;
    
    if (rt_i2c_transfer(g_sht30.i2c_bus, &msg, 1) != 1) {
        return -RT_ERROR;
    }
    
    rt_thread_mdelay(20);
    rt_kprintf("[SHT30] Soft reset completed\n");
    return RT_EOK;
}

bool sht30_controller_is_ready(void)
{
    return g_sht30.initialized && g_sht30.i2c_bus != NULL;
}

uint32_t sht30_controller_get_error_count(void)
{
    return g_sht30.error_count;
}


void sht30_scan_i2c_bus(void)
{
    if (!g_sht30.i2c_bus) {
        rt_kprintf("[SHT30] I2C bus not available for scanning\n");
        return;
    }
    
    rt_kprintf("[SHT30] Scanning I2C bus %s...\n", SHT30_I2C_BUS);
    
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        struct rt_i2c_msg msg;
        uint8_t dummy = 0;
        
        msg.addr = addr;
        msg.flags = RT_I2C_WR;
        msg.buf = &dummy;
        msg.len = 0; 
        
        if (rt_i2c_transfer(g_sht30.i2c_bus, &msg, 1) == 1) {
            rt_kprintf("[SHT30] Found device at address: 0x%02X\n", addr);
        }
    }
    
    rt_kprintf("[SHT30] I2C scan completed\n");
}

