/**
 * @file sdcard_monitor.c
 * @brief SD卡状态模块（精简版）
 * 
 * 热插拔监控功能已删除，因为：
 * 1. 上电无卡时sd0设备不会创建，后续插入无法检测
 * 2. 运行中插入SD卡会触发硬件复位（疑似硬件问题）
 */

#include "../device/sdcard_monitor.h"
#include <rtthread.h>
#include "../fs/fs_init.h"

static bool g_sd_online = false;

/**
 * @brief 初始化SD卡监控模块
 * @note  仅记录初始状态，不启动监控线程
 */
int sdcard_monitor_init(void)
{
    g_sd_online = fs_is_sdcard_mounted();
    rt_kprintf("[SDMON] SD card %s\n", g_sd_online ? "online" : "offline");
    return 0;
}

/**
 * @brief 检查SD卡是否在线
 */
bool sdcard_is_online(void)
{
    return g_sd_online;
}