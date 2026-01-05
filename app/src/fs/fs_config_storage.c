/**
 * @file fs_config_storage.c
 * @brief 基于文件系统的配置存储模块 - 实现
 * 
 * 使用 FatFs (ELM) 文件系统进行配置持久化存储
 */

#include "../fs/fs_config_storage.h"
#include <string.h>
#include <rtthread.h>
#include <dfs_file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* ============================================================================
 * 文件头结构（统一格式）
 * ============================================================================ */

typedef struct {
    uint32_t magic;         /* 魔数标识 */
    uint32_t version;       /* 版本号 */
    uint32_t data_size;     /* 数据大小 */
    uint32_t crc;           /* CRC32校验 */
} config_file_header_t;

/* ============================================================================
 * 私有变量
 * ============================================================================ */

static bool g_fs_ready = false;
static rt_mutex_t g_fs_mutex = RT_NULL;

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

/* ============================================================================
 * 目录创建辅助函数
 * ============================================================================ */

static int ensure_directory_exists(const char *dirpath)
{
    struct stat st;
    
    if (stat(dirpath, &st) == 0) {
        /* 目录已存在 */
        return FS_CFG_OK;
    }
    
    /* 创建目录 */
    if (mkdir(dirpath, 0777) == 0) {
        rt_kprintf("[FS_CFG] Created directory: %s\n", dirpath);
        return FS_CFG_OK;
    }
    
    rt_kprintf("[FS_CFG] Failed to create directory: %s\n", dirpath);
    return FS_CFG_ERR_WRITE;
}

/* ============================================================================
 * 公共 API 实现
 * ============================================================================ */

int fs_config_storage_init(void)
{
    if (g_fs_ready) {
        return FS_CFG_OK;
    }
    
    /* 创建互斥锁 */
    if (g_fs_mutex == RT_NULL) {
        g_fs_mutex = rt_mutex_create("fs_cfg", RT_IPC_FLAG_PRIO);
        if (g_fs_mutex == RT_NULL) {
            rt_kprintf("[FS_CFG] Failed to create mutex\n");
            return FS_CFG_ERR_NOT_INIT;
        }
    }
    
    /* 检查根目录是否可访问（文件系统是否已挂载） */
    struct stat st;
    if (stat("/", &st) != 0) {
        rt_kprintf("[FS_CFG] File system not mounted\n");
        return FS_CFG_ERR_NO_FS;
    }
    
    /* 确保配置目录存在 */
    int ret = ensure_directory_exists(CONFIG_DIR);
    if (ret != FS_CFG_OK) {
        return ret;
    }
    
    g_fs_ready = true;
    rt_kprintf("[FS_CFG] Initialized, config dir: %s\n", CONFIG_DIR);
    
    return FS_CFG_OK;
}

bool fs_config_is_ready(void)
{
    return g_fs_ready;
}

int fs_config_save(const char *filepath, uint32_t magic, uint32_t version,
                   const void *data, size_t data_size)
{
    if (!g_fs_ready) {
        return FS_CFG_ERR_NOT_INIT;
    }
    
    if (!filepath || !data || data_size == 0) {
        return FS_CFG_ERR_PARAM;
    }
    
    rt_mutex_take(g_fs_mutex, RT_WAITING_FOREVER);
    
    int ret = FS_CFG_OK;
    int fd = -1;
    
    /* 准备文件头 */
    config_file_header_t header = {
        .magic = magic,
        .version = version,
        .data_size = data_size,
        .crc = calculate_crc32((const uint8_t *)data, data_size)
    };
    
    /* 打开文件（创建或截断） */
    fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0);
    if (fd < 0) {
        rt_kprintf("[FS_CFG] Failed to open file for write: %s\n", filepath);
        ret = FS_CFG_ERR_OPEN;
        goto exit;
    }
    
    /* 写入头部 */
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        rt_kprintf("[FS_CFG] Failed to write header\n");
        ret = FS_CFG_ERR_WRITE;
        goto exit;
    }
    
    /* 写入数据 */
    if (write(fd, data, data_size) != (int)data_size) {
        rt_kprintf("[FS_CFG] Failed to write data\n");
        ret = FS_CFG_ERR_WRITE;
        goto exit;
    }
    
    rt_kprintf("[FS_CFG] Saved %s (magic=0x%08X, ver=%u, size=%u)\n",
               filepath, magic, version, data_size);

exit:
    if (fd >= 0) {
        close(fd);
    }
    rt_mutex_release(g_fs_mutex);
    return ret;
}

int fs_config_load(const char *filepath, uint32_t expected_magic,
                   void *data, size_t data_size, uint32_t *version)
{
    if (!g_fs_ready) {
        return FS_CFG_ERR_NOT_INIT;
    }
    
    if (!filepath || !data || data_size == 0) {
        return FS_CFG_ERR_PARAM;
    }
    
    rt_mutex_take(g_fs_mutex, RT_WAITING_FOREVER);
    
    int ret = FS_CFG_OK;
    int fd = -1;
    config_file_header_t header;
    
    /* 打开文件 */
    fd = open(filepath, O_RDONLY, 0);
    if (fd < 0) {
        rt_kprintf("[FS_CFG] File not found: %s\n", filepath);
        ret = FS_CFG_ERR_OPEN;
        goto exit;
    }
    
    /* 读取头部 */
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        rt_kprintf("[FS_CFG] Failed to read header\n");
        ret = FS_CFG_ERR_READ;
        goto exit;
    }
    
    /* 验证魔数 */
    if (header.magic != expected_magic) {
        rt_kprintf("[FS_CFG] Magic mismatch: expected 0x%08X, got 0x%08X\n",
                   expected_magic, header.magic);
        ret = FS_CFG_ERR_MAGIC;
        goto exit;
    }
    
    /* 验证数据大小 */
    if (header.data_size != data_size) {
        rt_kprintf("[FS_CFG] Size mismatch: expected %u, got %u\n",
                   data_size, header.data_size);
        ret = FS_CFG_ERR_READ;
        goto exit;
    }
    
    /* 读取数据 */
    if (read(fd, data, data_size) != (int)data_size) {
        rt_kprintf("[FS_CFG] Failed to read data\n");
        ret = FS_CFG_ERR_READ;
        goto exit;
    }
    
    /* 验证 CRC */
    uint32_t calc_crc = calculate_crc32((const uint8_t *)data, data_size);
    if (calc_crc != header.crc) {
        rt_kprintf("[FS_CFG] CRC mismatch: expected 0x%08X, got 0x%08X\n",
                   header.crc, calc_crc);
        ret = FS_CFG_ERR_CRC;
        goto exit;
    }
    
    /* 返回版本号 */
    if (version) {
        *version = header.version;
    }
    
    rt_kprintf("[FS_CFG] Loaded %s (ver=%u)\n", filepath, header.version);

exit:
    if (fd >= 0) {
        close(fd);
    }
    rt_mutex_release(g_fs_mutex);
    return ret;
}

int fs_config_delete(const char *filepath)
{
    if (!g_fs_ready) {
        return FS_CFG_ERR_NOT_INIT;
    }
    
    if (!filepath) {
        return FS_CFG_ERR_PARAM;
    }
    
    rt_mutex_take(g_fs_mutex, RT_WAITING_FOREVER);
    
    int ret = FS_CFG_OK;
    if (unlink(filepath) != 0) {
        rt_kprintf("[FS_CFG] Failed to delete: %s\n", filepath);
        ret = FS_CFG_ERR_WRITE;
    } else {
        rt_kprintf("[FS_CFG] Deleted: %s\n", filepath);
    }
    
    rt_mutex_release(g_fs_mutex);
    return ret;
}

bool fs_config_exists(const char *filepath)
{
    if (!filepath) {
        return false;
    }
    
    struct stat st;
    return (stat(filepath, &st) == 0);
}

int fs_config_get_info(const char *filepath, size_t *size, 
                       uint32_t *magic, uint32_t *version)
{
    if (!g_fs_ready) {
        return FS_CFG_ERR_NOT_INIT;
    }
    
    if (!filepath) {
        return FS_CFG_ERR_PARAM;
    }
    
    rt_mutex_take(g_fs_mutex, RT_WAITING_FOREVER);
    
    int ret = FS_CFG_OK;
    int fd = -1;
    config_file_header_t header;
    
    fd = open(filepath, O_RDONLY, 0);
    if (fd < 0) {
        ret = FS_CFG_ERR_OPEN;
        goto exit;
    }
    
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        ret = FS_CFG_ERR_READ;
        goto exit;
    }
    
    if (size) *size = header.data_size;
    if (magic) *magic = header.magic;
    if (version) *version = header.version;

exit:
    if (fd >= 0) {
        close(fd);
    }
    rt_mutex_release(g_fs_mutex);
    return ret;
}

/* ============================================================================
 * Shell 调试命令
 * ============================================================================ */

#ifdef RT_USING_FINSH
#include <finsh.h>

void fs_cfg_info(void)
{
    rt_kprintf("\n=== FS Config Storage ===\n");
    rt_kprintf("Ready: %s\n", g_fs_ready ? "YES" : "NO");
    rt_kprintf("Config dir: %s\n", CONFIG_DIR);
    
    /* 列出配置文件 */
    const char *files[] = {WIDGET_CONFIG_FILE, CUSTOM_KEY_CONFIG_FILE};
    for (int i = 0; i < sizeof(files)/sizeof(files[0]); i++) {
        if (fs_config_exists(files[i])) {
            size_t size;
            uint32_t magic, version;
            if (fs_config_get_info(files[i], &size, &magic, &version) == FS_CFG_OK) {
                rt_kprintf("  %s: magic=0x%08X ver=%u size=%u\n",
                           files[i], magic, version, size);
            }
        } else {
            rt_kprintf("  %s: NOT FOUND\n", files[i]);
        }
    }
    rt_kprintf("=========================\n");
}
MSH_CMD_EXPORT(fs_cfg_info, Show FS config storage info);

#endif /* RT_USING_FINSH */
