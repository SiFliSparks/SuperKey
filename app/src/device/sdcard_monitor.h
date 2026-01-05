/**
 * @file sdcard_monitor.h
 * @brief SD卡热插拔监控模块
 */

#ifndef SDCARD_MONITOR_H
#define SDCARD_MONITOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化SD卡监控模块
 * @return 0成功, <0失败
 */
int sdcard_monitor_init(void);

/**
 * @brief 检查SD卡是否在线
 * @return true在线, false离线
 */
bool sdcard_is_online(void);

#ifdef __cplusplus
}
#endif

#endif /* SDCARD_MONITOR_H */