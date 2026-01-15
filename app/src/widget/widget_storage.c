/**
 * @file widget_storage.c
 * @brief 小工具配置存储模块 - 实现（文件系统版本）
 * 
 * 使用文件系统替代直接 Flash 读写
 */

#include "../widget/widget_storage.h"
#include "../fs/fs_config_storage.h"
#include <string.h>
#include <rtthread.h>

/* ============================================================================
 * 私有变量
 * ============================================================================ */

static widget_config_t g_active_widget = {0};
static rt_mutex_t g_storage_mutex = RT_NULL;
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
 * 默认配置
 * ============================================================================ */

static void init_default_config(void)
{
    memset(&g_active_widget, 0, sizeof(g_active_widget));
    
    /* 默认选择随机数工具 */
    g_active_widget.type = WIDGET_TYPE_RANDOM_NUMBER;
    g_active_widget.config.random.max_option = RANDOM_MAX_100;
}

/* ============================================================================
 * 公共 API 实现
 * ============================================================================ */

int widget_storage_init(void)
{
    if (g_initialized) {
        return 0;
    }
    
    /* 创建互斥锁 */
    if (g_storage_mutex == RT_NULL) {
        g_storage_mutex = rt_mutex_create("wdg_mtx", RT_IPC_FLAG_PRIO);
        if (g_storage_mutex == RT_NULL) {
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
    if (widget_storage_load() != 0) {
        init_default_config();
        /* 首次保存默认配置 */
        widget_storage_save();
    }
    g_initialized = true;
    
    return 0;
}

int widget_storage_load(void)
{
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    widget_config_t temp_config;
    uint32_t version;
    
    int ret = fs_config_load(WIDGET_CONFIG_FILE, 
                             WIDGET_STORAGE_MAGIC,
                             &temp_config, 
                             sizeof(widget_config_t),
                             &version);
    
    if (ret == FS_CFG_OK) {
        /* 验证数据有效性 */
        if (temp_config.type < WIDGET_TYPE_MAX) {
            g_active_widget = temp_config;
        } else {
            ret = -1;
        }
    }
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
    return ret;
}

int widget_storage_save(void)
{
    if (!fs_config_is_ready()) {
        return -RT_ERROR;
    }
    
    if (g_storage_mutex) {
        rt_mutex_take(g_storage_mutex, RT_WAITING_FOREVER);
    }
    
    int ret = fs_config_save(WIDGET_CONFIG_FILE,
                             WIDGET_STORAGE_MAGIC,
                             WIDGET_STORAGE_VERSION,
                             &g_active_widget,
                             sizeof(widget_config_t));
    
    if (ret != FS_CFG_OK) {
    }
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
    return (ret == FS_CFG_OK) ? 0 : -RT_ERROR;
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
    
    g_active_widget = *config;
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
    /* 保存到文件系统 */
    return widget_storage_save();
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
    
    *config = g_active_widget;
    
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
    
    init_default_config();
    
    if (g_storage_mutex) {
        rt_mutex_release(g_storage_mutex);
    }
    
    /* 删除旧配置文件并保存新的 */
    fs_config_delete(WIDGET_CONFIG_FILE);
    widget_storage_save();
}

