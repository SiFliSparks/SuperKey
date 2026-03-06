/**
 * @file screen_types.h
 * @brief 屏幕系统通用数据结构定义 - 添加实用工具Group 4
 * 
 * 这个文件包含了屏幕系统中使用的所有数据结构，
 * 可以被多个模块引用，避免循环依赖问题。
 */

#ifndef SCREEN_TYPES_H
#define SCREEN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <rtthread.h>
#ifdef __cplusplus
extern "C" {
#endif

/* 屏幕组定义 - 新增Group 4和Group 5 */
typedef enum {
    SCREEN_GROUP_1 = 0,  // 第一组：时间/天气/传感器
    SCREEN_GROUP_2,      // 第二组：CPU-GPU/内存/网络
    SCREEN_GROUP_3,      // 第三组：HID快捷键
    SCREEN_GROUP_4,      // 第四组：实用工具（木鱼/番茄钟/秒表）
    SCREEN_GROUP_5,      // 第五组：自定义按键
    SCREEN_GROUP_MAX,
    SCREEN_GROUP_6 = SCREEN_GROUP_MAX,  // 第六组：MP3音乐播放器（暂时隐藏，不参与滚动切换）
} screen_group_t;

/* 界面层级定义 */
typedef enum {
    SCREEN_LEVEL_1 = 0,  /* 第一层级：主界面 */
    SCREEN_LEVEL_2,      /* 第二层级：子界面 */
    SCREEN_LEVEL_MAX
} screen_level_t;

/* 第二层级组定义 - 新增实用工具相关组 */
typedef enum {
    SCREEN_L2_TIME_GROUP = 0,    /* 时间扩展组 */
    SCREEN_L2_WEATHER_GROUP,     /* 天气扩展组 */
    SCREEN_L2_SYSTEM_GROUP,      /* 系统扩展组（预留） */
    SCREEN_L2_MEDIA_GROUP,       /* 媒体控制扩展组 */
    SCREEN_L2_WEB_GROUP,         
    SCREEN_L2_SHORTCUT_GROUP,    
    SCREEN_L2_MUYU_GROUP,        /* 赛博木鱼扩展组 - 新增 */
    SCREEN_L2_TOMATO_GROUP,      /* 番茄钟扩展组 - 新增 */
    SCREEN_L2_STOPWATCH_GROUP,     /* 秒表扩展组 - 新增 */
    SCREEN_L2_WIDGET_SELECTOR_GROUP, // 小工具选择器
    
    SCREEN_L2_GROUP_MAX
} screen_l2_group_t;

/* 第二层级页面定义 - 新增实用工具页面 */
typedef enum {
    /* 时间扩展组页面 */
    SCREEN_L2_TIME_DETAIL = 0,   /* 时间详情页 */
    
    /* 媒体控制组页面 */
    SCREEN_L2_MEDIA_CONTROL = 1, /* 媒体控制页 */
    
    /* 网页控制组页面  */
    SCREEN_L2_WEB_CONTROL = 2,   /* 网页控制页 */
    
    /* 快捷键控制组页面  */
    SCREEN_L2_SHORTCUT_CONTROL = 3, /* 快捷键控制页 */
    
    /* 赛博木鱼组页面 - 新增 */
    SCREEN_L2_MUYU_MAIN = 4,     /* 木鱼主界面 */
    
    /* 番茄钟组页面 - 新增 */
    SCREEN_L2_TOMATO_TIMER = 5,  /* 番茄钟计时器 */
    
    /* 秒表组页面 - 新增 */
    SCREEN_L2_STOPWATCH_TIMER = 6,  /* 秒表 */

    SCREEN_L2_WEATHER_FORECAST = 7, /* 天气预报页 */

    SCREEN_L2_WIDGET_SELECTOR = 8, /* 小工具选择器页面 */
    
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

/* 赛博木鱼数据结构 - 新增 */
typedef struct {
    uint32_t tap_count;          /* 敲击计数 */
    uint32_t total_taps;         /* 历史总计数 */
    uint32_t session_taps;       /* 本次会话计数 */
    char last_tap_time[32];      /* 最后敲击时间 */
    bool sound_enabled;          /* 音效开关 */
    uint8_t tap_effect_level;    /* 敲击特效级别 */
    bool auto_save;              /* 自动保存开关 */
} muyu_data_t;

/* 番茄钟数据结构 - 新增（预留） */
typedef struct {
    uint32_t work_duration_min;  /* 工作时长（分钟） */
    uint32_t break_duration_min; /* 休息时长（分钟） */
    uint32_t remaining_seconds;  /* 剩余秒数 */
    bool is_running;             /* 是否正在运行 */
    bool is_work_session;        /* 是否工作时段 */
    uint32_t completed_sessions; /* 完成的番茄钟数 */
} tomato_timer_data_t;

/* 秒表数据结构 - 新增（预留） */
typedef struct {
    uint32_t elapsed_deciseconds;  /* 已用时间(十分之一秒) */
    bool is_running;               /* 是否正在运行 */
    rt_tick_t start_tick;          /* 开始时刻 */
    rt_tick_t pause_tick;          /* 暂停时刻 */
    uint32_t pause_duration;       /* 累计暂停时长(tick) */
    bool valid;                    /* 数据有效性 */
} stopwatch_data_t;

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

/* 单日天气预报数据结构 */
typedef struct {
    char text[32];           /* 天气描述，如"阴"、"多云" */
    int temp_max;            /* 最高温度(°C) */
    int temp_min;            /* 最低温度(°C) */
    char wind_dir[32];       /* 风向，如"西北风" */
    char wind_scale[16];     /* 风力等级，如"3-4" */
    bool valid;              /* 数据有效性 */
} forecast_day_data_t;

/* 三天天气预报数据结构 */
typedef struct {
    forecast_day_data_t day0;  /* 今天 */
    forecast_day_data_t day1;  /* 明天 */
    forecast_day_data_t day2;  /* 后天 */
    char update_time[32];      /* 更新时间 */
    bool valid;                /* 整体数据有效性 */
} weather_forecast_data_t;

/* 简化版系统监控数据结构 - 仅包含finsh协议支持的字段 */
typedef struct {
    /* CPU相关 */
    float cpu_usage;        /* CPU使用率 (%) */
    float cpu_temp;         /* CPU温度 (°C) */
    float cpu_freq;         /* CPU频率 (MHz) */
    
    /* GPU相关 */
    float gpu_usage;        /* GPU使用率 (%) */
    float gpu_temp;         /* GPU温度 (°C) */
    float gpu_mem_used;     /* GPU显存已用 (GB) */
    float gpu_mem_total;    /* GPU显存总计 (GB) */
    
    /* 内存相关 */
    float ram_usage;        /* 内存使用率 (%) */
    float ram_used;         /* 内存已用 (GB) */
    float ram_total;        /* 内存总计 (GB) */
    
    /* 网络相关 */
    float net_upload_speed; /* 上传速度 (MB/s) */
    float net_download_speed; /* 下载速度 (MB/s) */
    
    char update_time[32];   /* 更新时间 */
    bool valid;             /* 数据有效性 */
} system_monitor_data_t;

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_TYPES_H */