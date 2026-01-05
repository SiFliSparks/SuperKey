/**
 * @file custom_key_storage.h
 * @brief 自定义按键存储模块 - 头文件
 * 
 * SF32LB52专用版本（文件系统存储）
 */

#ifndef CUSTOM_KEY_STORAGE_H
#define CUSTOM_KEY_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define CUSTOM_KEY_GROUP_COUNT      3       /* 自定义按键组数 */
#define CUSTOM_KEY_PER_GROUP        3       /* 每组按键数 */
#define CUSTOM_KEY_COMBO_MAX        4       /* 每个按键最多组合键数 */

#define CUSTOM_KEY_MAGIC            0x434B4559  /* "CKEY" */
#define CUSTOM_KEY_VERSION          2           /* 版本升级：文件系统存储 */

/* ============================================================================
 * 数据结构
 * ============================================================================ */

/* 单个组合键 */
typedef struct {
    uint8_t modifier;       /* 修饰键 (Ctrl=0x01, Shift=0x02, Alt=0x04, GUI=0x08) */
    uint8_t keycode;        /* HID keycode */
} key_combo_t;

/* 单个自定义按键配置 */
typedef struct {
    bool enabled;                               /* 是否启用 */
    uint8_t combo_count;                        /* 有效组合键数量 */
    key_combo_t combos[CUSTOM_KEY_COMBO_MAX];   /* 组合键序列 */
} custom_key_t;

/* 按键组 */
typedef struct {
    custom_key_t keys[CUSTOM_KEY_PER_GROUP];
} custom_key_group_t;

/* 配置数据结构（不含文件头，由 fs_config_storage 管理） */
typedef struct {
    custom_key_group_t groups[CUSTOM_KEY_GROUP_COUNT];  /* 按键组 */
} custom_key_data_t;

/* ============================================================================
 * API 函数
 * ============================================================================ */

/**
 * @brief 初始化自定义按键存储模块
 * @return 0 成功, <0 失败
 */
int custom_key_storage_init(void);

/**
 * @brief 从文件系统加载配置
 * @return 0 成功, <0 失败
 */
int custom_key_load_from_flash(void);

/**
 * @brief 保存配置到文件系统
 * @return 0 成功, <0 失败
 */
int custom_key_save_to_flash(void);

/**
 * @brief 设置单个按键配置
 * @param group 组索引 (0-2)
 * @param key_idx 按键索引 (0-2)
 * @param key_config 按键配置
 * @return 0 成功, <0 失败
 */
int custom_key_set(uint8_t group, uint8_t key_idx, const custom_key_t *key_config);

/**
 * @brief 获取单个按键配置
 * @param group 组索引 (0-2)
 * @param key_idx 按键索引 (0-2)
 * @param key_config 输出按键配置
 * @return 0 成功, <0 失败
 */
int custom_key_get(uint8_t group, uint8_t key_idx, custom_key_t *key_config);

/**
 * @brief 解析串口命令并设置按键
 * @param value 命令参数字符串 "group,key,mod1,key1,mod2,key2,mod3,key3,mod4,key4"
 * @return 0 成功, <0 失败
 */
int custom_key_parse_and_set(const char *value);

/**
 * @brief 获取完整配置数据指针 (只读)
 * @return 配置数据指针
 */
const custom_key_data_t* custom_key_get_data(void);

/**
 * @brief 重置为默认配置
 */
void custom_key_reset_to_default(void);

/**
 * @brief 检查配置是否有效
 * @return true 有效, false 无效
 */
bool custom_key_is_valid(void);

/**
 * @brief 调试用：打印当前配置
 */
void custom_key_dump_config(void);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_KEY_STORAGE_H */
