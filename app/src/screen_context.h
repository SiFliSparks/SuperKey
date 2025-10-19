#ifndef SCREEN_CONTEXT_H
#define SCREEN_CONTEXT_H

#include "key_manager.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

int screen_context_init_all(void);

int screen_context_deinit_all(void);

int screen_context_activate_for_group(screen_group_t group);

int screen_context_deactivate_all(void);

int screen_context_activate_for_level2(screen_l2_group_t l2_group);

int screen_context_deactivate_level2(void);

int screen_context_init_background_breathing(void);

int screen_context_cleanup_background_breathing(void);

void screen_context_process_background_restore(void);

int screen_context_restore_background_breathing(void);

int screen_context_handle_muyu_reset(void);

void screen_context_init_muyu_counter(void);

int screen_context_get_muyu_count(uint32_t *tap_count, uint32_t *total_taps);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_CONTEXT_H */