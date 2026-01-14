/**
 * @file screen_ui_resources.h
 * @brief 屏幕UI资源管理模块 - 图片资源声明和获取函数
 * 
 * 本模块集中管理所有图片资源的外部声明和获取函数，
 * 包括：数字图片、天气图标、功能图标、控制图标等
 */

#ifndef SCREEN_UI_RESOURCES_H
#define SCREEN_UI_RESOURCES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 数字图片资源 (0-9)
 ******************************************************************************/

extern const lv_image_dsc_t t0;
extern const lv_image_dsc_t t1;
extern const lv_image_dsc_t t2;
extern const lv_image_dsc_t t3;
extern const lv_image_dsc_t t4;
extern const lv_image_dsc_t t5;
extern const lv_image_dsc_t t6;
extern const lv_image_dsc_t t7;
extern const lv_image_dsc_t t8;
extern const lv_image_dsc_t t9;

/**
 * @brief 获取数字对应的图片资源
 * @param digit 数字 (0-9)
 * @return 对应的图片描述符指针，无效数字返回NULL
 */
const lv_image_dsc_t* get_digit_image(int digit);

/*******************************************************************************
 * 天气图标资源
 ******************************************************************************/

/* 晴天系列 */
extern const lv_image_dsc_t w100;   /* 晴 */
extern const lv_image_dsc_t w101;   /* 多云 */
extern const lv_image_dsc_t w102;   /* 少云 */
extern const lv_image_dsc_t w103;   /* 晴间多云 */
extern const lv_image_dsc_t w104;   /* 阴 */

/* 晴天系列（夜间） */
extern const lv_image_dsc_t w150;   /* 晴（夜间） */
extern const lv_image_dsc_t w151;   /* 多云（夜间） */
extern const lv_image_dsc_t w152;   /* 少云（夜间） */
extern const lv_image_dsc_t w153;   /* 晴间多云（夜间） */

/* 雨天系列 */
extern const lv_image_dsc_t w300;   /* 阵雨 */
extern const lv_image_dsc_t w301;   /* 强阵雨 */
extern const lv_image_dsc_t w302;   /* 雷阵雨 */
extern const lv_image_dsc_t w303;   /* 强雷阵雨 */
extern const lv_image_dsc_t w304;   /* 雷阵雨伴有冰雹 */
extern const lv_image_dsc_t w305;   /* 小雨 */
extern const lv_image_dsc_t w306;   /* 中雨 */
extern const lv_image_dsc_t w307;   /* 大雨 */
extern const lv_image_dsc_t w308;   /* 极端降雨 */
extern const lv_image_dsc_t w309;   /* 毛毛雨/细雨 */
extern const lv_image_dsc_t w310;   /* 暴雨 */
extern const lv_image_dsc_t w311;   /* 大暴雨 */
extern const lv_image_dsc_t w312;   /* 特大暴雨 */
extern const lv_image_dsc_t w313;   /* 冻雨 */
extern const lv_image_dsc_t w314;   /* 小到中雨 */
extern const lv_image_dsc_t w315;   /* 中到大雨 */
extern const lv_image_dsc_t w316;   /* 大到暴雨 */
extern const lv_image_dsc_t w317;   /* 暴雨到大暴雨 */
extern const lv_image_dsc_t w318;   /* 大暴雨到特大暴雨 */
extern const lv_image_dsc_t w350;   /* 阵雨（夜间） */
extern const lv_image_dsc_t w351;   /* 强阵雨（夜间） */
extern const lv_image_dsc_t w399;   /* 雨 */

/* 雪天系列 */
extern const lv_image_dsc_t w400;   /* 小雪 */
extern const lv_image_dsc_t w401;   /* 中雪 */
extern const lv_image_dsc_t w402;   /* 大雪 */
extern const lv_image_dsc_t w403;   /* 暴雪 */
extern const lv_image_dsc_t w404;   /* 雨夹雪 */
extern const lv_image_dsc_t w405;   /* 雨雪天气 */
extern const lv_image_dsc_t w406;   /* 阵雨夹雪 */
extern const lv_image_dsc_t w407;   /* 阵雪 */
extern const lv_image_dsc_t w408;   /* 小到中雪 */
extern const lv_image_dsc_t w409;   /* 中到大雪 */
extern const lv_image_dsc_t w410;   /* 大到暴雪 */
extern const lv_image_dsc_t w456;   /* 阵雨夹雪（夜间） */
extern const lv_image_dsc_t w457;   /* 阵雪（夜间） */
extern const lv_image_dsc_t w499;   /* 雪 */

/* 雾霾沙尘系列 */
extern const lv_image_dsc_t w500;   /* 薄雾 */
extern const lv_image_dsc_t w501;   /* 雾 */
extern const lv_image_dsc_t w502;   /* 霾 */
extern const lv_image_dsc_t w503;   /* 扬沙 */
extern const lv_image_dsc_t w504;   /* 浮尘 */
extern const lv_image_dsc_t w507;   /* 沙尘暴 */
extern const lv_image_dsc_t w508;   /* 强沙尘暴 */
extern const lv_image_dsc_t w509;   /* 浓雾 */
extern const lv_image_dsc_t w510;   /* 强浓雾 */
extern const lv_image_dsc_t w511;   /* 中度霾 */
extern const lv_image_dsc_t w512;   /* 重度霾 */
extern const lv_image_dsc_t w513;   /* 严重霾 */
extern const lv_image_dsc_t w514;   /* 大雾 */
extern const lv_image_dsc_t w515;   /* 特强浓雾 */

/* 特殊天气 */
extern const lv_image_dsc_t w900;   /* 热 */
extern const lv_image_dsc_t w901;   /* 冷 */
extern const lv_image_dsc_t w999;   /* 未知 */

/**
 * @brief 根据天气代码获取对应图标
 * @param weather_code 天气代码 (0-999)
 * @return 对应的图片描述符指针，无效代码返回未知天气图标
 */
const lv_image_dsc_t* get_weather_icon(int weather_code);

/*******************************************************************************
 * 功能图标资源
 ******************************************************************************/

/* Group 4 功能图标 */
extern const lv_image_dsc_t muyu;           /* 木鱼 */
extern const lv_image_dsc_t tomatolock;     /* 番茄钟 */
extern const lv_image_dsc_t calculagraph;   /* 计时器 */

/* Group 5 自定义图标 */
extern const lv_image_dsc_t custom1;        /* 自定义1 */
extern const lv_image_dsc_t custom2;        /* 自定义2 */
extern const lv_image_dsc_t custom3;        /* 自定义3 */

/* Group 3 控制图标 */
extern const lv_image_dsc_t media;          /* 媒体控制 */
extern const lv_image_dsc_t web;            /* 网页控制 */
extern const lv_image_dsc_t shortcut;       /* 快捷键 */

/* Group 2 系统监控图标 */
extern const lv_image_dsc_t cpuicon;        /* CPU图标 */
extern const lv_image_dsc_t gpuicon;        /* GPU图标 */
extern const lv_image_dsc_t memicon;        /* 内存图标 */

/* 功能图标获取函数 */
const lv_image_dsc_t* get_muyu_image(void);
const lv_image_dsc_t* get_tomato_image(void);
const lv_image_dsc_t* get_calculagraph_image(void);
const lv_image_dsc_t* get_custom1_image(void);
const lv_image_dsc_t* get_custom2_image(void);
const lv_image_dsc_t* get_custom3_image(void);
const lv_image_dsc_t* get_media_image(void);
const lv_image_dsc_t* get_web_image(void);
const lv_image_dsc_t* get_shortcut_image(void);
const lv_image_dsc_t* get_cpu_icon(void);
const lv_image_dsc_t* get_gpu_icon(void);
const lv_image_dsc_t* get_mem_icon(void);

/*******************************************************************************
 * L2页面控制图标资源
 ******************************************************************************/

/* 媒体控制图标 */
extern const lv_image_dsc_t pre_song;       /* 上一曲 */
extern const lv_image_dsc_t next_song;      /* 下一曲 */
extern const lv_image_dsc_t play;           /* 播放/暂停 */

/* 编辑控制图标 */
extern const lv_image_dsc_t ctrlc;          /* 复制 (Ctrl+C) */
extern const lv_image_dsc_t ctrlv;          /* 粘贴 (Ctrl+V) */
extern const lv_image_dsc_t ctrlz;          /* 撤销 (Ctrl+Z) */

/* 导航图标 */
extern const lv_image_dsc_t up;             /* 上翻页 */
extern const lv_image_dsc_t down;           /* 下翻页 */
extern const lv_image_dsc_t fresh;          /* 刷新 */

/* L2页面图标获取函数 */
const lv_image_dsc_t* get_pre_song_image(void);
const lv_image_dsc_t* get_next_song_image(void);
const lv_image_dsc_t* get_play_image(void);
const lv_image_dsc_t* get_ctrlc_image(void);
const lv_image_dsc_t* get_ctrlv_image(void);
const lv_image_dsc_t* get_ctrlz_image(void);
const lv_image_dsc_t* get_up_image(void);
const lv_image_dsc_t* get_down_image(void);
const lv_image_dsc_t* get_fresh_image(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_UI_RESOURCES_H */
