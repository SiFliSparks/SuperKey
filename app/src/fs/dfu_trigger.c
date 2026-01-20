/**
 * @file dfu_trigger.c
 * @brief DFU模式触发模块实现（独立版本，不依赖bt_pan_ota）
 * 
 * 本模块直接操作Flash中的DFU标志位，使系统重启后进入DFU升级程序
 */

#include "../fs/dfu_trigger.h"
#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_flash.h"
#include "ptab.h"
#include <string.h>

#define DBG_TAG "dfu_trigger"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/*===========================================================================
 * DFU 相关宏定义和数据结构（与dfu_pan兼容）
 *===========================================================================*/

/* DFU下载区域地址（从ptab.json获取） */
#ifndef DFU_PAN_LOADER_START_ADDR
    #define DFU_PAN_LOADER_START_ADDR   FLASH2_BASE_ADDR + 0x00A78000
#endif

#ifndef DFU_PAN_LOADER_SIZE
    #define DFU_PAN_LOADER_SIZE         0x00200000
#endif

/* 固件信息存储地址（DFU下载区域末尾4KB） */
#define FIRMWARE_INFO_BASE_ADDR     (DFU_PAN_LOADER_START_ADDR + DFU_PAN_LOADER_SIZE - 0x1000)

/* 最大固件文件数 */
#define MAX_FIRMWARE_FILES          3

/* 魔数定义（与dfu_pan兼容） */
#define FIRMWARE_INFO_MAGIC         0x64667500  /* "dfu" */
#define FIRMWARE_INFO_MAGIC_PAN     0x70616E00  /* "pan" */
#define FIRMWARE_MAGIC_DFU_PAN      ((uint32_t)FIRMWARE_INFO_MAGIC << 16 | (FIRMWARE_INFO_MAGIC_PAN & 0xFFFF))

/* 主程序Flash地址（从ptab.json: 0x12000000 + 0x00078000） */
#define MAIN_PROGRAM_ADDR           (FLASH2_BASE_ADDR + 0x00078000)

/**
 * @brief 固件文件信息结构体（与dfu_pan兼容）
 */
struct firmware_file_info
{
    char name[48];          /* 文件名 */
    char url[256];          /* 下载URL */
    uint32_t addr;          /* Flash地址 */
    uint32_t size;          /* 文件大小 */
    uint32_t crc32;         /* CRC32校验 */
    uint32_t region_size;   /* 区域大小 */
    uint32_t file_id;       /* 文件ID */
    uint32_t needs_update;  /* 更新标志(1=需要更新) */
    uint32_t magic;         /* 魔数验证 */
};

/*===========================================================================
 * 内部函数实现
 *===========================================================================*/

/**
 * @brief 设置DFU更新标志位（内部实现）
 */
static int dfu_set_update_flags_internal(void)
{
    struct firmware_file_info firmware_info[MAX_FIRMWARE_FILES];
    int data_size = sizeof(firmware_info);
    
    LOG_I("Setting DFU update flags...");
    LOG_I("Firmware info addr: 0x%08X", FIRMWARE_INFO_BASE_ADDR);
    
    /* 读取现有固件信息 */
    int read_result = rt_flash_read(FIRMWARE_INFO_BASE_ADDR, 
                                    (uint8_t *)firmware_info, data_size);
    if (read_result != data_size) {
        LOG_E("Failed to read firmware info from flash");
        return -1;
    }
    
    /* 标记所有有效条目需要更新 */
    int marked_count = 0;
    for (int i = 0; i < MAX_FIRMWARE_FILES; i++) {
        /* 检查是否为有效条目（非空且非0xFF） */
        if (firmware_info[i].name[0] != '\0' && 
            firmware_info[i].name[0] != 0xFF) {
            firmware_info[i].magic = FIRMWARE_MAGIC_DFU_PAN;
            firmware_info[i].needs_update = 1;
            LOG_I("Marked firmware[%d]: %s", i, firmware_info[i].name);
            marked_count++;
        }
    }
    
    /* 如果没有有效条目，创建DFU触发条目 */
    if (marked_count == 0) {
        LOG_I("No existing entries, creating DFU trigger entry");
        memset(&firmware_info[0], 0, sizeof(struct firmware_file_info));
        strncpy(firmware_info[0].name, "DFU_TRIGGER", sizeof(firmware_info[0].name) - 1);
        firmware_info[0].magic = FIRMWARE_MAGIC_DFU_PAN;
        firmware_info[0].needs_update = 1;
        firmware_info[0].addr = MAIN_PROGRAM_ADDR;
        firmware_info[0].size = 0;
        firmware_info[0].file_id = 0;
        marked_count = 1;
    }
    
    /* 擦除Flash */
    int erase_alignment = rt_flash_get_erase_alignment(FIRMWARE_INFO_BASE_ADDR);
    int aligned_size = ((data_size + erase_alignment - 1) / erase_alignment) * erase_alignment;
    
    LOG_I("Erasing flash at 0x%08X, size=%d", FIRMWARE_INFO_BASE_ADDR, aligned_size);
    if (rt_flash_erase(FIRMWARE_INFO_BASE_ADDR, aligned_size) != RT_EOK) {
        LOG_E("Failed to erase flash");
        return -1;
    }
    
    /* 写入标志位 */
    if (rt_flash_write(FIRMWARE_INFO_BASE_ADDR, (uint8_t *)firmware_info, data_size) != data_size) {
        LOG_E("Failed to write firmware info to flash");
        return -1;
    }
    
    LOG_I("Successfully marked %d entry(s) for DFU update", marked_count);
    return 0;
}

/**
 * @brief 打印固件信息（内部实现）
 */
static void dfu_print_files_internal(void)
{
    struct firmware_file_info firmware_info[MAX_FIRMWARE_FILES];
    int data_size = sizeof(firmware_info);
    
    rt_kprintf("\n======== DFU Firmware Info ========\n");
    rt_kprintf("Info address: 0x%08X\n", FIRMWARE_INFO_BASE_ADDR);
    rt_kprintf("Expected magic: 0x%08X\n", FIRMWARE_MAGIC_DFU_PAN);
    rt_kprintf("-----------------------------------\n");
    
    int read_result = rt_flash_read(FIRMWARE_INFO_BASE_ADDR, 
                                    (uint8_t *)firmware_info, data_size);
    if (read_result != data_size) {
        rt_kprintf("ERROR: Failed to read firmware info\n");
        return;
    }
    
    int valid_count = 0;
    for (int i = 0; i < MAX_FIRMWARE_FILES; i++) {
        if (firmware_info[i].name[0] != '\0' && 
            firmware_info[i].name[0] != 0xFF) {
            rt_kprintf("[%d] Name: %s\n", i, firmware_info[i].name);
            rt_kprintf("    Addr: 0x%08X\n", firmware_info[i].addr);
            rt_kprintf("    Size: %u bytes\n", firmware_info[i].size);
            rt_kprintf("    Magic: 0x%08X %s\n", firmware_info[i].magic,
                      (firmware_info[i].magic == FIRMWARE_MAGIC_DFU_PAN) ? "(valid)" : "(invalid)");
            rt_kprintf("    NeedsUpdate: %u\n", firmware_info[i].needs_update);
            rt_kprintf("-----------------------------------\n");
            valid_count++;
        }
    }
    
    if (valid_count == 0) {
        rt_kprintf("No valid firmware entries found.\n");
    }
    rt_kprintf("===================================\n\n");
}

/*===========================================================================
 * 公开API实现
 *===========================================================================*/

/**
 * @brief 设置DFU更新标志位
 */
int dfu_trigger_set_flags(void)
{
    return dfu_set_update_flags_internal();
}

/**
 * @brief 仅设置DFU标志位，不重启
 */
int dfu_trigger_set_flags_only(void)
{
    int ret = dfu_trigger_set_flags();
    if (ret == 0) {
        LOG_I("Flags set. System will enter DFU mode on next reboot.");
    }
    return ret;
}

/**
 * @brief 进入DFU模式（设置标志位并重启）
 */
void dfu_trigger_enter(void)
{
    LOG_I("Entering DFU mode...");
    
    int ret = dfu_trigger_set_flags();
    if (ret != 0) {
        LOG_E("Failed to set DFU flags, abort!");
        return;
    }
    
    LOG_I("System will reboot to DFU mode in 1 second...");
    rt_thread_mdelay(1000);
    
    LOG_I("Rebooting now...");
    HAL_PMU_Reboot();
    
    /* 不会执行到这里 */
    while(1) {
        rt_thread_mdelay(100);
    }
}

/*===========================================================================
 * MSH 命令
 *===========================================================================*/
#ifdef RT_USING_FINSH
#include <finsh.h>

/**
 * @brief 手动进入DFU模式的命令
 * 
 * 使用方法:
 *   dfu_enter          - 显示帮助（安全提示）
 *   dfu_enter -f       - 强制进入DFU（设置标志+重启）
 *   dfu_enter -s       - 仅设置标志位，不重启
 */
static void dfu_enter(int argc, char **argv)
{
    int force_mode = 0;
    int set_only = 0;
    
    /* 解析参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            force_mode = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            set_only = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            rt_kprintf("Usage: dfu_enter [options]\n");
            rt_kprintf("Options:\n");
            rt_kprintf("  -f    Force enter DFU mode (set flags and reboot)\n");
            rt_kprintf("  -s    Set update flags only, do not reboot\n");
            rt_kprintf("  -h    Show this help message\n");
            return;
        }
    }
    
    /* 非强制模式下提示用户 */
    if (!force_mode && !set_only) {
        rt_kprintf("\n");
        rt_kprintf("========================================\n");
        rt_kprintf("  WARNING: Enter DFU Mode\n");
        rt_kprintf("========================================\n");
        rt_kprintf("This will set the DFU update flags and\n");
        rt_kprintf("reboot the system into DFU mode.\n");
        rt_kprintf("\n");
        rt_kprintf("Use 'dfu_enter -f' to force enter DFU.\n");
        rt_kprintf("Use 'dfu_enter -s' to set flags only.\n");
        rt_kprintf("========================================\n");
        return;
    }
    
    /* 仅设置标志位 */
    if (set_only) {
        dfu_trigger_set_flags_only();
        rt_kprintf("Use 'reboot' command or power cycle to enter DFU.\n");
        return;
    }
    
    /* 强制进入DFU模式 */
    dfu_trigger_enter();
}
MSH_CMD_EXPORT(dfu_enter, Enter DFU mode manually. Use -h for help);

/**
 * @brief 打印当前固件信息
 */
static void dfu_info(int argc, char **argv)
{
    dfu_print_files_internal();
}
MSH_CMD_EXPORT(dfu_info, Print DFU firmware files info);

#endif /* RT_USING_FINSH */