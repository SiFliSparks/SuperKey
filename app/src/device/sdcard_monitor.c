/**
 * @file sdcard_monitor.c
 * @brief SD卡热插拔监控模块
 * 
 * 功能:
 * - 定时检测SD卡状态
 * - 拔出时通知相关模块
 * - 插入时重新挂载并通知相关模块重新加载资源
 */

#include "../device/sdcard_monitor.h"
#include <rtthread.h>
#include <dfs_file.h>
#include "spi_msd.h"
#include "../fs/fs_init.h"
#include "../custom/custom_icon_loader.h"
#include "../mp3/mp3_player_controller.h"

#define LOG_TAG                 "SDMON"
#define MONITOR_PERIOD_MS       1000    /* 检测周期1秒 */
#define SDCARD_DEVICE_NAME      "sd0"

static rt_thread_t g_monitor_thread = RT_NULL;
static bool g_last_mounted = false;
static bool g_sd_online = false;

/**
 * @brief 检测SD卡是否物理存在（通过发送CMD8检测）
 */
static bool detect_sdcard_present(void)
{
    rt_device_t dev = rt_device_find(SDCARD_DEVICE_NAME);
    if (dev == RT_NULL) {
        return false;
    }
    
    /* 使用 RT_DEVICE_CTRL_GET_INT 触发 msd_detection */
    rt_err_t result = rt_device_control(dev, RT_DEVICE_CTRL_GET_INT, RT_NULL);
    return (result == RT_EOK);
}

/**
 * @brief SD卡拔出处理
 */
static void on_sdcard_removed(void)
{
    rt_kprintf("[%s] SD card removed\n", LOG_TAG);
    
    g_sd_online = false;
    
    /* 停止MP3播放 */
    mp3_player_stop();
    
    /* 卸载文件系统 */
    fs_unmount_sdcard();
}

/**
 * @brief SD卡插入处理
 */
static void on_sdcard_inserted(void)
{
    rt_kprintf("[%s] SD card inserted\n", LOG_TAG);
    
    /* 重新初始化SD卡 */
    if (msd_reinit() != RT_EOK) {
        rt_kprintf("[%s] SD card reinit failed\n", LOG_TAG);
        return;
    }
    
    /* 延时等待SD卡稳定 */
    rt_thread_mdelay(200);
    
    /* 重新挂载文件系统 */
    if (fs_remount_sdcard() != RT_EOK) {
        rt_kprintf("[%s] SD card mount failed\n", LOG_TAG);
        return;
    }
    
    g_sd_online = true;
    
    /* 重新加载自定义图标 */
    custom_icon_reload();
    
    /* 重新扫描MP3音乐 */
    mp3_player_rescan();
    
    rt_kprintf("[%s] SD card ready\n", LOG_TAG);
}

/**
 * @brief SD卡监控线程
 */
static void sdcard_monitor_thread_entry(void *param)
{
    rt_kprintf("[%s] Monitor started\n", LOG_TAG);
    
    /* 获取初始状态 */
    g_last_mounted = fs_is_sdcard_mounted();
    g_sd_online = g_last_mounted;
    
    while (1) {
        rt_thread_mdelay(MONITOR_PERIOD_MS);
        
        bool current_mounted = fs_is_sdcard_mounted();
        
        if (g_last_mounted && !current_mounted) {
            /* 检测到SD卡可能被拔出 */
            if (!detect_sdcard_present()) {
                on_sdcard_removed();
            }
        }
        else if (!g_last_mounted) {
            /* SD卡之前未挂载，检测是否有新卡插入 */
            if (detect_sdcard_present()) {
                on_sdcard_inserted();
                current_mounted = fs_is_sdcard_mounted();
            }
        }
        
        g_last_mounted = current_mounted;
    }
}

/**
 * @brief 初始化SD卡监控模块
 */
int sdcard_monitor_init(void)
{
    g_monitor_thread = rt_thread_create("sd_mon",
                                        sdcard_monitor_thread_entry,
                                        RT_NULL,
                                        1024,
                                        20,
                                        10);
    if (g_monitor_thread != RT_NULL) {
        rt_thread_startup(g_monitor_thread);
        rt_kprintf("[%s] Initialized\n", LOG_TAG);
        return 0;
    }
    
    rt_kprintf("[%s] Failed to create thread\n", LOG_TAG);
    return -1;
}

/**
 * @brief 检查SD卡是否在线
 */
bool sdcard_is_online(void)
{
    return g_sd_online;
}