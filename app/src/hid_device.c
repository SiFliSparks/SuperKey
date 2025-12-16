#include "hid_device.h"
#include "rtthread.h"
#include "bf0_hal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include <string.h>
#include <rthw.h> 
#define USBD_VID           0x38f4
#define USBD_PID           0x1000
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 0x0409

#define HID_EP_ADDR          0x86
#define HID_EP_SIZE          9
#define HID_EP_INTERVAL      10

#define USB_HID_CONFIG_DESC_SIZ  34

static const uint8_t hid_combined_report_desc[] = {
    /* Keyboard Report */
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    
    /* Modifier keys (8 bits) */
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,       //   Usage Minimum (Left Control)
    0x29, 0xE7,       //   Usage Maximum (Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    
    /* Reserved byte */
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x03,       //   Input (Constant)
    
    /* LED output (5 bits + 3 padding) */
    0x95, 0x05,       //   Report Count (5)
    0x75, 0x01,       //   Report Size (1)
    0x05, 0x08,       //   Usage Page (LEDs)
    0x19, 0x01,       //   Usage Minimum (Num Lock)
    0x29, 0x05,       //   Usage Maximum (Kana)
    0x91, 0x02,       //   Output (Data, Variable, Absolute)
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x03,       //   Report Size (3)
    0x91, 0x03,       //   Output (Constant)
    
    /* Keycodes (6 bytes for 6KRO) */
    0x95, 0x06,       //   Report Count (6) - 6-Key Rollover
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0xFF,       //   Logical Maximum (255)
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data, Array)
    0xC0,             // End Collection

    /* Consumer Control Report */
    0x05, 0x0C,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID (2)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x09, 0xE9,       //   Usage (Volume Increment)
    0x09, 0xEA,       //   Usage (Volume Decrement)
    0x09, 0xCD,       //   Usage (Play/Pause)
    0x09, 0xB5,       //   Usage (Scan Next Track)
    0x09, 0xB6,       //   Usage (Scan Previous Track)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x05,       //   Report Count (5)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    0x95, 0x03,       //   Report Count (3) - Padding
    0x81, 0x03,       //   Input (Constant)
    0xC0              // End Collection
};

#ifdef CONFIG_USBDEV_ADVANCE_DESC

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01)
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_DESC_SIZ, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    0x09, USB_DESCRIPTOR_TYPE_INTERFACE, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,
    0x09, HID_DESCRIPTOR_TYPE_HID, 0x11, 0x01, 0x00, 0x01, 0x22, sizeof(hid_combined_report_desc), 0x00,
    0x07, USB_DESCRIPTOR_TYPE_ENDPOINT, HID_EP_ADDR, 0x03, HID_EP_SIZE, 0x00, HID_EP_INTERVAL,
};

static const uint8_t device_quality_descriptor[] = {
    0x0a, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },
    "CherryUSB",
    "HID Combo (KBD+Consumer, 6KRO)",
    "202412345678",
};

static const uint8_t *device_descriptor_callback(uint8_t speed) { (void)speed; return device_descriptor; }
static const uint8_t *config_descriptor_callback(uint8_t speed) { (void)speed; return config_descriptor; }
static const uint8_t *device_quality_descriptor_callback(uint8_t speed) { (void)speed; return device_quality_descriptor; }
static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index > 3) return NULL;
    return string_descriptors[index];
}

const struct usb_descriptor hid_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};
#else
#  error "This module requires CONFIG_USBDEV_ADVANCE_DESC"
#endif

typedef enum { HID_STATE_IDLE = 0, HID_STATE_BUSY = 1 } hid_state_t;

static volatile bool g_configured = false;
static volatile hid_state_t g_hid_state = HID_STATE_IDLE;
static rt_sem_t g_hid_complete_sem = RT_NULL;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buf[HID_EP_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct usbd_interface intf_hid;

/* ============================================================================
 * 6KRO 按键状态管理
 * ============================================================================ */

typedef struct {
    uint8_t modifier;                    /* 当前修饰键状态 */
    uint8_t keys[HID_KBD_MAX_KEYS];      /* 当前按下的keycodes */
    uint8_t key_count;                   /* 当前按下的按键数量 */
    rt_mutex_t lock;                     /* 线程安全锁 */
} kbd_state_t;

static kbd_state_t g_kbd_state = {0};

/* ============================================================================
 * ISR 安全辅助函数
 * ============================================================================ */

/**
 * @brief 检查是否在中断上下文中
 * @return true 在 ISR 中, false 在线程中
 */
static inline bool is_in_isr_context(void)
{
    return (rt_interrupt_get_nest() > 0);
}

/**
 * @brief ISR 安全的获取锁
 * @note 在 ISR 中使用中断屏蔽，在线程中使用 mutex
 * @return 原中断状态（用于 ISR）或 0（用于线程）
 */
static rt_base_t kbd_lock_acquire(void)
{
    if (is_in_isr_context()) {
        /* ISR 中：使用中断屏蔽保护临界区 */
        return rt_hw_interrupt_disable();
    } else {
        /* 线程中：使用 mutex */
        if (g_kbd_state.lock) {
            rt_mutex_take(g_kbd_state.lock, RT_WAITING_FOREVER);
        }
        return 0;
    }
}

/**
 * @brief ISR 安全的释放锁
 * @param level kbd_lock_acquire 返回的值
 */
static void kbd_lock_release(rt_base_t level)
{
    if (is_in_isr_context()) {
        /* ISR 中：恢复中断状态 */
        rt_hw_interrupt_enable(level);
    } else {
        /* 线程中：释放 mutex */
        if (g_kbd_state.lock) {
            rt_mutex_release(g_kbd_state.lock);
        }
    }
}

/* 初始化键盘状态 */
static void kbd_state_init(void)
{
    memset(&g_kbd_state, 0, sizeof(g_kbd_state));
    if (!g_kbd_state.lock) {
        g_kbd_state.lock = rt_mutex_create("kbd_st", RT_IPC_FLAG_PRIO);
    }
}

/**
 * @brief 清空键盘状态 - ISR 安全版本
 * @note 可以安全地在 ISR 或线程上下文中调用
 */
static void kbd_state_clear(void)
{
    rt_base_t level = kbd_lock_acquire();
    
    g_kbd_state.modifier = 0;
    memset(g_kbd_state.keys, 0, sizeof(g_kbd_state.keys));
    g_kbd_state.key_count = 0;
    
    kbd_lock_release(level);
}

/* 查找keycode在数组中的位置，返回-1表示未找到 */
static int kbd_find_key(uint8_t keycode)
{
    for (int i = 0; i < HID_KBD_MAX_KEYS; i++) {
        if (g_kbd_state.keys[i] == keycode) {
            return i;
        }
    }
    return -1;
}

/* 查找空闲位置，返回-1表示已满 */
static int kbd_find_empty_slot(void)
{
    for (int i = 0; i < HID_KBD_MAX_KEYS; i++) {
        if (g_kbd_state.keys[i] == 0) {
            return i;
        }
    }
    return -1;
}

/* ============================================================================
 * USB 事件处理
 * ============================================================================ */

/**
 * @brief USB 事件处理回调
 * @note 此函数在 ISR 上下文中被调用！不能使用 mutex
 */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    switch (event) {
    case USBD_EVENT_RESET:
    case USBD_EVENT_DISCONNECTED:
        g_configured = false;
        kbd_state_clear();  /* ISR 安全：内部使用中断屏蔽 */
        if (g_hid_state == HID_STATE_BUSY) {
            g_hid_state = HID_STATE_IDLE;
            if (g_hid_complete_sem) {
                rt_sem_release(g_hid_complete_sem);  /* rt_sem_release 在 ISR 中是安全的 */
            }
        } else {
            g_hid_state = HID_STATE_IDLE;
        }
        break;
    case USBD_EVENT_CONFIGURED:
        g_configured = true;
        g_hid_state = HID_STATE_IDLE;
        kbd_state_clear();  /* ISR 安全：内部使用中断屏蔽 */
        /* 注意：rt_kprintf 在 ISR 中可能不安全，移除或使用标志延迟打印 */
        break;
    default:
        break;
    }
}

/**
 * @brief HID 端点回调
 * @note 此函数在 ISR 上下文中被调用！
 */
static void hid_ep_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
    g_hid_state = HID_STATE_IDLE;
    if (g_hid_complete_sem) {
        rt_sem_release(g_hid_complete_sem);  /* rt_sem_release 在 ISR 中是安全的 */
    }
}

static struct usbd_endpoint ep_hid_in = { .ep_cb = hid_ep_cb, .ep_addr = HID_EP_ADDR };

bool hid_device_ready(void) { return g_configured; }

/* ============================================================================
 * 底层发送函数
 * ============================================================================ */

static int hid_send(const uint8_t *data, uint32_t len)
{
    if (!g_configured) {
        return -1;
    }
    if (!g_hid_complete_sem) {
        return -1;
    }
    if (len > HID_EP_SIZE) {
        return -1;
    }
    if (g_hid_state == HID_STATE_BUSY) {
        return -1;
    }

    memcpy(hid_buf, data, len);

    g_hid_state = HID_STATE_BUSY;
    int ret = usbd_ep_start_write(0, HID_EP_ADDR, hid_buf, len);
    if (ret < 0) {
        g_hid_state = HID_STATE_IDLE;
        return ret;
    }

    rt_err_t sem_result = rt_sem_take(g_hid_complete_sem, rt_tick_from_millisecond(1000));
    if (sem_result != RT_EOK) {
        return -1;
    }
    
    return 0;
}

void hid_reset_semaphore(void) 
{
    if (!g_hid_complete_sem) {
        return;
    }
    g_hid_state = HID_STATE_IDLE;
}

bool hid_is_busy(void)
{
    return (g_hid_state == HID_STATE_BUSY);
}

/* ============================================================================
 * 键盘报告发送 - 支持6KRO
 * ============================================================================ */

/**
 * @brief 发送完整的键盘报告（使用当前状态）
 * 
 * 报告格式 (9 bytes):
 * [0] Report ID = 0x01
 * [1] Modifier keys
 * [2] Reserved = 0x00
 * [3-8] Keycodes (6 keys max)
 */
static int kbd_send_current_state(void)
{
    uint8_t rpt[HID_EP_SIZE] = {0};
    
    rpt[0] = 0x01;  /* Report ID */
    rpt[1] = g_kbd_state.modifier;
    rpt[2] = 0x00;  /* Reserved */
    
    /* 复制当前按下的所有keycodes */
    for (int i = 0; i < HID_KBD_MAX_KEYS; i++) {
        rpt[3 + i] = g_kbd_state.keys[i];
    }
    
    return hid_send(rpt, HID_EP_SIZE);
}

/* 旧版单键报告（保持兼容） */
static int kbd_send_report(uint8_t modifier, uint8_t keycode)
{
    uint8_t rpt[HID_EP_SIZE] = {0};
    rpt[0] = 0x01;
    rpt[1] = modifier;
    rpt[2] = 0x00;
    rpt[3] = keycode;
    return hid_send(rpt, HID_EP_SIZE);
}

static int cons_send_report(uint8_t bits)
{
    uint8_t rpt[2];
    rpt[0] = 0x02;
    rpt[1] = bits & 0x1F;
    return hid_send(rpt, sizeof(rpt));
}

/* ============================================================================
 * 公共API - 原有接口（保持向后兼容）
 * ============================================================================ */

void hid_kbd_send(uint8_t modifier, uint8_t keycode)
{
    kbd_send_report(modifier, keycode);
}

/* 单键长按（旧API，会覆盖其他按键） */
void hid_kbd_press(uint8_t modifier, uint8_t keycode)
{
    kbd_send_report(modifier, keycode);
}

/* 释放所有按键（旧API） */
void hid_kbd_release(void)
{
    kbd_state_clear();
    kbd_send_report(0, 0);
}

void hid_kbd_send_combo(uint8_t modifier, uint8_t keycode)
{
    if (kbd_send_report(modifier, keycode) == 0) {
        rt_thread_mdelay(15);
        kbd_send_report(0, 0);
        rt_thread_mdelay(5);
    }
}

void hid_consumer_click(uint8_t bits)
{
    if (cons_send_report(bits) == 0) {
        rt_thread_mdelay(15);
        cons_send_report(0x00);
        rt_thread_mdelay(5);
    }
}

/* ============================================================================
 * 公共API - 新增6KRO接口
 * ============================================================================ */

int hid_kbd_key_down(uint8_t modifier, uint8_t keycode)
{
    if (!g_configured) {
        return -1;
    }
    
    rt_base_t level = kbd_lock_acquire();
    
    int ret = 0;
    
    /* 合并修饰键 */
    g_kbd_state.modifier |= modifier;
    
    /* 如果keycode非零且不在列表中，添加它 */
    if (keycode != 0) {
        int existing = kbd_find_key(keycode);
        if (existing < 0) {
            /* 按键不在列表中，找空位添加 */
            int slot = kbd_find_empty_slot();
            if (slot >= 0) {
                g_kbd_state.keys[slot] = keycode;
                g_kbd_state.key_count++;
            } else {
                /* 已满6个键 */
                ret = -2;
                rt_kprintf("[HID] 6KRO limit reached\n");
            }
        }
        /* 按键已存在，忽略重复按下 */
    }
    
    kbd_lock_release(level);
    
    /* 发送当前状态 */
    if (ret == 0) {
        ret = kbd_send_current_state();
    }
    
    return ret;
}

int hid_kbd_key_up(uint8_t modifier, uint8_t keycode)
{
    if (!g_configured) {
        return -1;
    }
    
    rt_base_t level = kbd_lock_acquire();
    
    /* 移除修饰键 */
    g_kbd_state.modifier &= ~modifier;
    
    /* 如果keycode非零，从列表中移除 */
    if (keycode != 0) {
        int slot = kbd_find_key(keycode);
        if (slot >= 0) {
            g_kbd_state.keys[slot] = 0;
            if (g_kbd_state.key_count > 0) {
                g_kbd_state.key_count--;
            }
        }
    }
    
    kbd_lock_release(level);
    
    /* 发送当前状态 */
    return kbd_send_current_state();
}

void hid_kbd_release_all(void)
{
    kbd_state_clear();
    kbd_send_current_state();
}

int hid_kbd_get_pressed_count(void)
{
    return g_kbd_state.key_count;
}

bool hid_kbd_is_key_pressed(uint8_t keycode)
{
    if (keycode == 0) return false;
    return (kbd_find_key(keycode) >= 0);
}

/* ============================================================================
 * 初始化
 * ============================================================================ */

void hid_device_init(uint8_t busid, uintptr_t reg_base)
{
    /* 初始化键盘状态 */
    kbd_state_init();
    
    if (g_hid_complete_sem == RT_NULL) {
        g_hid_complete_sem = rt_sem_create("hid_sem", 0, RT_IPC_FLAG_PRIO);
        if (g_hid_complete_sem == RT_NULL) {
            return;
        }
    }

#ifdef CONFIG_USBDEV_ADVANCE_DESC
    usbd_desc_register(busid, &hid_descriptor);
#else
#   error "CONFIG_USBDEV_ADVANCE_DESC is required"
#endif

    usbd_add_interface(busid, usbd_hid_init_intf(busid, &intf_hid,
                        hid_combined_report_desc, sizeof(hid_combined_report_desc)));
    usbd_add_endpoint(busid, &ep_hid_in);

    usbd_initialize(busid, reg_base, usbd_event_handler);
}