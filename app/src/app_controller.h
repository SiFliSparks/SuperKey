#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <stdbool.h>
#include "encoder_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

int app_controller_init(void);

int app_controller_deinit(void);

int app_controller_switch_mode(const char *mode_name);

const char* app_controller_get_current_mode(void);

int app_controller_set_encoder_mode(encoder_mode_t mode);
encoder_mode_t app_controller_get_encoder_mode(void);

bool app_controller_is_encoder_available(void);

bool app_controller_is_hid_activated(void);

int app_controller_force_activate_hid(void);

int app_controller_force_activate_none(void);

#ifdef __cplusplus
}
#endif
#endif