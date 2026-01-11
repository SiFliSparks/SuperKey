/**
 * @file hid_cdc_composite.h
 * @brief HID + CDC ACM 复合USB设备
 */

#ifndef HID_CDC_COMPOSITE_H
#define HID_CDC_COMPOSITE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * HID 相关定义 
 * ============================================================================ */

#define MOD_LCTRL  0x01
#define MOD_LSHIFT 0x02
#define MOD_LALT   0x04
#define MOD_LGUI   0x08
#define MOD_RCTRL  0x10
#define MOD_RSHIFT 0x20
#define MOD_RALT   0x40
#define MOD_RGUI   0x80

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
#define CC_SCAN_NEXT   (1u << 3)
#define CC_SCAN_PREV   (1u << 4)

/* 6-Key Rollover 最大同时按键数 */
#define HID_KBD_MAX_KEYS  6

/* ============================================================================
 * CDC 相关定义
 * ============================================================================ */

/* CDC 接收缓冲区大小 */
#define CDC_RX_BUFFER_SIZE  512
#define CDC_TX_BUFFER_SIZE  512

/* CDC 数据接收回调函数类型 */
typedef void (*cdc_rx_callback_t)(const uint8_t *data, uint32_t len);

/* ============================================================================
 * 复合设备初始化
 * ============================================================================ */

/**
 * @brief 初始化HID+CDC复合设备
 * @param busid USB总线ID
 * @param reg_base USB控制器寄存器基地址
 */
void hid_cdc_composite_init(uint8_t busid, uintptr_t reg_base);

/**
 * @brief 兼容旧API的初始化函数
 */
void hid_device_init(uint8_t busid, uintptr_t reg_base);

/* ============================================================================
 * HID 键盘 API 
 * ============================================================================ */

void hid_kbd_send(uint8_t modifier, uint8_t keycode);
void hid_kbd_send_combo(uint8_t modifier, uint8_t keycode);
void hid_consumer_click(uint8_t bits);
bool hid_device_ready(void);
void hid_reset_semaphore(void);
bool hid_is_busy(void);
void hid_kbd_press(uint8_t modifier, uint8_t keycode);
void hid_kbd_release(void);

/* 6KRO 多键支持 */
int hid_kbd_key_down(uint8_t modifier, uint8_t keycode);
int hid_kbd_key_up(uint8_t modifier, uint8_t keycode);
void hid_kbd_release_all(void);
int hid_kbd_get_pressed_count(void);
bool hid_kbd_is_key_pressed(uint8_t keycode);

/* ============================================================================
 * CDC 虚拟串口 API
 * ============================================================================ */

bool cdc_device_ready(void);
int cdc_write(const uint8_t *data, uint32_t len);
int cdc_write_string(const char *str);
void cdc_set_rx_callback(cdc_rx_callback_t callback);
bool cdc_is_tx_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* HID_CDC_COMPOSITE_H */