/**
 * @file screen_ui_group2.h
 * @brief Group 2 UI模块 - 系统监控 (CPU/GPU/内存/网络)
 * 
 * Group 2 包含三个面板:
 * - 左屏: CPU监控 (温度、占用率仪表)
 * - 中屏: 内存监控 (使用率、图表) + 网络速度 (上传/下载)
 * - 右屏: GPU监控 (温度、占用率仪表)
 */

#ifndef SCREEN_UI_GROUP2_H
#define SCREEN_UI_GROUP2_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Group 2 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - CPU监控面板
 * @param parent 父容器
 * 
 * 包含:
 * - CPU图标 (全尺寸)
 * - CPU温度显示 (右上角)
 * - CPU占用率仪表盘 (底部)
 */
void group2_build_left_cpu_panel(lv_obj_t *parent);

/**
 * @brief 构建中屏 - 内存/网络监控面板
 * @param parent 父容器
 * 
 * 包含:
 * - 内存图标 (全尺寸)
 * - 内存使用率 (左侧)
 * - 内存历史图表 (左下)
 * - 网络上传速度 (右侧)
 * - 网络下载速度 (右下)
 */
void group2_build_middle_memory_panel(lv_obj_t *parent);

/**
 * @brief 构建右屏 - GPU监控面板
 * @param parent 父容器
 * 
 * 包含:
 * - GPU图标 (全尺寸)
 * - GPU温度显示 (右上角)
 * - GPU占用率仪表盘 (底部)
 */
void group2_build_right_gpu_panel(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_GROUP2_H */
