/**
 * @file power_manager.h
 * @brief Power management — handles PC sleep/wake commands.
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POWER_STATE_NORMAL = 0,
    POWER_STATE_SLEEP,
} power_state_t;

/** Init power manager. Call after event_bus_init() and LCD ready. */
int power_manager_init(void);

/** Enter sleep: LCD off, backlight off, LEDs off, sensor stopped. */
int power_manager_enter_sleep(void);

/** Exit sleep: LCD on, backlight on, LEDs restored, sensor resumed. */
int power_manager_exit_sleep(void);

/** Get current power state. */
power_state_t power_manager_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_H */