#include "custom_key_storage.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>
#include "bf0_hal.h"


#define CUSTOM_KEY_FLASH_ADDR       (0x12CA8000)
#define CUSTOM_KEY_FLASH_SIZE       (4096)
#define FLASH_SECTOR_SIZE           (4096)

/* 访问模式: 0=RAM only, 1=rt_flash, 2=HAL直接 */
#define FLASH_ACCESS_MODE           1   



static custom_key_config_t g_config = {0};
static rt_mutex_t g_config_mutex = NULL;
static bool g_initialized = false;

/* ============================================================================
 * CRC32
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

static void update_config_crc(custom_key_config_t *config)
{
    size_t crc_offset = offsetof(custom_key_config_t, crc);
    config->crc = calculate_crc32((const uint8_t *)config, crc_offset);
}

static bool validate_config(const custom_key_config_t *config)
{
    if (!config || config->magic != CUSTOM_KEY_MAGIC) return false;
    size_t crc_offset = offsetof(custom_key_config_t, crc);
    return calculate_crc32((const uint8_t *)config, crc_offset) == config->crc;
}

static void init_default_config(void)
{
    memset(&g_config, 0, sizeof(g_config));
    g_config.magic = CUSTOM_KEY_MAGIC;
    g_config.version = CUSTOM_KEY_VERSION;
    
    for (int g = 0; g < CUSTOM_KEY_GROUP_COUNT; g++) {
        for (int k = 0; k < CUSTOM_KEY_PER_GROUP; k++) {
            g_config.groups[g].keys[k].enabled = false;
            g_config.groups[g].keys[k].combo_count = 0;
        }
    }
    update_config_crc(&g_config);
}

/* ============================================================================
 * 公共API
 * ============================================================================ */

int custom_key_storage_init(void)
{
    if (g_initialized) return 0;
    
    rt_kprintf("[CKey] Init mode=%d addr=0x%08X\n", FLASH_ACCESS_MODE, CUSTOM_KEY_FLASH_ADDR);
    
    if (!g_config_mutex) {
        g_config_mutex = rt_mutex_create("ck_mtx", RT_IPC_FLAG_PRIO);
        if (!g_config_mutex) return -RT_ENOMEM;
    }
    
#if FLASH_ACCESS_MODE > 0
    if (custom_key_load_from_flash() != 0)
#endif
    {
        init_default_config();
    }
    
    g_initialized = true;
    rt_kprintf("[CKey] Ready\n");
    return 0;
}

int custom_key_load_from_flash(void)
{
#if FLASH_ACCESS_MODE == 0
    return -1;
#else
    if (g_config_mutex) rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    
    custom_key_config_t *flash = (custom_key_config_t *)CUSTOM_KEY_FLASH_ADDR;
    int ret = -1;
    
    if (validate_config(flash)) {
        memcpy(&g_config, flash, sizeof(g_config));
        rt_kprintf("[CKey] Loaded from Flash\n");
        ret = 0;
    }
    
    if (g_config_mutex) rt_mutex_release(g_config_mutex);
    return ret;
#endif
}

int custom_key_save_to_flash(void)
{
#if FLASH_ACCESS_MODE == 0
    rt_kprintf("[CKey] RAM mode - config not persisted\n");
    return 0;
#else
    if (g_config_mutex) rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    
    int ret = 0;
    update_config_crc(&g_config);
    
    rt_kprintf("[CKey] Saving...\n");
    
    #if FLASH_ACCESS_MODE == 1
    extern int rt_flash_erase(uint32_t addr, size_t size);
    extern int rt_flash_write(uint32_t addr, const uint8_t *buf, size_t size);
    ret = rt_flash_erase(CUSTOM_KEY_FLASH_ADDR, CUSTOM_KEY_FLASH_SIZE);
    if (ret == 0) {
        ret = rt_flash_write(CUSTOM_KEY_FLASH_ADDR, (uint8_t*)&g_config, sizeof(g_config));
    }
    #elif FLASH_ACCESS_MODE == 2
    ret = hal_flash_erase(CUSTOM_KEY_FLASH_ADDR, CUSTOM_KEY_FLASH_SIZE);
    if (ret == 0) {
        ret = hal_flash_write(CUSTOM_KEY_FLASH_ADDR, (uint8_t*)&g_config, sizeof(g_config));
    }
    #endif
    
    if (ret != 0) {
        rt_kprintf("[CKey] Save failed: %d\n", ret);
    } else {
        rt_kprintf("[CKey] Saved OK\n");
    }
    
    if (g_config_mutex) rt_mutex_release(g_config_mutex);
    return ret;
#endif
}

int custom_key_set(uint8_t group, uint8_t key_idx, const custom_key_t *key_config)
{
    if (!key_config) return -RT_EINVAL;
    if (group >= CUSTOM_KEY_GROUP_COUNT || key_idx >= CUSTOM_KEY_PER_GROUP) {
        rt_kprintf("[CKey] Invalid g=%d k=%d\n", group, key_idx);
        return -RT_EINVAL;
    }
    
    if (g_config_mutex) rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    
    g_config.groups[group].keys[key_idx] = *key_config;
    
    rt_kprintf("[CKey] Set g%d.k%d en=%d cnt=%d", 
               group, key_idx, key_config->enabled, key_config->combo_count);
    if (key_config->combo_count > 0) {
        rt_kprintf(" [0x%02X+0x%02X]", 
                   key_config->combos[0].modifier, key_config->combos[0].keycode);
    }
    rt_kprintf("\n");
    
    if (g_config_mutex) rt_mutex_release(g_config_mutex);
    
    /* RAM模式下不保存到Flash */
#if FLASH_ACCESS_MODE > 0
    return custom_key_save_to_flash();
#else
    return 0;
#endif
}

int custom_key_get(uint8_t group, uint8_t key_idx, custom_key_t *key_config)
{
    if (!key_config) return -RT_EINVAL;
    if (group >= CUSTOM_KEY_GROUP_COUNT || key_idx >= CUSTOM_KEY_PER_GROUP) return -RT_EINVAL;
    if (!g_initialized) return -RT_ERROR;
    
    if (g_config_mutex) rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    *key_config = g_config.groups[group].keys[key_idx];
    if (g_config_mutex) rt_mutex_release(g_config_mutex);
    
    return 0;
}

int custom_key_parse_and_set(const char *value)
{
    if (!value) return -RT_EINVAL;
    
    rt_kprintf("[CKey] Parse: \"%s\"\n", value);
    
    unsigned int group, key_idx;
    int mod[CUSTOM_KEY_COMBO_MAX] = {0};
    int keycode[CUSTOM_KEY_COMBO_MAX] = {0};
    
    int n = sscanf(value, "%u,%u,%i,%i,%i,%i,%i,%i,%i,%i",
                   &group, &key_idx,
                   &mod[0], &keycode[0], &mod[1], &keycode[1],
                   &mod[2], &keycode[2], &mod[3], &keycode[3]);
    
    rt_kprintf("[CKey] n=%d g=%u k=%u m=0x%X c=0x%X\n", n, group, key_idx, mod[0], keycode[0]);
    
    if (n < 4) return -RT_EINVAL;
    if (group >= CUSTOM_KEY_GROUP_COUNT || key_idx >= CUSTOM_KEY_PER_GROUP) return -RT_EINVAL;
    
    custom_key_t cfg = {0};
    cfg.enabled = true;
    
    for (int i = 0; i < CUSTOM_KEY_COMBO_MAX && keycode[i] != 0; i++) {
        cfg.combos[cfg.combo_count].modifier = (uint8_t)mod[i];
        cfg.combos[cfg.combo_count].keycode = (uint8_t)keycode[i];
        cfg.combo_count++;
    }
    
    if (cfg.combo_count == 0) cfg.enabled = false;
    
    return custom_key_set(group, key_idx, &cfg);
}

const custom_key_config_t* custom_key_get_config(void) { return &g_config; }

void custom_key_reset_to_default(void)
{
    if (g_config_mutex) rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    init_default_config();
    if (g_config_mutex) rt_mutex_release(g_config_mutex);
    
#if FLASH_ACCESS_MODE > 0
    custom_key_save_to_flash();
#endif
}

bool custom_key_is_valid(void)
{
    return g_initialized && (g_config.magic == CUSTOM_KEY_MAGIC);
}

/* ============================================================================
 * 调试
 * ============================================================================ */

void custom_key_dump_config(void)
{
    rt_kprintf("\n=== Custom Key (mode=%d) ===\n", FLASH_ACCESS_MODE);
    
    int cnt = 0;
    for (int g = 0; g < CUSTOM_KEY_GROUP_COUNT; g++) {
        for (int k = 0; k < CUSTOM_KEY_PER_GROUP; k++) {
            custom_key_t *key = &g_config.groups[g].keys[k];
            if (key->enabled && key->combo_count > 0) {
                cnt++;
                rt_kprintf("G%d.K%d:", g, k);
                for (int c = 0; c < key->combo_count; c++) {
                    rt_kprintf(" [0x%02X+0x%02X]", key->combos[c].modifier, key->combos[c].keycode);
                }
                rt_kprintf("\n");
            }
        }
    }
    if (cnt == 0) rt_kprintf("(empty)\n");
    rt_kprintf("===========================\n");
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void ck_dump(void) { custom_key_dump_config(); }
void ck_reset(void) { custom_key_reset_to_default(); }
void ck_test(void) { custom_key_parse_and_set("0,0,0x01,0x04,0,0,0,0,0,0"); ck_dump(); }
MSH_CMD_EXPORT(ck_dump, Dump custom keys);
MSH_CMD_EXPORT(ck_reset, Reset custom keys);
MSH_CMD_EXPORT(ck_test, Test Ctrl+A);
#endif