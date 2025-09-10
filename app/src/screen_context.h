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

#ifdef __cplusplus
}
#endif
#endif