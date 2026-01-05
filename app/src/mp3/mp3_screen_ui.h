/**
 * @file mp3_screen_ui.h
 * @brief MP3 页面 UI 构建模块头文件
 * 
 * 修复版本 - 添加了清理函数声明
 */

#ifndef __MP3_SCREEN_UI_H__
#define __MP3_SCREEN_UI_H__

#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 构建 MP3 页面 UI
 * @param parent 父对象 (屏幕)
 * @return 0 成功
 */
int mp3_screen_ui_build(lv_obj_t *parent);

/**
 * @brief 更新 MP3 页面 UI
 * @return 0 成功
 */
int mp3_screen_ui_update(void);

/**
 * @brief 检查 UI 是否已构建
 * @return true 已构建
 */
bool mp3_screen_ui_is_built(void);

/**
 * @brief 清理 MP3 页面 UI
 * 
 * 释放 UI 资源并重置构建标志
 */
void mp3_screen_ui_cleanup(void);

/**
 * @brief 获取 MP3 屏幕对象
 * @return 屏幕对象指针，未构建时返回 NULL
 */
lv_obj_t* mp3_screen_ui_get_screen(void);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_SCREEN_UI_H__ */