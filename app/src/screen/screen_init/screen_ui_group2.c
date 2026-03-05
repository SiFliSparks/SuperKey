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
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_temp, LV_ALIGN_TOP_RIGHT, -5, 3);
    
    /* CPU占用率仪表 - 原位置不动 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge = create_usage_arc(parent, 
        lv_color_make(255, 165, 0), NULL);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge, LV_ALIGN_BOTTOM_MID, 0, 10);
    
    /* 圆环内部：占用率 - 偏上 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_usage = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, "--%");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, get_style_medium(), 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, lv_color_make(255, 165, 0), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_usage, LV_ALIGN_CENTER, 0, -10);
    
    /* 圆环内部：CPU频率 - 占用率下方 */
    g_ui_mgr.handles.group2_cpu_gpu.cpu_freq = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.cpu_gauge);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.cpu_freq, "-- MHz");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.cpu_freq, get_style_medium(), 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.cpu_freq, lv_color_make(180, 180, 180), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.cpu_freq, LV_ALIGN_CENTER, 0, 8);
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
    
    /* ========== 左侧：内存进度条 + 已用/总计 ========== */
    
    /* 内存使用率进度条 - 顶部水平条 */
    g_ui_mgr.handles.group2_memory.ram_usage = lv_bar_create(parent);
    lv_obj_set_size(g_ui_mgr.handles.group2_memory.ram_usage, 55, 6);
    lv_bar_set_range(g_ui_mgr.handles.group2_memory.ram_usage, 0, 100);
    lv_bar_set_value(g_ui_mgr.handles.group2_memory.ram_usage, 0, LV_ANIM_OFF);
    /* Background: same grey as CPU/GPU arc */
    lv_obj_set_style_bg_color(g_ui_mgr.handles.group2_memory.ram_usage, lv_color_make(40, 40, 40), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_ui_mgr.handles.group2_memory.ram_usage, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(g_ui_mgr.handles.group2_memory.ram_usage, 3, LV_PART_MAIN);
    /* Indicator: gold */
    lv_obj_set_style_bg_color(g_ui_mgr.handles.group2_memory.ram_usage, lv_color_make(255, 215, 0), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_ui_mgr.handles.group2_memory.ram_usage, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_ui_mgr.handles.group2_memory.ram_usage, 3, LV_PART_INDICATOR);
    lv_obj_align(g_ui_mgr.handles.group2_memory.ram_usage, LV_ALIGN_TOP_LEFT, 5, 38);
    
    /* "已用" label - style_small, grey */
    lv_obj_t *used_label = lv_label_create(parent);
    lv_label_set_text(used_label, "已用");
    lv_obj_add_style(used_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(used_label, lv_color_make(180, 180, 180), 0);
    lv_obj_align_to(used_label,
                    g_ui_mgr.handles.group2_memory.ram_usage, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);
    
    /* mem_used value - style_medium, gold */
    g_ui_mgr.handles.group2_memory.ram_used = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_used, "-- GB");
    lv_obj_add_style(g_ui_mgr.handles.group2_memory.ram_used, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_memory.ram_used, lv_color_make(255, 215, 0), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_memory.ram_used,
                    used_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    
    /* "总计" label - style_small, grey */
    lv_obj_t *total_label = lv_label_create(parent);
    lv_label_set_text(total_label, "总计");
    lv_obj_add_style(total_label, &g_ui_mgr.handles.style_small, 0);
    lv_obj_set_style_text_color(total_label, lv_color_make(180, 180, 180), 0);
    lv_obj_align_to(total_label,
                    g_ui_mgr.handles.group2_memory.ram_used, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 1);
    
    /* mem_total value - style_medium, gold */
    g_ui_mgr.handles.group2_memory.ram_total = lv_label_create(parent);
    lv_label_set_text(g_ui_mgr.handles.group2_memory.ram_total, "-- GB");
    lv_obj_add_style(g_ui_mgr.handles.group2_memory.ram_total, &g_ui_mgr.handles.style_medium, 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_memory.ram_total, lv_color_make(255, 215, 0), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_memory.ram_total,
                    total_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    
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
    lv_obj_align(g_ui_mgr.handles.group2_network.net_download, LV_ALIGN_RIGHT_MID, -3, 53);
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
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_temp, LV_ALIGN_TOP_RIGHT, -5, 3);
    
    /* GPU占用率仪表 - 原位置不动 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge = create_usage_arc(parent, 
        lv_color_make(0, 255, 127), NULL);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge, LV_ALIGN_BOTTOM_MID, 0, 10);
    
    /* 圆环内部：占用率 - 偏上 */
    g_ui_mgr.handles.group2_cpu_gpu.gpu_usage = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, "--%");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, get_style_medium(), 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, lv_color_make(0, 255, 127), 0);
    lv_obj_align(g_ui_mgr.handles.group2_cpu_gpu.gpu_usage, LV_ALIGN_CENTER, 0, -14);
    
    /* 圆环内部：已用 gpu_mem_used - 同一行横排 */
    lv_obj_t *vram_used_hint = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge);
    lv_label_set_text(vram_used_hint, "已用");
    lv_obj_add_style(vram_used_hint, get_style_small(), 0);
    lv_obj_set_style_text_color(vram_used_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(vram_used_hint, LV_ALIGN_CENTER, -18, 2);
    
    g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used, "--G");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used, get_style_medium(), 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used, lv_color_make(180, 180, 180), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_used,
                    vram_used_hint, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
    
    /* 圆环内部：总计 gpu_mem_total - 同一行横排 */
    lv_obj_t *vram_total_hint = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge);
    lv_label_set_text(vram_total_hint, "共");
    lv_obj_add_style(vram_total_hint, get_style_small(), 0);
    lv_obj_set_style_text_color(vram_total_hint, lv_color_make(180, 180, 180), 0);
    lv_obj_align(vram_total_hint, LV_ALIGN_CENTER, -22, 16);
    
    g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total = lv_label_create(g_ui_mgr.handles.group2_cpu_gpu.gpu_gauge);
    lv_label_set_text(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total, "--G");
    lv_obj_add_style(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total, get_style_medium(), 0);
    lv_obj_set_style_text_color(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total, lv_color_make(180, 180, 180), 0);
    lv_obj_align_to(g_ui_mgr.handles.group2_cpu_gpu.gpu_mem_total,
                    vram_total_hint, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
}