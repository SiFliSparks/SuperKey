#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int led_controller_init(void);
int led_controller_deinit(void);
int led_controller_light_led(int led_index, uint32_t color, uint32_t duration_ms);
int led_controller_turn_off_all(void);
int led_controller_set_led_color(int led_index, uint32_t color);
int led_controller_set_led_colors(const uint32_t *colors, uint16_t count);
bool led_controller_is_ready(void);

#define LED_COLOR_OFF     0x000000
#define LED_COLOR_RED     0xFF0000
#define LED_COLOR_GREEN   0x00FF00
#define LED_COLOR_BLUE    0x0000FF
#define LED_COLOR_WHITE   0xFFFFFF
#define LED_COLOR_YELLOW  0xFFFF00
#define LED_COLOR_PURPLE  0xFF00FF
#define LED_COLOR_CYAN    0x00FFFF

#ifdef __cplusplus
}
#endif
#endif