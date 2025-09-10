#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#define MOD_LCTRL 0x01
#define MOD_LSHIFT 0x02
#define MOD_LALT  0x04
#define MOD_LGUI  0x08

#define OS_MODIFIER MOD_LCTRL

#define KEY_A 0x04
#define KEY_C 0x06
#define KEY_V 0x19
#define KEY_X 0x1B
#define KEY_Z 0x1D
#define KEY_PAGE_UP   0x4B
#define KEY_PAGE_DOWN 0x4E
#define KEY_F5        0x3E

#define CC_VOL_UP      (1u << 0)
#define CC_VOL_DOWN    (1u << 1)
#define CC_PLAY_PAUSE  (1u << 2)

#ifdef __cplusplus
extern "C" {
#endif

void hid_device_init(uint8_t busid, uintptr_t reg_base);
void hid_kbd_send(uint8_t modifier, uint8_t keycode);
void hid_kbd_send_combo(uint8_t modifier, uint8_t keycode);
void hid_consumer_click(uint8_t bits);
bool hid_device_ready(void);
void hid_reset_semaphore(void);
bool hid_is_busy(void);
int hid_get_semaphore_count(void);

#ifdef __cplusplus
}
#endif
#endif