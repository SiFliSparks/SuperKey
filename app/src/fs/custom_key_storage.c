/**
 * @file custom_key_storage.c
 * @brief 自定义按键存储模块 - 实现（文件系统版本）
 * 
 * 使用文件系统替代直接 Flash 读写
 */

#include "../fs/custom_key_storage.h"
#include "../fs/fs_config_storage.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>

/* ============================================================================
 * 私有变量
 * ============================================================================ */

static custom_key_data_t g_key_data = {0};
static rt_mutex_t g_config_mutex = RT_NULL;
static bool g_initialized = false;

/* ============================================================================
 * 默认配置
 * ============================================================================ */

static void init_default_config(void)
{
    memset(&g_key_data, 0, sizeof(g_key_data));
    
    for (int g = 0; g < CUSTOM_KEY_GROUP_COUNT; g++) {
        for (int k = 0; k < CUSTOM_KEY_PER_GROUP; k++) {
            g_key_data.groups[g].keys[k].enabled = false;
            g_key_data.groups[g].keys[k].combo_count = 0;
        }
    }
}

/* ============================================================================
 * 公共 API 实现
 * ============================================================================ */

int custom_key_storage_init(void)
{
    if (g_initialized) {
        return 0;
    }
    
    /* 创建互斥锁 */
    if (g_config_mutex == RT_NULL) {
        g_config_mutex = rt_mutex_create("ck_mtx", RT_IPC_FLAG_PRIO);
        if (g_config_mutex == RT_NULL) {
            return -RT_ENOMEM;
        }
    }
    
    /* 检查文件系统是否就绪 */
    if (!fs_config_is_ready()) {
        /* 尝试初始化文件系统配置模块 */
        if (fs_config_storage_init() != FS_CFG_OK) {
            init_default_config();
            g_initialized = true;
            return 0;
        }
    }
    
    /* 尝试从文件加载配置 */
    if (custom_key_load_from_flash() != 0) {
        init_default_config();
    }
    
    g_initialized = true;
    rt_kprintf("[CKey] Ready\n");
    
    return 0;
}

int custom_key_load_from_flash(void)
{
    if (g_config_mutex) {
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    }
    
    custom_key_data_t temp_data;
    uint32_t version;
    
    int ret = fs_config_load(CUSTOM_KEY_CONFIG_FILE,
                             CUSTOM_KEY_MAGIC,
                             &temp_data,
                             sizeof(custom_key_data_t),
                             &version);
    
    if (ret == FS_CFG_OK) {
        g_key_data = temp_data;
    }
    
    if (g_config_mutex) {
        rt_mutex_release(g_config_mutex);
    }
    
    return (ret == FS_CFG_OK) ? 0 : -1;
}

int custom_key_save_to_flash(void)
{
    if (!fs_config_is_ready()) {
        return -RT_ERROR;
    }
    
    if (g_config_mutex) {
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    }
    
    int ret = fs_config_save(CUSTOM_KEY_CONFIG_FILE,
                             CUSTOM_KEY_MAGIC,
                             CUSTOM_KEY_VERSION,
                             &g_key_data,
                             sizeof(custom_key_data_t));
    
    if (ret != FS_CFG_OK) {
        rt_kprintf("[CKey] Save failed: %d\n", ret);
    }
    
    if (g_config_mutex) {
        rt_mutex_release(g_config_mutex);
    }
    
    return (ret == FS_CFG_OK) ? 0 : -RT_ERROR;
}

int custom_key_set(uint8_t group, uint8_t key_idx, const custom_key_t *key_config)
{
    if (!key_config) {
        return -RT_EINVAL;
    }
    
    if (group >= CUSTOM_KEY_GROUP_COUNT || key_idx >= CUSTOM_KEY_PER_GROUP) {
        rt_kprintf("[CKey] Invalid g=%d k=%d\n", group, key_idx);
        return -RT_EINVAL;
    }
    
    if (g_config_mutex) {
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    }
    
    g_key_data.groups[group].keys[key_idx] = *key_config;
    
    rt_kprintf("[CKey] Set g%d.k%d en=%d cnt=%d", 
               group, key_idx, key_config->enabled, key_config->combo_count);
    if (key_config->combo_count > 0) {
        rt_kprintf(" [0x%02X+0x%02X]", 
                   key_config->combos[0].modifier, key_config->combos[0].keycode);
    }
    rt_kprintf("\n");
    
    if (g_config_mutex) {
        rt_mutex_release(g_config_mutex);
    }
    
    /* 保存到文件系统 */
    return custom_key_save_to_flash();
}

int custom_key_get(uint8_t group, uint8_t key_idx, custom_key_t *key_config)
{
    if (!key_config) {
        return -RT_EINVAL;
    }
    
    if (group >= CUSTOM_KEY_GROUP_COUNT || key_idx >= CUSTOM_KEY_PER_GROUP) {
        return -RT_EINVAL;
    }
    
    if (!g_initialized) {
        return -RT_ERROR;
    }
    
    if (g_config_mutex) {
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    }
    
    *key_config = g_key_data.groups[group].keys[key_idx];
    
    if (g_config_mutex) {
        rt_mutex_release(g_config_mutex);
    }
    
    return 0;
}

int custom_key_parse_and_set(const char *value)
{
    if (!value) {
        return -RT_EINVAL;
    }
    
    rt_kprintf("[CKey] Parse: \"%s\"\n", value);
    
    unsigned int group, key_idx;
    int mod[CUSTOM_KEY_COMBO_MAX] = {0};
    int keycode[CUSTOM_KEY_COMBO_MAX] = {0};
    
    int n = sscanf(value, "%u,%u,%i,%i,%i,%i,%i,%i,%i,%i",
                   &group, &key_idx,
                   &mod[0], &keycode[0], &mod[1], &keycode[1],
                   &mod[2], &keycode[2], &mod[3], &keycode[3]);
    
    rt_kprintf("[CKey] n=%d g=%u k=%u m=0x%X c=0x%X\n", n, group, key_idx, mod[0], keycode[0]);
    
    if (n < 4) {
        return -RT_EINVAL;
    }
    
    if (group >= CUSTOM_KEY_GROUP_COUNT || key_idx >= CUSTOM_KEY_PER_GROUP) {
        return -RT_EINVAL;
    }
    
    custom_key_t cfg = {0};
    cfg.enabled = true;
    
    for (int i = 0; i < CUSTOM_KEY_COMBO_MAX && keycode[i] != 0; i++) {
        cfg.combos[cfg.combo_count].modifier = (uint8_t)mod[i];
        cfg.combos[cfg.combo_count].keycode = (uint8_t)keycode[i];
        cfg.combo_count++;
    }
    
    if (cfg.combo_count == 0) {
        cfg.enabled = false;
    }
    
    return custom_key_set(group, key_idx, &cfg);
}

const custom_key_data_t* custom_key_get_data(void)
{
    return &g_key_data;
}

void custom_key_reset_to_default(void)
{
    if (g_config_mutex) {
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    }
    
    init_default_config();
    
    if (g_config_mutex) {
        rt_mutex_release(g_config_mutex);
    }
    
    /* 删除旧配置文件并保存新的 */
    fs_config_delete(CUSTOM_KEY_CONFIG_FILE);
    custom_key_save_to_flash();
}

bool custom_key_is_valid(void)
{
    return g_initialized;
}


