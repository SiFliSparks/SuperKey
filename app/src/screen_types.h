/**
 * @file screen_types.h
 * @brief 屏幕系统通用数据结构定义
 * 
 * 这个文件包含了屏幕系统中使用的所有数据结构，
 * 可以被多个模块引用，避免循环依赖问题。
 */

#ifndef SCREEN_TYPES_H
#define SCREEN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 屏幕组定义 */
typedef enum {
    SCREEN_GROUP_1 = 0,  /* 第一组：时间/天气/股票 */
    SCREEN_GROUP_2,      /* 第二组：CPU-GPU/内存/网络 */
    SCREEN_GROUP_3,      /* 第三组：HID快捷键 */
    SCREEN_GROUP_MAX
} screen_group_t;

/* 界面层级定义 */
typedef enum {
    SCREEN_LEVEL_1 = 0,  /* 第一层级：主界面 */
    SCREEN_LEVEL_2,      /* 第二层级：子界面 */
    SCREEN_LEVEL_MAX
} screen_level_t;

/* 第二层级组定义 */
typedef enum {
    SCREEN_L2_TIME_GROUP = 0,    /* 时间扩展组 */
    SCREEN_L2_WEATHER_GROUP,     /* 天气扩展组（预留） */
    SCREEN_L2_SYSTEM_GROUP,      /* 系统扩展组（预留） */
    SCREEN_L2_MEDIA_GROUP,       /* 媒体控制扩展组 */
    SCREEN_L2_WEB_GROUP,         
    SCREEN_L2_SHORTCUT_GROUP,    
    SCREEN_L2_GROUP_MAX
} screen_l2_group_t;

/* 第二层级页面定义 */
typedef enum {
    /* 时间扩展组页面 */
    SCREEN_L2_TIME_DETAIL = 0,   /* 时间详情页 */
    
    /* 媒体控制组页面 */
    SCREEN_L2_MEDIA_CONTROL = 1, /* 媒体控制页 */
    
    /* 网页控制组页面  */
    SCREEN_L2_WEB_CONTROL = 2,   /* 网页控制页 */
    
    /* 快捷键控制组页面  */
    SCREEN_L2_SHORTCUT_CONTROL = 3, /* 快捷键控制页 */
    
    /* 其他组页面（预留） */
    SCREEN_L2_PAGE_MAX
} screen_l2_page_t;

/* 层级切换上下文结构 */
typedef struct {
    screen_level_t current_level;       /* 当前层级 */
    screen_group_t l1_current_group;    /* 第一层级当前组 */
    screen_l2_group_t l2_current_group; /* 第二层级当前组 */
    screen_l2_page_t l2_current_page;   /* 第二层级当前页面 */
    screen_group_t l1_previous_group;   /* 返回时的第一层级组 */
} screen_hierarchy_context_t;

/* 简化版天气数据结构 - 仅包含finsh协议支持的字段 */
typedef struct {
    char city[32];           /* 城市名称（通过city_code映射） */
    char weather[32];        /* 天气描述（通过weather_code映射） */
    float temperature;       /* 温度(°C) - 来自temp字段 */
    float humidity;          /* 湿度(%) - 来自humidity字段 */
    int pressure;            /* 气压(hPa) ，来自pressure字段 */
    char update_time[32];    /* 更新时间 */
    bool valid;              /* 数据有效性 */
    
    /* 内部字段，用于映射 */
    int weather_code;        /* 天气代码 */
    int city_code;           /* 城市代码 */
} weather_data_t;

/* 简化版股票数据结构 - 仅包含finsh协议支持的字段 */
typedef struct {
    char symbol[16];         /* 股票代码（默认值） */
    char name[64];           /* 股票名称 - 来自stock_name字段 */
    float current_price;     /* 当前价格 - 来自stock_price字段 */
    float change_value;      /* 涨跌额 - 来自stock_change字段 */
    float change_percent;    /* 涨跌幅(%) - 自动计算 */
    char update_time[32];    /* 更新时间 */
    bool valid;              /* 数据有效性 */
} stock_data_t;

/* 简化版系统监控数据结构 - 仅包含finsh协议支持的字段 */
typedef struct {
    /* CPU相关 - 来自cpu和cpu_temp字段 */
    float cpu_usage;        /* CPU使用率 (%) */
    float cpu_temp;         /* CPU温度 (°C) */
    
    /* GPU相关 - 来自gpu和gpu_temp字段 */
    float gpu_usage;        /* GPU使用率 (%) */
    float gpu_temp;         /* GPU温度 (°C) */
    
    /* 内存相关 - 来自mem字段 */
    float ram_usage;        /* 内存使用率 (%) */
    
    /* 网络相关 - 来自net_up和net_down字段 */
    float net_upload_speed; /* 上传速度 (MB/s) */
    float net_download_speed; /* 下载速度 (MB/s) */
    
    char update_time[32];   /* 更新时间 */
    bool valid;             /* 数据有效性 */
} system_monitor_data_t;

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_TYPES_H */