/**
 * @file widget_storage.c
 * @brief 小工具配置Flash存储模块 - 实现
 */

#include "widget_storage.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>

/* Flash存储地址 - 需要根据实际硬件调整 */
#define WIDGET_FLASH_ADDR       (0x12CAC000)
#define WIDGET_FLASH_SIZE       (4096)

/* 访问模式: 0=RAM only, 1=rt_flash, 2=HAL直接 */
#define FLASH_ACCESS_MODE       1

/* 静态存储 */
static widget_storage_t g_widget_storage = {0};
static rt_mutex_t g_storage_mutex = NULL;
static bool g_initialized = false;

/* 小工具类型名称 */
static const char* widget_type_names[] = {
    [WIDGET_TYPE_NONE] = "未选择",
    [WIDGET_TYPE_RANDOM_NUMBER] = "随机数",
    [WIDGET_TYPE_OFF_WORK_COUNTDOWN] = "下班倒计时",
    [WIDGET_TYPE_WHAT_TO_EAT] = "今天吃什么"
};

/* 随机数最大值表 */
static const int random_max_values[] = {
    [RANDOM_MAX_10] = 10,
    [RANDOM_MAX_100] = 100,
    [RANDOM_MAX_1000] = 1000
};

/* ============================================================================
 * CRC32 计算
 * ============================================================================ */

static uint32_t calculate_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

static void update_storage_crc(widget_storage_t *storage)
{
    size_t crc_offset = offsetof(widget_storage_t, crc);
    storage->crc = calculate_crc32((const uint8_t *)storage, crc_offset);
}

static bool validate_storage(const widget_storage_t *storage)
{
    if (!storage || storage->magic != WIDGET_STORAGE_MAGIC) {
        return false;
    }
    size_t crc_offset = offsetof(widget_storage_t, crc);
    return calculate_crc32((const uint8_t *)storage, crc_offset) == storage->crc;
}

static void init_default_storage(void)
{
    memset(&g_widget_storage, 0, sizeof(g_widget_storage));
    g_widget_storage.magic = WIDGET_STORAGE_MAGIC;
    g_widget_storage.version = WIDGET_STORAGE_VERSION;
    
    /* 默认选择随机数工具 */
    g_widget_storage.active_widget.type = WIDGET_TYPE_RANDOM_NUMBER;
    g_widget_storage.active_widget.config.random.max_option = RANDOM_MAX_100;
    
    update_storage_crc(&g_widget_storage);
}

/* ============================================================================
 * 公共API
 * ============================================================================ */

int widget_storage_init(void)
{
    if (g_initialized) {
        return 0;
    }
    
    rt_kprintf("[Widget] Init mode=%d addr=0x%08X\n", FLASH_ACCESS_MODE, WIDGET_FLASH_ADDR);
    
    if (!g_storage_mutex) {
        g_storage_mutex = rt_mutex_create("wdg_mtx", RT_IPC_FLAG_PRIO);
        if (!g_storage_mutex) {
            return -RT_ENOMEM;
        }
    }
    
#if FLASH_ACCESS_MODE > 0
    if (widget_storage_load() != 0)
#endif
    {
        init_default_storage();
    }
    
    g_initialized = true;
    rt_kprintf("[Widget] Ready, active=%s\n", 
               widget_get_type_name(g_widget_storage.active_widget.type));
    return 0;
}

int widget_storage_load(void)
{
#if FLASH_ACCESS_MODE == 0
    return -1;
#else
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    widget_storage_t *flash = (widget_storage_t *)WIDGET_FLASH_ADDR;
    int ret = -1;
    
    if (validate_storage(flash)) {
        memcpy(&g_widget_storage, flash, sizeof(g_widget_storage));
        rt_kprintf("[Widget] Loaded from Flash\n");
        ret = 0;
    }
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    return ret;
#endif
}

int widget_storage_save(void)
{
#if FLASH_ACCESS_MODE == 0
    rt_kprintf("[Widget] RAM mode - not persisted\n");
    return 0;
#else
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    int ret = 0;
    update_storage_crc(&g_widget_storage);
    
    rt_kprintf("[Widget] Saving...\n");
    
    #if FLASH_ACCESS_MODE == 1
    extern int rt_flash_erase(uint32_t addr, size_t size);
    extern int rt_flash_write(uint32_t addr, const uint8_t *buf, size_t size);
    ret = rt_flash_erase(WIDGET_FLASH_ADDR, WIDGET_FLASH_SIZE);
    if (ret == 0) {
        ret = rt_flash_write(WIDGET_FLASH_ADDR, (uint8_t*)&g_widget_storage, sizeof(g_widget_storage));
    }
    #elif FLASH_ACCESS_MODE == 2
    ret = hal_flash_erase(WIDGET_FLASH_ADDR, WIDGET_FLASH_SIZE);
    if (ret == 0) {
        ret = hal_flash_write(WIDGET_FLASH_ADDR, (uint8_t*)&g_widget_storage, sizeof(g_widget_storage));
    }
    #endif
    
    if (ret != 0) {
        rt_kprintf("[Widget] Save failed: %d\n", ret);
    } else {
        rt_kprintf("[Widget] Saved OK\n");
    }
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    return ret;
#endif
}

int widget_storage_set_active(const widget_config_t *config)
{
    if (!config) {
        return -RT_EINVAL;
    }
    
    if (config->type >= WIDGET_TYPE_MAX) {
        return -RT_EINVAL;
    }
    
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    g_widget_storage.active_widget = *config;
    
    rt_kprintf("[Widget] Set active: %s\n", widget_get_type_name(config->type));
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
#if FLASH_ACCESS_MODE > 0
    return widget_storage_save();
#else
    return 0;
#endif
}

int widget_storage_get_active(widget_config_t *config)
{
    if (!config) {
        return -RT_EINVAL;
    }
    
    if (!g_initialized) {
        return -RT_ERROR;
    }
    
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    *config = g_widget_storage.active_widget;
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
    return 0;
}

const char* widget_get_type_name(widget_type_t type)
{
    if (type >= WIDGET_TYPE_MAX) {
        return "未知";
    }
    return widget_type_names[type];
}

int widget_get_random_max_value(random_max_option_t option)
{
    if (option >= RANDOM_MAX_OPTIONS) {
        return 100;
    }
    return random_max_values[option];
}

void widget_storage_reset(void)
{
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    init_default_storage();
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
#if FLASH_ACCESS_MODE > 0
    widget_storage_save();
#endif
}

bool widget_storage_is_valid(void)
{
    return g_initialized && (g_widget_storage.magic == WIDGET_STORAGE_MAGIC);
}

void widget_storage_dump(void)
{
    rt_kprintf("\n=== Widget Storage (mode=%d) ===\n", FLASH_ACCESS_MODE);
    rt_kprintf("Active: %s\n", widget_get_type_name(g_widget_storage.active_widget.type));
    
    switch (g_widget_storage.active_widget.type) {
        case WIDGET_TYPE_RANDOM_NUMBER:
            rt_kprintf("  Max: %d\n", 
                widget_get_random_max_value(g_widget_storage.active_widget.config.random.max_option));
            break;
        case WIDGET_TYPE_OFF_WORK_COUNTDOWN:
            rt_kprintf("  Target: %02d:%02d\n",
                g_widget_storage.active_widget.config.offwork.target_hour,
                g_widget_storage.active_widget.config.offwork.target_minute);
            break;
        default:
            break;
    }
    rt_kprintf("================================\n");
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void wdg_dump(void) { widget_storage_dump(); }
void wdg_reset(void) { widget_storage_reset(); }
MSH_CMD_EXPORT(wdg_dump, Dump widget config);
MSH_CMD_EXPORT(wdg_reset, Reset widget config);
#endif
