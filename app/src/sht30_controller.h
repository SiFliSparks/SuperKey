#ifndef SHT30_CONTROLLER_H
#define SHT30_CONTROLLER_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SHT30_FORMAT_TEXT,      
    SHT30_FORMAT_CSV,       
    SHT30_FORMAT_JSON,      
    SHT30_FORMAT_BINARY,    
    SHT30_FORMAT_SI         
} sht30_format_t;

typedef struct {
    float temperature_c;    
    float temperature_k;    
    float temperature_f;    
    float humidity_rh;      

    float dew_point_c;      
    float humidity_abs;     
    float vapor_pressure;   
    float enthalpy;         

    rt_tick_t timestamp;    
    uint32_t sample_count;  

    bool valid;             
    uint8_t crc_status;     
} sht30_data_t;

typedef struct {
    bool enabled;           
    uint32_t interval_ms;   
    sht30_format_t format;  
    bool include_derived;   
} sht30_report_config_t;


int sht30_controller_init(void);

int sht30_controller_deinit(void);

int sht30_controller_read(sht30_data_t *data);

int sht30_controller_start_continuous(uint32_t interval_ms);

int sht30_controller_stop_continuous(void);

int sht30_controller_config_report(const sht30_report_config_t *config);

int sht30_controller_send_data(sht30_format_t format);

int sht30_controller_get_latest(sht30_data_t *data);

int sht30_controller_get_statistics(float *temp_avg, float *humi_avg, 
                                    float *temp_min, float *temp_max,
                                    float *humi_min, float *humi_max);

int sht30_controller_reset_statistics(void);

int sht30_controller_set_temp_offset(float offset);

int sht30_controller_set_humi_offset(float offset);

int sht30_controller_soft_reset(void);

bool sht30_controller_is_ready(void);

uint32_t sht30_controller_get_error_count(void);

#ifdef __cplusplus
}
#endif
#endif /* SHT30_CONTROLLER_H */