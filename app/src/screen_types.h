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

#ifdef __cplusplus
extern "C" {
#endif

/* 屏幕组定义 - 新增Group 4 */
typedef enum {
    SCREEN_GROUP_1 = 0,  /* 第一组：时间/天气/股票 */
    SCREEN_GROUP_2,      /* 第二组：CPU-GPU/内存/网络 */
    SCREEN_GROUP_3,      /* 第三组：HID快捷键 */
    SCREEN_GROUP_4,      /* 第四组：实用工具（木鱼/番茄钟/全屏图片） */
    SCREEN_GROUP_MAX
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
    SCREEN_L2_WEATHER_GROUP,     /* 天气扩展组（预留） */
    SCREEN_L2_SYSTEM_GROUP,      /* 系统扩展组（预留） */
    SCREEN_L2_MEDIA_GROUP,       /* 媒体控制扩展组 */
    SCREEN_L2_WEB_GROUP,         
    SCREEN_L2_SHORTCUT_GROUP,    
    SCREEN_L2_MUYU_GROUP,        /* 赛博木鱼扩展组 - 新增 */
    SCREEN_L2_TOMATO_GROUP,      /* 番茄钟扩展组 - 新增 */
    SCREEN_L2_GALLERY_GROUP,     /* 全屏图片扩展组 - 新增 */
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
    
    /* 全屏图片组页面 - 新增 */
    SCREEN_L2_GALLERY_VIEW = 6,  /* 图片查看器 */
    
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

/* 全屏图片数据结构 - 新增（预留） */
typedef struct {
    uint8_t current_image_index; /* 当前图片索引 */
    uint8_t total_images;        /* 图片总数 */
    bool slideshow_enabled;      /* 幻灯片模式 */
    uint32_t slide_interval_ms;  /* 幻灯片间隔 */
    bool zoom_enabled;           /* 缩放功能 */
    float zoom_factor;           /* 缩放比例 */
} gallery_data_t;

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