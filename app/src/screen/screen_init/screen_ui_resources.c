/**
 * @file screen_ui_resources.c
 * @brief 屏幕UI资源管理模块实现
 * 
 * 本模块实现图片资源的获取函数，包括：
 * - 数字图片获取
 * - 天气图标映射和获取
 * - 功能图标获取
 * - L2页面控制图标获取
 */

#include "screen_ui_resources.h"
#include <stddef.h>

/*******************************************************************************
 * 数字图片数组
 ******************************************************************************/

static const lv_image_dsc_t* s_digit_images[10] = {
    &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8, &t9
};

/*******************************************************************************
 * 天气图标映射表
 ******************************************************************************/

#define WEATHER_ICON_MAP_MAX 1000

static const lv_image_dsc_t* s_weather_icon_map[WEATHER_ICON_MAP_MAX] = {
    /* 晴天系列 100-104 */
    [100] = &w100, [101] = &w101, [102] = &w102, [103] = &w103, [104] = &w104,
    
    /* 晴天系列（夜间） 150-153 */
    [150] = &w150, [151] = &w151, [152] = &w152, [153] = &w153,
    
    /* 雨天系列 300-318 */
    [300] = &w300, [301] = &w301, [302] = &w302, [303] = &w303, [304] = &w304,
    [305] = &w305, [306] = &w306, [307] = &w307, [308] = &w308, [309] = &w309,
    [310] = &w310, [311] = &w311, [312] = &w312, [313] = &w313, [314] = &w314,
    [315] = &w315, [316] = &w316, [317] = &w317, [318] = &w318,
    
    /* 雨天系列（夜间） 350-351, 399 */
    [350] = &w350, [351] = &w351, [399] = &w399,
    
    /* 雪天系列 400-410, 456-457, 499 */
    [400] = &w400, [401] = &w401, [402] = &w402, [403] = &w403, [404] = &w404,
    [405] = &w405, [406] = &w406, [407] = &w407, [408] = &w408, [409] = &w409,
    [410] = &w410, [456] = &w456, [457] = &w457, [499] = &w499,
    
    /* 雾霾沙尘系列 500-515 */
    [500] = &w500, [501] = &w501, [502] = &w502, [503] = &w503, [504] = &w504,
    [507] = &w507, [508] = &w508, [509] = &w509, [510] = &w510, [511] = &w511,
    [512] = &w512, [513] = &w513, [514] = &w514, [515] = &w515,
    
    /* 特殊天气 900-901, 999 */
    [900] = &w900, [901] = &w901, [999] = &w999
};

/*******************************************************************************
 * 数字图片获取
 ******************************************************************************/

const lv_image_dsc_t* get_digit_image(int digit)
{
    if (digit < 0 || digit > 9) {
        return NULL;
    }
    return s_digit_images[digit];
}

/*******************************************************************************
 * 天气图标获取
 ******************************************************************************/

const lv_image_dsc_t* get_weather_icon(int weather_code)
{
    if (weather_code >= 0 && weather_code < WEATHER_ICON_MAP_MAX && 
        s_weather_icon_map[weather_code] != NULL) {
        return s_weather_icon_map[weather_code];
    }
    /* 默认返回未知天气图标 */
    return &w999;
}

/*******************************************************************************
 * 功能图标获取 - Group 4 (木鱼/番茄钟/计时器)
 ******************************************************************************/

const lv_image_dsc_t* get_muyu_image(void)
{
    return &muyu;
}

const lv_image_dsc_t* get_tomato_image(void)
{
    return &tomatolock;
}

const lv_image_dsc_t* get_calculagraph_image(void)
{
    return &calculagraph;
}

/*******************************************************************************
 * 功能图标获取 - Group 5 (自定义)
 ******************************************************************************/

const lv_image_dsc_t* get_custom1_image(void)
{
    return &custom1;
}

const lv_image_dsc_t* get_custom2_image(void)
{
    return &custom2;
}

const lv_image_dsc_t* get_custom3_image(void)
{
    return &custom3;
}

/*******************************************************************************
 * 功能图标获取 - Group 3 (媒体/网页/快捷键)
 ******************************************************************************/

const lv_image_dsc_t* get_media_image(void)
{
    return &media;
}

const lv_image_dsc_t* get_web_image(void)
{
    return &web;
}

const lv_image_dsc_t* get_shortcut_image(void)
{
    return &shortcut;
}

/*******************************************************************************
 * 功能图标获取 - Group 2 (系统监控)
 ******************************************************************************/

const lv_image_dsc_t* get_cpu_icon(void)
{
    return &cpuicon;
}

const lv_image_dsc_t* get_gpu_icon(void)
{
    return &gpuicon;
}

const lv_image_dsc_t* get_mem_icon(void)
{
    return &memicon;
}

/*******************************************************************************
 * L2页面图标获取 - 媒体控制
 ******************************************************************************/

const lv_image_dsc_t* get_pre_song_image(void)
{
    return &pre_song;
}

const lv_image_dsc_t* get_next_song_image(void)
{
    return &next_song;
}

const lv_image_dsc_t* get_play_image(void)
{
    return &play;
}

/*******************************************************************************
 * L2页面图标获取 - 编辑控制
 ******************************************************************************/

const lv_image_dsc_t* get_ctrlc_image(void)
{
    return &ctrlc;
}

const lv_image_dsc_t* get_ctrlv_image(void)
{
    return &ctrlv;
}

const lv_image_dsc_t* get_ctrlz_image(void)
{
    return &ctrlz;
}

/*******************************************************************************
 * L2页面图标获取 - 导航控制
 ******************************************************************************/

const lv_image_dsc_t* get_up_image(void)
{
    return &up;
}

const lv_image_dsc_t* get_down_image(void)
{
    return &down;
}

const lv_image_dsc_t* get_fresh_image(void)
{
    return &fresh;
}
