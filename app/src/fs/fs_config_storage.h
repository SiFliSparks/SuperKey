/**
 * @file fs_config_storage.h
 * @brief 基于文件系统的配置存储模块 - 头文件
 * 
 * 替代直接 Flash 读写，使用文件系统进行配置持久化存储
 * 适用于 SF32LB52X 系列芯片
 */

#ifndef FS_CONFIG_STORAGE_H
#define FS_CONFIG_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置路径定义
 * ============================================================================ */

#define CONFIG_DIR              "/config"           /* 配置文件目录 */
#define WIDGET_CONFIG_FILE      "/config/widget.dat"
#define CUSTOM_KEY_CONFIG_FILE  "/config/keys.dat"

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

#define FS_CFG_OK               0
#define FS_CFG_ERR_NOT_INIT     (-1)
#define FS_CFG_ERR_NO_FS        (-2)
#define FS_CFG_ERR_OPEN         (-3)
#define FS_CFG_ERR_READ         (-4)
#define FS_CFG_ERR_WRITE        (-5)
#define FS_CFG_ERR_CRC          (-6)
#define FS_CFG_ERR_MAGIC        (-7)
#define FS_CFG_ERR_PARAM        (-8)

/* ============================================================================
 * API 函数
 * ============================================================================ */

/**
 * @brief 初始化文件系统配置存储模块
 * @note  必须在文件系统挂载之后调用
 * @return FS_CFG_OK 成功, <0 失败
 */
int fs_config_storage_init(void);

/**
 * @brief 检查文件系统是否就绪
 * @return true 就绪, false 未就绪
 */
bool fs_config_is_ready(void);

/**
 * @brief 保存配置数据到文件
 * @param filepath 文件路径
 * @param magic 魔数标识
 * @param version 版本号
 * @param data 数据指针
 * @param data_size 数据大小（不含头部）
 * @return FS_CFG_OK 成功, <0 失败
 */
int fs_config_save(const char *filepath, uint32_t magic, uint32_t version,
                   const void *data, size_t data_size);

/**
 * @brief 从文件加载配置数据
 * @param filepath 文件路径
 * @param expected_magic 期望的魔数
 * @param data 数据输出缓冲区
 * @param data_size 期望的数据大小
 * @param[out] version 读取到的版本号（可为NULL）
 * @return FS_CFG_OK 成功, <0 失败
 */
int fs_config_load(const char *filepath, uint32_t expected_magic,
                   void *data, size_t data_size, uint32_t *version);

/**
 * @brief 删除配置文件
 * @param filepath 文件路径
 * @return FS_CFG_OK 成功, <0 失败
 */
int fs_config_delete(const char *filepath);

/**
 * @brief 检查配置文件是否存在
 * @param filepath 文件路径
 * @return true 存在, false 不存在
 */
bool fs_config_exists(const char *filepath);

/**
 * @brief 获取配置文件信息
 * @param filepath 文件路径
 * @param[out] size 文件大小
 * @param[out] magic 魔数
 * @param[out] version 版本号
 * @return FS_CFG_OK 成功, <0 失败
 */
int fs_config_get_info(const char *filepath, size_t *size, 
                       uint32_t *magic, uint32_t *version);

#ifdef __cplusplus
}
#endif

#endif /* FS_CONFIG_STORAGE_H */
