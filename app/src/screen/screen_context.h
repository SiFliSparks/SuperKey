#ifndef SCREEN_CONTEXT_H
#define SCREEN_CONTEXT_H

#include "manager/key_manager.h"
#include "screen/screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 番茄钟类型定义 ========== */
typedef enum {
    TOMATO_MODE_FOCUS = 0,      /* 专注模式 */
    TOMATO_MODE_SHORT_BREAK,    /* 短休息 */
    TOMATO_MODE_LONG_BREAK,     /* 长休息 */
    TOMATO_MODE_MAX
} tomato_mode_t;

typedef enum {
    TOMATO_STATE_IDLE = 0,      /* 待机 */
    TOMATO_STATE_RUNNING,       /* 计时中 */
    TOMATO_STATE_PAUSED,        /* 已暂停 */
    TOMATO_STATE_COMPLETED,     /* 已完成 */
} tomato_state_t;

typedef struct {
    tomato_mode_t current_mode;
    tomato_state_t current_state;
    uint16_t remaining_seconds;
    uint16_t total_seconds;
    uint8_t current_round;
    uint8_t progress_percent;
    uint16_t today_completed;
    uint8_t continuous_count;
    uint32_t total_completed;
    uint16_t focus_duration_min;
    uint16_t short_break_min;
    uint16_t long_break_min;
    uint8_t long_break_interval;
    bool valid;
    rt_tick_t last_update_tick;
} tomato_data_t;


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

void screen_context_init_tomato_data(void);

int screen_context_get_tomato_data(tomato_data_t *data);

int screen_context_handle_tomato_start(void);

int screen_context_handle_tomato_pause(void);

int screen_context_handle_tomato_stop(void);

int screen_context_handle_tomato_mode_switch(void);

int screen_context_handle_tomato_complete(void);

void tomato_process_countdown(void);

void screen_context_init_stopwatch_data(void);

int screen_context_get_stopwatch_data(stopwatch_data_t *data);

int screen_context_handle_stopwatch_start(void);

int screen_context_handle_stopwatch_pause(void);

int screen_context_handle_stopwatch_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_CONTEXT_H */