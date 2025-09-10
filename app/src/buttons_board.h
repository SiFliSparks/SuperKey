#ifndef BUTTONS_BOARD_H
#define BUTTONS_BOARD_H

#include "button.h"
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTON_KEY1   0
#define BUTTON_KEY2   1  
#define BUTTON_KEY3   2
#define BUTTON_KEY4   3
#define BUTTON_COUNT  4

int buttons_board_init(button_handler_t unified_callback);

int buttons_board_pin_to_idx(int32_t pin);

int buttons_board_count(void);

int buttons_board_deinit(void);

int buttons_board_enable(int key_idx);
int buttons_board_disable(int key_idx);

bool buttons_board_is_pressed(int key_idx);

#ifdef __cplusplus
}
#endif
#endif