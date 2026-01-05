/**
 * @file fs_init.c
 * @brief 文件系统初始化模块 - 支持内部Flash和TF卡
 * 
 * 功能说明：
 * - 内部Flash文件系统挂载到根目录 "/"
 * - TF卡(SD卡)通过SPI接口挂载到 "/sdcard" 目录
 */

#include <rtthread.h>
#include <dfs_file.h>
#include <sys/stat.h>
#include "drv_flash.h"
#include "../fs/fs_config_storage.h"

/* 如果启用了SPI SD卡支持 */
#ifdef RT_USING_SPI_MSD
#include "spi_msd.h"
#endif

/* ============================================================================
 * 配置定义
 * ============================================================================ */

/* 内部Flash文件系统区域 */
#ifndef FS_REGION_START_ADDR
#define FS_REGION_START_ADDR    (0x128A0000)    /* 0x12000000 + 0x008A0000 */
#define FS_REGION_SIZE          (0x00400000)    /* 4MB */
#endif

#define FS_DEVICE_NAME          "root"

/* TF卡配置 */
#define SDCARD_DEVICE_NAME      "sd0"           /* SD卡块设备名 */
#define SDCARD_MOUNT_PATH       "/sdcard"       /* SD卡挂载路径 */
#define SDCARD_DETECT_TIMEOUT   100             /* SD卡检测超时次数 */
#define SDCARD_DETECT_DELAY_MS  30              /* 每次检测延时(ms) */

/* ============================================================================
 * 私有变量
 * ============================================================================ */

static bool g_fs_mounted = false;
static bool g_sdcard_mounted = false;

/* ============================================================================
 * 内部Flash文件系统挂载
 * ============================================================================ */

/**
 * @brief 初始化并挂载内部Flash文件系统
 * @return RT_EOK 成功, 其他失败
 */
static int fs_mount_internal_flash(void)
{
    rt_kprintf("[FS] Mounting internal flash...\n");
    rt_kprintf("[FS] Region: 0x%08X, Size: 0x%08X (%d KB)\n", 
               FS_REGION_START_ADDR, FS_REGION_SIZE, FS_REGION_SIZE / 1024);
    
    /* 注册 MTD 设备 */
    register_mtd_device(FS_REGION_START_ADDR, FS_REGION_SIZE, FS_DEVICE_NAME);
    
    /* 尝试挂载文件系统 */
    if (dfs_mount(FS_DEVICE_NAME, "/", "elm", 0, 0) == 0) {
        rt_kprintf("[FS] Internal flash mount success\n");
        g_fs_mounted = true;
        return RT_EOK;
    }
    
    /* 挂载失败，尝试格式化 */
    rt_kprintf("[FS] Mount failed, formatting...\n");
    
    if (dfs_mkfs("elm", FS_DEVICE_NAME) == 0) {
        rt_kprintf("[FS] Format success, mounting again...\n");
        
        if (dfs_mount(FS_DEVICE_NAME, "/", "elm", 0, 0) == 0) {
            rt_kprintf("[FS] Mount success after format\n");
            g_fs_mounted = true;
            return RT_EOK;
        }
    }
    
    rt_kprintf("[FS] Internal flash mount failed!\n");
    return -RT_ERROR;
}

/* ============================================================================
 * TF卡(SD卡)文件系统挂载
 * ============================================================================ */

#ifdef RT_USING_SPI_MSD

/**
 * @brief 等待SD卡设备就绪
 * @return RT_EOK 成功, -RT_ETIMEOUT 超时
 */
static int sdcard_wait_ready(void)
{
    uint16_t timeout = SDCARD_DETECT_TIMEOUT;
    
    rt_kprintf("[SDCARD] Waiting for SD card...\n");
    
    while (timeout--) {
        rt_thread_mdelay(SDCARD_DETECT_DELAY_MS);
        if (rt_device_find(SDCARD_DEVICE_NAME) != RT_NULL) {
            rt_kprintf("[SDCARD] SD card detected\n");
            return RT_EOK;
        }
    }
    
    rt_kprintf("[SDCARD] SD card not found (timeout)\n");
    return -RT_ETIMEOUT;
}

/**
 * @brief 确保挂载目录存在
 * @param path 目录路径
 * @return RT_EOK 成功
 */
static int ensure_mount_dir(const char *path)
{
    struct stat st;
    
    if (stat(path, &st) == 0) {
        return RT_EOK;  /* 目录已存在 */
    }
    
    if (mkdir(path, 0777) == 0) {
        rt_kprintf("[SDCARD] Created mount directory: %s\n", path);
        return RT_EOK;
    }
    
    rt_kprintf("[SDCARD] Failed to create directory: %s\n", path);
    return -RT_ERROR;
}

/**
 * @brief 挂载TF卡文件系统
 * @return RT_EOK 成功, 其他失败
 */
static int fs_mount_sdcard(void)
{
    rt_kprintf("[SDCARD] Initializing SD card...\n");
    
    /* 等待SD卡设备就绪 */
    if (sdcard_wait_ready() != RT_EOK) {
        rt_kprintf("[SDCARD] SD card not available\n");
        return -RT_ENOSYS;
    }
    
    /* 确保根文件系统已挂载 */
    if (!g_fs_mounted) {
        rt_kprintf("[SDCARD] Root filesystem not mounted, skip SD card\n");
        return -RT_ERROR;
    }
    
    /* 创建挂载目录 */
    if (ensure_mount_dir(SDCARD_MOUNT_PATH) != RT_EOK) {
        return -RT_ERROR;
    }
    
    /* 尝试挂载SD卡 */
    if (dfs_mount(SDCARD_DEVICE_NAME, SDCARD_MOUNT_PATH, "elm", 0, 0) == 0) {
        rt_kprintf("[SDCARD] Mount success at %s\n", SDCARD_MOUNT_PATH);
        g_sdcard_mounted = true;
        
        /* 打印SD卡信息 */
        rt_device_t sd_dev = rt_device_find(SDCARD_DEVICE_NAME);
        if (sd_dev && sd_dev->user_data) {
            struct msd_device *msd = (struct msd_device *)sd_dev->user_data;
            uint32_t capacity_mb = (msd->geometry.sector_count * 
                                   msd->geometry.bytes_per_sector) / (1024 * 1024);
            rt_kprintf("[SDCARD] Capacity: %u MB\n", capacity_mb);
            rt_kprintf("[SDCARD] Block size: %u bytes\n", msd->geometry.block_size);
        }
        
        return RT_EOK;
    }
    
    /* 挂载失败，尝试格式化 */
    rt_kprintf("[SDCARD] Mount failed, try formatting...\n");
    
    if (dfs_mkfs("elm", SDCARD_DEVICE_NAME) == 0) {
        rt_kprintf("[SDCARD] Format success, mounting again...\n");
        
        if (dfs_mount(SDCARD_DEVICE_NAME, SDCARD_MOUNT_PATH, "elm", 0, 0) == 0) {
            rt_kprintf("[SDCARD] Mount success after format\n");
            g_sdcard_mounted = true;
            return RT_EOK;
        }
    }
    
    rt_kprintf("[SDCARD] Mount failed!\n");
    return -RT_ERROR;
}

/**
 * @brief 卸载TF卡文件系统
 * @return RT_EOK 成功
 */
int fs_unmount_sdcard(void)
{
    if (!g_sdcard_mounted) {
        return RT_EOK;
    }
    
    if (dfs_unmount(SDCARD_MOUNT_PATH) == 0) {
        rt_kprintf("[SDCARD] Unmounted\n");
        g_sdcard_mounted = false;
        return RT_EOK;
    }
    
    rt_kprintf("[SDCARD] Unmount failed\n");
    return -RT_ERROR;
}

#endif /* RT_USING_SPI_MSD */

/* ============================================================================
 * 公共API
 * ============================================================================ */

/**
 * @brief 初始化并挂载所有文件系统
 * @return RT_EOK 成功, 其他失败
 */
int fs_init(void)
{
    if (g_fs_mounted) {
        return RT_EOK;
    }
    
    rt_kprintf("[FS] Initializing file systems...\n");
    
    /* 1. 挂载内部Flash文件系统 */
    int ret = fs_mount_internal_flash();
    if (ret != RT_EOK) {
        return ret;
    }
    
    /* 2. 初始化配置存储模块 */
    if (g_fs_mounted) {
        ret = fs_config_storage_init();
        if (ret != FS_CFG_OK) {
            rt_kprintf("[FS] Config storage init failed: %d\n", ret);
        }
    }
    
#ifdef RT_USING_SPI_MSD
    /* 3. 挂载TF卡文件系统 (可选，失败不影响系统启动) */
    fs_mount_sdcard();
#endif
    
    rt_kprintf("[FS] File system initialization complete\n");
    return g_fs_mounted ? RT_EOK : -RT_ERROR;
}

/**
 * @brief 检查内部Flash文件系统是否已挂载
 * @return true 已挂载, false 未挂载
 */
bool fs_is_mounted(void)
{
    return g_fs_mounted;
}

/**
 * @brief 检查TF卡是否已挂载
 * @return true 已挂载, false 未挂载
 */
bool fs_is_sdcard_mounted(void)
{
#ifdef RT_USING_SPI_MSD
    return g_sdcard_mounted;
#else
    return false;
#endif
}

/**
 * @brief 获取TF卡挂载路径
 * @return 挂载路径字符串
 */
const char* fs_get_sdcard_path(void)
{
#ifdef RT_USING_SPI_MSD
    return SDCARD_MOUNT_PATH;
#else
    return NULL;
#endif
}

/**
 * @brief 重新挂载TF卡
 * @return RT_EOK 成功
 */
int fs_remount_sdcard(void)
{
#ifdef RT_USING_SPI_MSD
    fs_unmount_sdcard();
    rt_thread_mdelay(100);
    return fs_mount_sdcard();
#else
    return -RT_ENOSYS;
#endif
}

/* 使用 INIT_ENV_EXPORT 确保在环境初始化阶段调用 */
INIT_ENV_EXPORT(fs_init);

/* ============================================================================
 * Shell 调试命令
 * ============================================================================ */

#ifdef RT_USING_FINSH
#include <finsh.h>

void fs_status(void)
{
    rt_kprintf("\n=== File System Status ===\n");
    
    /* 内部Flash状态 */
    rt_kprintf("[Internal Flash]\n");
    rt_kprintf("  Mounted: %s\n", g_fs_mounted ? "YES" : "NO");
    rt_kprintf("  Region: 0x%08X\n", FS_REGION_START_ADDR);
    rt_kprintf("  Size: %d KB\n", FS_REGION_SIZE / 1024);
    rt_kprintf("  Device: %s\n", FS_DEVICE_NAME);
    
#ifdef RT_USING_SPI_MSD
    /* TF卡状态 */
    rt_kprintf("\n[SD Card]\n");
    rt_kprintf("  Mounted: %s\n", g_sdcard_mounted ? "YES" : "NO");
    rt_kprintf("  Mount path: %s\n", SDCARD_MOUNT_PATH);
    rt_kprintf("  Device: %s\n", SDCARD_DEVICE_NAME);
    
    if (g_sdcard_mounted) {
        rt_device_t sd_dev = rt_device_find(SDCARD_DEVICE_NAME);
        if (sd_dev && sd_dev->user_data) {
            struct msd_device *msd = (struct msd_device *)sd_dev->user_data;
            uint32_t capacity_mb = (msd->geometry.sector_count * 
                                   msd->geometry.bytes_per_sector) / (1024 * 1024);
            rt_kprintf("  Capacity: %u MB\n", capacity_mb);
            
            /* 卡类型 */
            rt_kprintf("  Card type: ");
            switch(msd->card_type) {
                case MSD_CARD_TYPE_MMC:      rt_kprintf("MMC\n"); break;
                case MSD_CARD_TYPE_SD_V1_X:  rt_kprintf("SD V1.x\n"); break;
                case MSD_CARD_TYPE_SD_V2_X:  rt_kprintf("SD V2.0\n"); break;
                case MSD_CARD_TYPE_SD_SDHC:  rt_kprintf("SDHC\n"); break;
                default:                      rt_kprintf("Unknown\n");
            }
        }
    }
#else
    rt_kprintf("\n[SD Card] Not enabled (RT_USING_SPI_MSD)\n");
#endif
    
    rt_kprintf("==========================\n");
}
MSH_CMD_EXPORT(fs_status, Show file system status);

#ifdef RT_USING_SPI_MSD
void sd_mount(void)
{
    if (g_sdcard_mounted) {
        rt_kprintf("SD card already mounted\n");
        return;
    }
    fs_mount_sdcard();
}
MSH_CMD_EXPORT(sd_mount, Mount SD card);

void sd_unmount(void)
{
    fs_unmount_sdcard();
}
MSH_CMD_EXPORT(sd_unmount, Unmount SD card);

void sd_remount(void)
{
    fs_remount_sdcard();
}
MSH_CMD_EXPORT(sd_remount, Remount SD card);
#endif

#endif /* RT_USING_FINSH */