/**
 * @file custom_icon_loader.c
 * @brief 自定义图标加载模块 - 内存加载模式
 * 
 * 参考main_ezip.c例程，将ezip文件加载到PSRAM
 */

#include "custom_icon_loader.h"
#include "../fs/fs_init.h"
#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include "mem_section.h"

#define LOG_TAG     "ICON"

/* ============================================================================
 * PSRAM 堆管理（参考main_ezip.c）
 * ============================================================================ */

static uint8_t psram_heap_pool[512 * 1024] L2_RET_BSS_SECT(psram_heap_pool);
static struct rt_memheap psram_memheap;
static bool psram_initialized = false;

static int psram_heap_init(void)
{
    if (psram_initialized) {
        return 0;
    }
    rt_memheap_init(&psram_memheap, "icon_psram", (void *)psram_heap_pool,
                    sizeof(psram_heap_pool));
    psram_initialized = true;
    rt_kprintf("[%s] PSRAM heap initialized (%d KB)\n", LOG_TAG, sizeof(psram_heap_pool) / 1024);
    return 0;
}

static void *psram_heap_malloc(uint32_t size)
{
    return rt_memheap_alloc(&psram_memheap, size);
}

static void psram_heap_free(void *p)
{
    rt_memheap_free(p);
}

/* ============================================================================
 * 图标加载
 * ============================================================================ */

/* 图标文件名列表 */
static const char* g_icon_filenames[CUSTOM_ICON_COUNT] = {
    CUSTOM_ICON_1_FILENAME,
    CUSTOM_ICON_2_FILENAME,
    CUSTOM_ICON_3_FILENAME
};

/* 图标状态 */
static struct {
    bool initialized;
    custom_icon_status_t icons[CUSTOM_ICON_COUNT];
} g_loader = {0};

/**
 * @brief 加载ezip文件到PSRAM（完全参考main_ezip.c的load_ezip函数）
 */
static int load_ezip(lv_img_dsc_t *img_dsc, const char *ezip_path)
{
    if (img_dsc == NULL || ezip_path == NULL) {
        return -1;
    }
    
    FILE *fp = fopen(ezip_path, "rb");
    if (fp == NULL) {
        rt_kprintf("[%s] Failed to open: %s\n", LOG_TAG, ezip_path);
        return -1;
    }
    
    /* 读取图片头部 */
    fread(&img_dsc->header, sizeof(lv_img_header_t), 1, fp);
    
    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, sizeof(lv_img_header_t), SEEK_SET);
    
    /* 计算数据大小 */
    img_dsc->data_size = file_size - sizeof(lv_img_header_t);
    
    /* 分配PSRAM内存 */
    img_dsc->data = (const uint8_t *)psram_heap_malloc(img_dsc->data_size);
    if (img_dsc->data == NULL) {
        rt_kprintf("[%s] Failed to allocate %d bytes from PSRAM\n", LOG_TAG, img_dsc->data_size);
        fclose(fp);
        return -1;
    }
    
    /* 读取图片数据 */
    size_t read_size = fread((void *)img_dsc->data, 1, img_dsc->data_size, fp);
    if (read_size != img_dsc->data_size) {
        rt_kprintf("[%s] Read error: %d/%d bytes\n", LOG_TAG, read_size, img_dsc->data_size);
        psram_heap_free((void *)img_dsc->data);
        img_dsc->data = NULL;
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    rt_kprintf("[%s] Loaded: %s (%dx%d, %d bytes)\n", LOG_TAG, ezip_path,
               img_dsc->header.w, img_dsc->header.h, img_dsc->data_size);
    return 0;
}

/**
 * @brief 卸载ezip图片
 */
static void unload_ezip(lv_img_dsc_t *img_dsc)
{
    if (img_dsc && img_dsc->data) {
        psram_heap_free((void *)img_dsc->data);
        img_dsc->data = NULL;
        img_dsc->data_size = 0;
    }
}

/**
 * @brief 加载单个图标
 */
static bool load_icon(custom_icon_index_t index)
{
    if (index >= CUSTOM_ICON_COUNT) {
        return false;
    }
    
    custom_icon_status_t *status = &g_loader.icons[index];
    
    /* 构建完整路径 */
    char path[64];
    rt_snprintf(path, sizeof(path), "%s%s", 
                CUSTOM_ICON_SD_PATH, g_icon_filenames[index]);
    
    /* 加载到PSRAM */
    if (load_ezip(&status->img_dsc, path) == 0) {
        status->loaded = true;
        return true;
    }
    
    status->loaded = false;
    return false;
}

/* ============================================================================
 * 公共API
 * ============================================================================ */

int custom_icon_loader_init(void)
{
    if (g_loader.initialized) {
        return 0;
    }
    
    rt_kprintf("[%s] Initializing...\n", LOG_TAG);
    
    memset(&g_loader, 0, sizeof(g_loader));
    
    /* 初始化PSRAM堆 */
    psram_heap_init();
    
    /* 检查SD卡 */
    if (!fs_is_sdcard_mounted()) {
        rt_kprintf("[%s] SD card not mounted\n", LOG_TAG);
        g_loader.initialized = true;
        return 0;
    }
    
    /* 加载所有图标 */
    int found = 0;
    for (int i = 0; i < CUSTOM_ICON_COUNT; i++) {
        if (load_icon((custom_icon_index_t)i)) {
            found++;
        }
    }
    
    rt_kprintf("[%s] Loaded %d/%d custom icons\n", LOG_TAG, found, CUSTOM_ICON_COUNT);
    
    g_loader.initialized = true;
    return 0;
}

const lv_img_dsc_t* custom_icon_get_dsc(custom_icon_index_t index)
{
    if (index >= CUSTOM_ICON_COUNT) {
        return NULL;
    }
    
    if (!g_loader.initialized) {
        custom_icon_loader_init();
    }
    
    if (g_loader.icons[index].loaded) {
        return &g_loader.icons[index].img_dsc;
    }
    
    return NULL;
}

bool custom_icon_is_loaded(custom_icon_index_t index)
{
    if (index >= CUSTOM_ICON_COUNT || !g_loader.initialized) {
        return false;
    }
    return g_loader.icons[index].loaded;
}

void custom_icon_unload_all(void)
{
    for (int i = 0; i < CUSTOM_ICON_COUNT; i++) {
        if (g_loader.icons[i].loaded) {
            unload_ezip(&g_loader.icons[i].img_dsc);
            g_loader.icons[i].loaded = false;
        }
    }
    rt_kprintf("[%s] All icons unloaded\n", LOG_TAG);
}

/**
 * @brief 重新加载所有图标（SD卡热插拔时调用）
 */
int custom_icon_reload(void)
{
    rt_kprintf("[%s] Reloading icons...\n", LOG_TAG);
    
    /* 先卸载旧图标释放内存 */
    custom_icon_unload_all();
    
    /* 检查SD卡 */
    if (!fs_is_sdcard_mounted()) {
        rt_kprintf("[%s] SD card not mounted\n", LOG_TAG);
        return 0;
    }
    
    /* 重新加载所有图标 */
    int found = 0;
    for (int i = 0; i < CUSTOM_ICON_COUNT; i++) {
        if (load_icon((custom_icon_index_t)i)) {
            found++;
        }
    }
    
    rt_kprintf("[%s] Reloaded %d/%d custom icons\n", LOG_TAG, found, CUSTOM_ICON_COUNT);
    return found;
}

void custom_icon_dump_status(void)
{
    rt_kprintf("\n=== Custom Icon Status ===\n");
    rt_kprintf("Initialized: %s\n", g_loader.initialized ? "YES" : "NO");
    rt_kprintf("SD Card: %s\n", fs_is_sdcard_mounted() ? "Mounted" : "Not mounted");
    
    for (int i = 0; i < CUSTOM_ICON_COUNT; i++) {
        custom_icon_status_t *s = &g_loader.icons[i];
        rt_kprintf("Icon %d: %s", i + 1, s->loaded ? "LOADED" : "NOT LOADED");
        if (s->loaded) {
            rt_kprintf(" (%dx%d, %d bytes)", 
                       s->img_dsc.header.w, s->img_dsc.header.h, s->img_dsc.data_size);
        }
        rt_kprintf("\n");
    }
    rt_kprintf("==========================\n");
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void icon_status(void) { custom_icon_dump_status(); }
MSH_CMD_EXPORT(icon_status, Show custom icon status);
#endif