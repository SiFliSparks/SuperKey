/**
 * @file screen_ui_group2.c
 * @brief Group 2 UI模块实现 - 系统监控 (CPU/GPU/内存/网络)
 * 
 * Group 2 包含三个面板:
 * - 左屏: CPU监控 (温度、占用率仪表)
 * - 中屏: 内存监控 (使用率、图表) + 网络速度 (上传/下载)
 * - 右屏: GPU监控 (温度、占用率仪表)
 */

#include "screen_ui_group2.h"
#include "screen_ui_common.h"
#include "screen_ui_resources.h"
#include "../screen_ui_manager.h"

/*******************************************************************************
 * 外部引用
 ******************************************************************************/

/* 全局UI管理器 */
extern screen_ui_manager_t g_ui_mgr;

/*******************************************************************************
 * Group 2 L1 面板构建
 ******************************************************************************/

/**
 * @brief 构建左屏 - CPU监控面板
 */
void group2_build_left_cpu_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* CPU图标 - 居中显示，占满板块 */
    lv_obj_t *cpu_icon = create_fullsize_icon(parent, get_cpu_icon());
    (void)cpu_icon;  /* 避免未使用警告 */
    
    /* CPU温度 - 右上角 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_temp = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, "--.-°C");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, lv_color_make(255, 100, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    /* CPU占用率仪表 - 图标下方 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge = create_usage_arc(parent, 
        lv_color_make(255, 165, 0), &g_ui_mgr.handles.group2_cpu_gpu.cpu_usage);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge, LV_ALIGN_BOTTOM_MID, 0, 10);
}

/**
 * @brief 构建中屏 - 内存/网络监控面板
 */
void group2_build_middle_memory_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* 内存图标 - 居中显示，占满板块 */
    lv_obj_t *mem_icon = create_fullsize_icon(parent, get_mem_icon());
    (void)mem_icon;  /* 避免未使用警告 */
    
    /* ========== 左侧：内存使用率 + 图表 ========== */
    
    /* 内存使用率 - 左上角 */
    g_ui_mgr.handles.group2_memory.ram_usage = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_usage, "--.--%");
    lv_obj_add_style(g_ui_mgr.handles.group2_memory.ram_usage, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_memory.ram_usage, lv_color_make(255, 215, 0), 0);
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_usage, LV_ALIGN_LEFT_MID, 5, -15);
    
    /* 内存图表 - 左侧底部 */
    g_ui_mgr.handles.group2_memory.ram_chart = create_memory_chart(parent, lv_color_make(255, 215, 0));
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_chart, LV_ALIGN_BOTTOM_LEFT, 3, -3);
    
    /* ========== 右侧：网络上传/下载 ========== */
    
    /* 上传速度 - 右侧中上 */
    g_ui_mgr.handles.group2_network.net_upload = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_upload, "-.--MB");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_upload, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_upload, lv_color_make(255, 100, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group2_network.net_upload, LV_ALIGN_RIGHT_MID, -3, 7);
    
    /* 下载速度 - 上传速度下方，右对齐 */
    g_ui_mgr.handles.group2_network.net_download = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_network.net_download, "-.--MB");
    lv_obj_add_style(g_ui_mgr.handles.group2_network.net_download, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_network.net_download, lv_color_make(100, 255, 100), 0);
    lv_obj_align(g_ui_mgr.handles.group2_network.net_download, LV_ALIGN_RIGHT_MID, -3, 50);
}

/**
 * @brief 构建右屏 - GPU监控面板
 */
void group2_build_right_gpu_panel(lv_obj_t *parent)
{
    if (!parent) return;
    
    /* GPU图标 - 居中显示，占满板块 */
    lv_obj_t *gpu_icon = create_fullsize_icon(parent, get_gpu_icon());
    (void)gpu_icon;  /* 避免未使用警告 */
    
    /* GPU温度 - 右上角 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_temp = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, "--.-°C");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, &g_ui_mgr.handles.style_large, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, lv_color_make(100, 255, 150), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    /* GPU占用率仪表 - 图标下方 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge = create_usage_arc(parent, 
        lv_color_make(0, 255, 127), &g_ui_mgr.handles.group2_cpu_gpu.gpu_usage);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge, LV_ALIGN_BOTTOM_MID, 0, 10);
}
