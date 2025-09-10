#ifndef LED_CONTEXT_H
#define LED_CONTEXT_H

#include "key_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int led_context_init(void);
int led_context_deinit(void);
int led_context_activate(void);
int led_context_deactivate(void);

#ifdef __cplusplus
}
#endif
#endif