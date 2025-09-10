#ifndef ENCODER_CONTEXT_H
#define ENCODER_CONTEXT_H

#include "key_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int encoder_context_init(void);

int encoder_context_deinit(void);

int encoder_context_activate(void);

int encoder_context_deactivate(void);

#ifdef __cplusplus
}
#endif
#endif