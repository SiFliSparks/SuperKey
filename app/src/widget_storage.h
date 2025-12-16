/**
 * @file widget_storage.h
 * @brief 小工具配置Flash存储模块 - 头文件
 * 
 * 支持断电记忆功能，保存用户选择的小工具配置
 */

#ifndef WIDGET_STORAGE_H
#define WIDGET_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define WIDGET_STORAGE_MAGIC        0x57494447  /* "WIDG" */
#define WIDGET_STORAGE_VERSION      1

/* 小工具类型定义 */
typedef enum {
    WIDGET_TYPE_NONE = 0,           /* 未选择 */
    WIDGET_TYPE_RANDOM_NUMBER,      /* 随机数工具 */
    WIDGET_TYPE_OFF_WORK_COUNTDOWN, /* 下班倒计时 */
    WIDGET_TYPE_WHAT_TO_EAT,        /* 今天吃什么 */
    WIDGET_TYPE_MAX
} widget_type_t;

/* 随机数工具最大值选项 */
typedef enum {
    RANDOM_MAX_10 = 0,
    RANDOM_MAX_100,
    RANDOM_MAX_1000,
    RANDOM_MAX_OPTIONS
} random_max_option_t;

/* 随机数工具配置 */
typedef struct {
    random_max_option_t max_option;  /* 最大值选项: 10/100/1000 */
} widget_random_config_t;

/* 下班倒计时配置 */
typedef struct {
    uint8_t target_hour;             /* 目标小时 (0-23) */
    uint8_t target_minute;           /* 目标分钟 (0-59) */
} widget_offwork_config_t;

/* 今天吃什么配置 (暂无额外配置) */
typedef struct {
    uint8_t reserved;
} widget_food_config_t;

/* 单个小工具配置 */
typedef struct {
    widget_type_t type;              /* 小工具类型 */
    union {
        widget_random_config_t random;
        widget_offwork_config_t offwork;
        widget_food_config_t food;
    } config;
} widget_config_t;

/* 完整存储结构 */
typedef struct {
    uint32_t magic;                  /* 魔数标识 */
    uint32_t version;                /* 版本号 */
    widget_config_t active_widget;   /* 当前激活的小工具 */
    uint32_t crc;                    /* CRC校验 */
} widget_storage_t;

/* ============================================================================
 * API 函数
 * ============================================================================ */

/**
 * @brief 初始化小工具存储模块
 * @return 0 成功, <0 失败
 */
int widget_storage_init(void);

/**
 * @brief 从Flash加载配置
 * @return 0 成功, <0 失败
 */
int widget_storage_load(void);

/**
 * @brief 保存配置到Flash
 * @return 0 成功, <0 失败
 */
int widget_storage_save(void);

/**
 * @brief 设置当前激活的小工具
 * @param config 小工具配置
 * @return 0 成功, <0 失败
 */
int widget_storage_set_active(const widget_config_t *config);

/**
 * @brief 获取当前激活的小工具配置
 * @param config 输出配置
 * @return 0 成功, <0 失败
 */
int widget_storage_get_active(widget_config_t *config);

/**
 * @brief 获取小工具类型名称
 * @param type 小工具类型
 * @return 类型名称字符串
 */
const char* widget_get_type_name(widget_type_t type);

/**
 * @brief 获取随机数最大值
 * @param option 最大值选项
 * @return 实际最大值
 */
int widget_get_random_max_value(random_max_option_t option);

/**
 * @brief 重置为默认配置
 */
void widget_storage_reset(void);

/**
 * @brief 检查配置是否有效
 * @return true 有效, false 无效
 */
bool widget_storage_is_valid(void);

/**
 * @brief 调试用：打印当前配置
 */
void widget_storage_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_STORAGE_H */
