/**
 * @file fs_init.h
 * @brief 文件系统初始化模块 - 头文件
 * 
 * 支持内部Flash和TF卡(SD卡)文件系统
 */

#ifndef FS_INIT_H
#define FS_INIT_H

#include <stdbool.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 路径定义
 * ============================================================================ */

#define SDCARD_MOUNT_PATH       "/sdcard"       /* SD卡挂载路径 */

/* ============================================================================
 * API 函数
 * ============================================================================ */

/**
 * @brief 初始化并挂载所有文件系统
 * @note  此函数通过 INIT_ENV_EXPORT 自动调用
 * @return RT_EOK 成功, 其他失败
 */
int fs_init(void);

/**
 * @brief 检查内部Flash文件系统是否已挂载
 * @return true 已挂载, false 未挂载
 */
bool fs_is_mounted(void);

/**
 * @brief 检查TF卡是否已挂载
 * @return true 已挂载, false 未挂载
 */
bool fs_is_sdcard_mounted(void);

/**
 * @brief 获取TF卡挂载路径
 * @return 挂载路径字符串，未启用则返回NULL
 */
const char* fs_get_sdcard_path(void);

/**
 * @brief 卸载TF卡文件系统
 * @return RT_EOK 成功
 */
int fs_unmount_sdcard(void);

/**
 * @brief 重新挂载TF卡
 * @note  用于热插拔场景
 * @return RT_EOK 成功
 */
int fs_remount_sdcard(void);

#ifdef __cplusplus
}
#endif

#endif /* FS_INIT_H */
