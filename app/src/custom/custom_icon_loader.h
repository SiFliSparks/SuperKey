/**
 * @file custom_icon_loader.h
 * @brief 自定义图标加载模块 - 内存加载模式
 * 
 * 参考main_ezip.c例程，将ezip文件加载到内存
 */

#ifndef CUSTOM_ICON_LOADER_H
#define CUSTOM_ICON_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置
 * ============================================================================ */

#define CUSTOM_ICON_COUNT           3
#define CUSTOM_ICON_SD_PATH         "/sdcard/ICON/"
#define CUSTOM_ICON_1_FILENAME      "custom_1.ezip"
#define CUSTOM_ICON_2_FILENAME      "custom_2.ezip"
#define CUSTOM_ICON_3_FILENAME      "custom_3.ezip"

/* ============================================================================
 * 类型定义
 * ============================================================================ */

typedef enum {
    CUSTOM_ICON_1 = 0,
    CUSTOM_ICON_2 = 1,
    CUSTOM_ICON_3 = 2,
} custom_icon_index_t;

typedef struct {
    bool loaded;                /* 是否已加载到内存 */
    lv_img_dsc_t img_dsc;       /* LVGL图片描述符 */
} custom_icon_status_t;

/* ============================================================================
 * API
 * ============================================================================ */

int custom_icon_loader_init(void);
const lv_img_dsc_t* custom_icon_get_dsc(custom_icon_index_t index);
bool custom_icon_is_loaded(custom_icon_index_t index);
void custom_icon_unload_all(void);
void custom_icon_dump_status(void);
int custom_icon_reload(void);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_ICON_LOADER_H */