#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include "button.h" 
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEY_CTX_NONE = 0,
    KEY_CTX_HID_SHORTCUT,
    KEY_CTX_MENU_NAVIGATION,
    KEY_CTX_VOLUME_CONTROL,
    KEY_CTX_SETTINGS,
    KEY_CTX_SYSTEM,
    KEY_CTX_L2_TIME,
    KEY_CTX_L2_MEDIA,
    KEY_CTX_L2_WEB,
    KEY_CTX_L2_SHORTCUT,
    KEY_CTX_MAX
} key_context_id_t;

typedef int (*key_handler_t)(int key_idx, button_action_t action, void *user_data);

typedef struct {
    key_context_id_t id;
    const char *name;
    key_handler_t handler;
    void *user_data;
    uint8_t priority;
    bool exclusive;
} key_context_config_t;

int key_manager_init(void);
int key_manager_deinit(void);
int key_manager_register_context(const key_context_config_t *config);
int key_manager_unregister_context(key_context_id_t ctx_id);
int key_manager_activate_context(key_context_id_t ctx_id);
int key_manager_deactivate_context(key_context_id_t ctx_id);
key_context_id_t key_manager_get_active_context(void);
int key_manager_push_context(key_context_id_t ctx_id);
int key_manager_pop_context(void);
const char* key_manager_get_context_name(key_context_id_t ctx_id);
int key_manager_enable_led_feedback(bool enable);
bool key_manager_is_led_feedback_enabled(void);

#ifdef __cplusplus
}
#endif
#endif