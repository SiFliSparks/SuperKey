/**
 * @file firmware_version.c
 * @brief 固件版本号实现
 */

#include "../version/firmware_version.h"
#include <stdbool.h>
#include <string.h>

const char* firmware_get_version_string(void)
{
    return FW_VERSION_STRING;
}

const char* firmware_get_version_type(void)
{
    return FW_VERSION_TYPE;
}

uint32_t firmware_get_version_code(void)
{
    return FW_VERSION_CODE;
}

uint8_t firmware_get_major(void)
{
    return FW_VERSION_MAJOR;
}

uint8_t firmware_get_minor(void)
{
    return FW_VERSION_MINOR;
}

uint8_t firmware_get_patch(void)
{
    return FW_VERSION_PATCH;
}

bool firmware_is_release(void)
{
    return (strcmp(FW_VERSION_TYPE, "release") == 0);
}