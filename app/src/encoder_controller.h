#ifndef ENCODER_CONTROLLER_H
#define ENCODER_CONTROLLER_H

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ENCODER_MODE_IDLE = 0,
    ENCODER_MODE_VOLUME,
    ENCODER_MODE_SCROLL,
    ENCODER_MODE_BRIGHTNESS,
    ENCODER_MODE_MENU_NAV,
    ENCODER_MODE_SCREEN_SWITCH,
    ENCODER_MODE_CUSTOM,
    ENCODER_MODE_MAX
} encoder_mode_t;

typedef enum {
    ENCODER_DIR_CW = 1,
    ENCODER_DIR_CCW = -1,
    ENCODER_DIR_NONE = 0
} encoder_direction_t;

int encoder_controller_init(void);
int encoder_controller_deinit(void);
int encoder_controller_set_mode(encoder_mode_t mode);
encoder_mode_t encoder_controller_get_mode(void);
int32_t encoder_controller_get_count(void);
int encoder_controller_reset_count(void);
int32_t encoder_controller_get_delta(void);
int encoder_controller_start_polling(void);
int encoder_controller_stop_polling(void);
int encoder_controller_set_sensitivity(uint8_t divider);
const char* encoder_controller_get_mode_name(encoder_mode_t mode);
bool encoder_controller_is_ready(void);
int encoder_controller_enable_screen_switch(bool enable);
bool encoder_controller_is_screen_switch_enabled(void);

#ifdef __cplusplus
}
#endif
#endif