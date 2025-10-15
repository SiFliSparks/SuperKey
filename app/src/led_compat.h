#ifndef LED_COMPAT_H
#define LED_COMPAT_H

#include "drv_rgbled.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LED颜色常量兼容性定义 - 映射到drv_rgbled.h的定义 */
#define LED_COLOR_OFF           RGB_COLOR_BLACK
#define LED_COLOR_WHITE         RGB_COLOR_WHITE
#define LED_COLOR_RED           RGB_COLOR_RED
#define LED_COLOR_GREEN         RGB_COLOR_GREEN
#define LED_COLOR_BLUE          RGB_COLOR_BLUE
#define LED_COLOR_YELLOW        RGB_COLOR_YELLOW
#define LED_COLOR_CYAN          RGB_COLOR_CYAN
#define LED_COLOR_MAGENTA       RGB_COLOR_MAGENTA

/* 扩展颜色定义（drv_rgbled.h中没有的） */
#define LED_COLOR_ORANGE        0xFF8000
#define LED_COLOR_PURPLE        0x8000FF
#define LED_COLOR_PINK          0xFF80C0

/* LIGHT_COLOR兼容性定义 */
#define LIGHT_COLOR_OFF         LED_COLOR_OFF
#define LIGHT_COLOR_WHITE       LED_COLOR_WHITE
#define LIGHT_COLOR_RED         LED_COLOR_RED
#define LIGHT_COLOR_GREEN       LED_COLOR_GREEN
#define LIGHT_COLOR_BLUE        LED_COLOR_BLUE
#define LIGHT_COLOR_YELLOW      LED_COLOR_YELLOW
#define LIGHT_COLOR_CYAN        LED_COLOR_CYAN
#define LIGHT_COLOR_MAGENTA     LED_COLOR_MAGENTA
#define LIGHT_COLOR_ORANGE      LED_COLOR_ORANGE
#define LIGHT_COLOR_PURPLE      LED_COLOR_PURPLE
#define LIGHT_COLOR_PINK        LED_COLOR_PINK

#ifdef __cplusplus
}
#endif

#endif /* LED_COMPAT_H */