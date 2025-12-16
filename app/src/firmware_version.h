/**
 * @file firmware_version.h
 * @brief 固件版本号定义
 */

#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

#define FW_VERSION_TYPE         "dev"

/**
 * @brief 主版本号 (Major)
 * 重大功能变更或不兼容的API修改时递增
 */
#define FW_VERSION_MAJOR        1

/**
 * @brief 次版本号 (Minor)
 * 新增功能但保持向下兼容时递增
 */
#define FW_VERSION_MINOR        2

/**
 * @brief 修订版本号 (Patch)
 * Bug修复或小改动时递增
 */
#define FW_VERSION_PATCH        5

/* 辅助宏用于字符串化 */
#define FW_STRINGIFY(x)         #x
#define FW_TOSTRING(x)          FW_STRINGIFY(x)

/**
 * @brief 完整版本字符串
 * 格式: "release1.1.2" 或 "dev1.1.2"
 */
#define FW_VERSION_STRING       FW_VERSION_TYPE \
                                FW_TOSTRING(FW_VERSION_MAJOR) "." \
                                FW_TOSTRING(FW_VERSION_MINOR) "." \
                                FW_TOSTRING(FW_VERSION_PATCH)

/**
 * @brief 纯数字版本字符串
 * 格式: "1.1.2"
 */
#define FW_VERSION_NUMBER       FW_TOSTRING(FW_VERSION_MAJOR) "." \
                                FW_TOSTRING(FW_VERSION_MINOR) "." \
                                FW_TOSTRING(FW_VERSION_PATCH)

/**
 * @brief 版本号整数表示 (便于比较)
 * 格式: 0xMMmmpp (MM=Major, mm=Minor, pp=Patch)
 */
#define FW_VERSION_CODE         ((FW_VERSION_MAJOR << 16) | \
                                 (FW_VERSION_MINOR << 8)  | \
                                 (FW_VERSION_PATCH))

/* ============================================================================
 * API 函数
 * ============================================================================ */

/**
 * @brief 获取完整版本字符串
 * @return 版本字符串指针 (如 "release1.1.2")
 */
const char* firmware_get_version_string(void);

/**
 * @brief 获取版本类型
 * @return "release" 或 "dev"
 */
const char* firmware_get_version_type(void);

/**
 * @brief 获取版本号整数
 * @return 版本号整数 (0xMMmmpp)
 */
uint32_t firmware_get_version_code(void);

/**
 * @brief 获取主版本号
 */
uint8_t firmware_get_major(void);

/**
 * @brief 获取次版本号
 */
uint8_t firmware_get_minor(void);

/**
 * @brief 获取修订版本号
 */
uint8_t firmware_get_patch(void);

/**
 * @brief 检查是否为发布版本
 * @return true=release版本, false=dev版本
 */
bool firmware_is_release(void);

#ifdef __cplusplus
}
#endif

#endif /* FIRMWARE_VERSION_H */