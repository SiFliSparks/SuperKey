/**
 * @file hid_cdc_composite.c
 * @brief HID + CDC ACM 复合USB设备实现
 * 
 * 端点分配:
 * - EP0: 控制端点 (默认)
 * - EP1 IN (0x81): HID 中断端点
 * - EP2 OUT (0x02): CDC 数据输出端点
 * - EP3 IN (0x83): CDC 数据输入端点
 * - EP4 IN (0x84): CDC 中断端点 (通知)
 */

#include "hid_cdc_composite.h"
#include "rtthread.h"
#include "bf0_hal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usbd_cdc_acm.h"
#include <string.h>
#include <rthw.h>

/* ============================================================================
 * USB 描述符定义
 * ============================================================================ */

/* USB VID/PID - 与原HID设备保持一致 */
#define USBD_VID           0x38f4
#define USBD_PID           0x1000
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 0x0409

/* 端点地址定义 */
#define HID_INT_EP         0x81    /* HID 中断输入端点 */
#define CDC_INT_EP         0x86    /* CDC 中断端点 */
#define CDC_OUT_EP         0x03    /* CDC 数据输出端点 */
#define CDC_IN_EP          0x85    /* CDC 数据输入端点 */

/* 端点大小 */
#define HID_INT_EP_SIZE    9
#define HID_INT_EP_INTERVAL 10

#ifdef CONFIG_USB_HS
    #define CDC_MAX_MPS    512
#else
    #define CDC_MAX_MPS    64
#endif

/* 接口编号 */
#define HID_INTF_NUM       0       /* HID 接口 */
#define CDC_INTF0_NUM      1       /* CDC 控制接口 */
#define CDC_INTF1_NUM      2       /* CDC 数据接口 */

/* 描述符长度计算 */
#define HID_DESCRIPTOR_LEN    (9 + 9 + 7)  /* Interface + HID + Endpoint */
#define USB_CONFIG_SIZE       (9 + HID_DESCRIPTOR_LEN + CDC_ACM_DESCRIPTOR_LEN)

/* ============================================================================
 * HID 报告描述符
 * ============================================================================ */

static const uint8_t hid_report_desc[] = {
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

/* ============================================================================
 * USB 配置描述符 (复合设备)
 * ============================================================================ */

#ifdef CONFIG_USBDEV_ADVANCE_DESC

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t config_descriptor[] = {
    /* Configuration Descriptor */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    
    /* ===================== HID Interface ===================== */
    /* Interface Descriptor */
    0x09,                           /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE,  /* bDescriptorType */
    HID_INTF_NUM,                   /* bInterfaceNumber */
    0x00,                           /* bAlternateSetting */
    0x01,                           /* bNumEndpoints */
    0x03,                           /* bInterfaceClass: HID */
    0x00,                           /* bInterfaceSubClass */
    0x00,                           /* bInterfaceProtocol */
    0x00,                           /* iInterface */
    
    /* HID Descriptor */
    0x09,                           /* bLength */
    HID_DESCRIPTOR_TYPE_HID,        /* bDescriptorType */
    0x11, 0x01,                     /* bcdHID: 1.11 */
    0x00,                           /* bCountryCode */
    0x01,                           /* bNumDescriptors */
    0x22,                           /* bDescriptorType: Report */
    sizeof(hid_report_desc), 0x00,  /* wDescriptorLength */
    
    /* HID Interrupt IN Endpoint */
    0x07,                           /* bLength */
    USB_DESCRIPTOR_TYPE_ENDPOINT,   /* bDescriptorType */
    HID_INT_EP,                     /* bEndpointAddress */
    0x03,                           /* bmAttributes: Interrupt */
    HID_INT_EP_SIZE, 0x00,          /* wMaxPacketSize */
    HID_INT_EP_INTERVAL,            /* bInterval */
    
    /* ===================== CDC ACM Interface ===================== */
    CDC_ACM_DESCRIPTOR_INIT(CDC_INTF0_NUM, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x00)
};

static const uint8_t device_quality_descriptor[] = {
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02,
    0xEF, 0x02, 0x01,
    0x40,
    0x00,
    0x00,
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },           /* Langid */
    "SiFli",                                 /* Manufacturer */
    "HID+CDC Composite Device",              /* Product */
    "202501100001",                          /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= sizeof(string_descriptors) / sizeof(string_descriptors[0])) {
        return NULL;
    }
    return string_descriptors[index];
}

static const struct usb_descriptor composite_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

#else
#error "This module requires CONFIG_USBDEV_ADVANCE_DESC"
#endif

/* ============================================================================
 * 数据缓冲区
 * ============================================================================ */

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buf[HID_INT_EP_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_read_buffer[CDC_RX_BUFFER_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_write_buffer[CDC_TX_BUFFER_SIZE];

/* ============================================================================
 * 状态变量
 * ============================================================================ */

typedef enum { 
    HID_STATE_IDLE = 0, 
    HID_STATE_BUSY = 1 
} hid_state_t;

static volatile bool g_usb_configured = false;
static volatile hid_state_t g_hid_state = HID_STATE_IDLE;
static volatile bool g_cdc_tx_busy = false;
static volatile bool g_cdc_dtr_enabled = false;

static rt_sem_t g_hid_complete_sem = RT_NULL;
static rt_sem_t g_cdc_tx_sem = RT_NULL;

static cdc_rx_callback_t g_cdc_rx_callback = NULL;

/* ============================================================================
 * 6KRO 键盘状态管理
 * ============================================================================ */

typedef struct {
    uint8_t modifier;
    uint8_t keys[HID_KBD_MAX_KEYS];
    uint8_t key_count;
    rt_mutex_t lock;
} kbd_state_t;

static kbd_state_t g_kbd_state = {0};

/* ISR安全辅助函数 */
static inline bool is_in_isr_context(void)
{
    return (rt_interrupt_get_nest() > 0);
}

static rt_base_t kbd_lock_acquire(void)
{
    if (is_in_isr_context()) {
        return rt_hw_interrupt_disable();
    } else {
        if (g_kbd_state.lock) {
            rt_mutex_take(g_kbd_state.lock, RT_WAITING_FOREVER);
        }
        return 0;
    }
}

static void kbd_lock_release(rt_base_t level)
{
    if (is_in_isr_context()) {
        rt_hw_interrupt_enable(level);
    } else {
        if (g_kbd_state.lock) {
            rt_mutex_release(g_kbd_state.lock);
        }
    }
}

static void kbd_state_init(void)
{
    memset(&g_kbd_state, 0, sizeof(g_kbd_state));
    if (!g_kbd_state.lock) {
        g_kbd_state.lock = rt_mutex_create("kbd_st", RT_IPC_FLAG_PRIO);
    }
}

static void kbd_state_clear(void)
{
    rt_base_t level = kbd_lock_acquire();
    g_kbd_state.modifier = 0;
    memset(g_kbd_state.keys, 0, sizeof(g_kbd_state.keys));
    g_kbd_state.key_count = 0;
    kbd_lock_release(level);
}

static int kbd_find_key(uint8_t keycode)
{
    for (int i = 0; i < HID_KBD_MAX_KEYS; i++) {
        if (g_kbd_state.keys[i] == keycode) {
            return i;
        }
    }
    return -1;
}

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
 * USB 接口结构
 * ============================================================================ */

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct usbd_interface intf_hid;
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct usbd_interface intf_cdc_ctrl;
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct usbd_interface intf_cdc_data;

/* ============================================================================
 * USB 事件处理
 * ============================================================================ */

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    
    switch (event) {
    case USBD_EVENT_RESET:
    case USBD_EVENT_DISCONNECTED:
        g_usb_configured = false;
        g_cdc_dtr_enabled = false;
        kbd_state_clear();
        
        if (g_hid_state == HID_STATE_BUSY) {
            g_hid_state = HID_STATE_IDLE;
            if (g_hid_complete_sem) {
                rt_sem_release(g_hid_complete_sem);
            }
        }
        
        if (g_cdc_tx_busy) {
            g_cdc_tx_busy = false;
            if (g_cdc_tx_sem) {
                rt_sem_release(g_cdc_tx_sem);
            }
        }
        break;
        
    case USBD_EVENT_CONFIGURED:
        g_usb_configured = true;
        g_hid_state = HID_STATE_IDLE;
        g_cdc_tx_busy = false;
        kbd_state_clear();
        
        /* 清空CDC TX信号量，确保初始状态正确 */
        if (g_cdc_tx_sem) {
            /* 尝试清空可能残留的信号量 */
            while (rt_sem_trytake(g_cdc_tx_sem) == RT_EOK) {
                /* 清空 */
            }
        }
        
        /* 启动 CDC 接收 */
        usbd_ep_start_read(busid, CDC_OUT_EP, cdc_read_buffer, CDC_RX_BUFFER_SIZE);
        break;
        
    default:
        break;
    }
}

/* ============================================================================
 * HID 端点回调
 * ============================================================================ */

static void hid_int_ep_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
    
    g_hid_state = HID_STATE_IDLE;
    if (g_hid_complete_sem) {
        rt_sem_release(g_hid_complete_sem);
    }
}

static struct usbd_endpoint hid_int_ep = {
    .ep_addr = HID_INT_EP,
    .ep_cb = hid_int_ep_cb
};

/* ============================================================================
 * CDC 端点回调
 * ============================================================================ */

static void cdc_bulk_out_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;
    
    /* 调用用户回调 */
    if (g_cdc_rx_callback && nbytes > 0) {
        g_cdc_rx_callback(cdc_read_buffer, nbytes);
    }
    
    /* 继续接收 */
    usbd_ep_start_read(busid, CDC_OUT_EP, cdc_read_buffer, CDC_RX_BUFFER_SIZE);
}

static void cdc_bulk_in_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;
    
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        /* 发送 ZLP */
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
    } else {
        g_cdc_tx_busy = false;
        if (g_cdc_tx_sem) {
            rt_sem_release(g_cdc_tx_sem);
        }
    }
}

static struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = cdc_bulk_out_cb
};

static struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = cdc_bulk_in_cb
};

/* ============================================================================
 * CDC ACM 控制请求处理
 * ============================================================================ */

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    (void)intf;
    g_cdc_dtr_enabled = dtr;
}

void usbd_cdc_acm_set_rts(uint8_t busid, uint8_t intf, bool rts)
{
    (void)busid;
    (void)intf;
    (void)rts;
    /* 可以根据需要处理 RTS */
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    (void)intf;
    (void)line_coding;
    /* 可以根据需要处理波特率等设置 */
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    (void)intf;
    
    line_coding->dwDTERate = 115200;
    line_coding->bDataBits = 8;
    line_coding->bParityType = 0;
    line_coding->bCharFormat = 0;
}

void usbd_cdc_acm_send_break(uint8_t busid, uint8_t intf)
{
    (void)busid;
    (void)intf;
}

/* ============================================================================
 * HID 底层发送
 * ============================================================================ */

static int hid_send(const uint8_t *data, uint32_t len)
{
    if (!g_usb_configured) {
        return -1;
    }
    if (!g_hid_complete_sem) {
        return -1;
    }
    if (len > HID_INT_EP_SIZE) {
        return -1;
    }
    if (g_hid_state == HID_STATE_BUSY) {
        return -1;
    }

    memcpy(hid_buf, data, len);

    g_hid_state = HID_STATE_BUSY;
    int ret = usbd_ep_start_write(0, HID_INT_EP, hid_buf, len);
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

static int kbd_send_current_state(void)
{
    uint8_t rpt[HID_INT_EP_SIZE] = {0};
    
    rpt[0] = 0x01;  /* Report ID */
    rpt[1] = g_kbd_state.modifier;
    rpt[2] = 0x00;  /* Reserved */
    
    for (int i = 0; i < HID_KBD_MAX_KEYS; i++) {
        rpt[3 + i] = g_kbd_state.keys[i];
    }
    
    return hid_send(rpt, HID_INT_EP_SIZE);
}

static int kbd_send_report(uint8_t modifier, uint8_t keycode)
{
    uint8_t rpt[HID_INT_EP_SIZE] = {0};
    rpt[0] = 0x01;
    rpt[1] = modifier;
    rpt[2] = 0x00;
    rpt[3] = keycode;
    return hid_send(rpt, HID_INT_EP_SIZE);
}

static int cons_send_report(uint8_t bits)
{
    uint8_t rpt[2];
    rpt[0] = 0x02;
    rpt[1] = bits & 0x1F;
    return hid_send(rpt, sizeof(rpt));
}

/* ============================================================================
 * HID 公共 API
 * ============================================================================ */

bool hid_device_ready(void)
{
    return g_usb_configured;
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

void hid_kbd_send(uint8_t modifier, uint8_t keycode)
{
    kbd_send_report(modifier, keycode);
}

void hid_kbd_press(uint8_t modifier, uint8_t keycode)
{
    kbd_send_report(modifier, keycode);
}

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

int hid_kbd_key_down(uint8_t modifier, uint8_t keycode)
{
    if (!g_usb_configured) {
        return -1;
    }
    
    rt_base_t level = kbd_lock_acquire();
    
    int ret = 0;
    g_kbd_state.modifier |= modifier;
    
    if (keycode != 0) {
        int existing = kbd_find_key(keycode);
        if (existing < 0) {
            int slot = kbd_find_empty_slot();
            if (slot >= 0) {
                g_kbd_state.keys[slot] = keycode;
                g_kbd_state.key_count++;
            } else {
                ret = -2;
            }
        }
    }
    
    kbd_lock_release(level);
    
    if (ret == 0) {
        ret = kbd_send_current_state();
    }
    
    return ret;
}

int hid_kbd_key_up(uint8_t modifier, uint8_t keycode)
{
    if (!g_usb_configured) {
        return -1;
    }
    
    rt_base_t level = kbd_lock_acquire();
    
    g_kbd_state.modifier &= ~modifier;
    
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
 * CDC 公共 API
 * ============================================================================ */

bool cdc_device_ready(void)
{
    return g_usb_configured && g_cdc_dtr_enabled;
}

int cdc_write(const uint8_t *data, uint32_t len)
{
    if (!g_usb_configured) {
        return -1;
    }
    if (!g_cdc_dtr_enabled) {
        /* 主机未打开串口，不发送 */
        return -2;
    }
    if (!g_cdc_tx_sem) {
        return -3;
    }
    if (len == 0) {
        return 0;
    }
    if (len > CDC_TX_BUFFER_SIZE) {
        len = CDC_TX_BUFFER_SIZE;
    }
    
    /* 如果上一次发送还在进行，等待完成 */
    if (g_cdc_tx_busy) {
        rt_err_t result = rt_sem_take(g_cdc_tx_sem, rt_tick_from_millisecond(500));
        if (result != RT_EOK) {
            /* 超时，重置状态 */
            g_cdc_tx_busy = false;
            return -4;
        }
    }
    
    memcpy(cdc_write_buffer, data, len);
    
    g_cdc_tx_busy = true;
    int ret = usbd_ep_start_write(0, CDC_IN_EP, cdc_write_buffer, len);
    if (ret < 0) {
        g_cdc_tx_busy = false;
        return ret;
    }
    
    /* 等待发送完成 */
    rt_err_t result = rt_sem_take(g_cdc_tx_sem, rt_tick_from_millisecond(500));
    if (result != RT_EOK) {
        /* 超时，重置状态 */
        g_cdc_tx_busy = false;
        return -5;
    }
    
    return (int)len;
}

int cdc_write_string(const char *str)
{
    if (!str) {
        return -1;
    }
    return cdc_write((const uint8_t *)str, strlen(str));
}

void cdc_set_rx_callback(cdc_rx_callback_t callback)
{
    g_cdc_rx_callback = callback;
}

bool cdc_is_tx_busy(void)
{
    return g_cdc_tx_busy;
}

/* ============================================================================
 * 复合设备初始化
 * ============================================================================ */

void hid_cdc_composite_init(uint8_t busid, uintptr_t reg_base)
{
    /* 初始化键盘状态 */
    kbd_state_init();
    
    /* 创建信号量 */
    if (g_hid_complete_sem == RT_NULL) {
        g_hid_complete_sem = rt_sem_create("hid_sem", 0, RT_IPC_FLAG_PRIO);
        if (g_hid_complete_sem == RT_NULL) {
            rt_kprintf("[USB] Failed to create HID semaphore\n");
            return;
        }
    }
    
    if (g_cdc_tx_sem == RT_NULL) {
        g_cdc_tx_sem = rt_sem_create("cdc_sem", 0, RT_IPC_FLAG_PRIO);
        if (g_cdc_tx_sem == RT_NULL) {
            rt_kprintf("[USB] Failed to create CDC semaphore\n");
            return;
        }
    }
    
    /* 注册描述符 */
#ifdef CONFIG_USBDEV_ADVANCE_DESC
    usbd_desc_register(busid, &composite_descriptor);
#else
#   error "CONFIG_USBDEV_ADVANCE_DESC is required"
#endif
    
    /* 添加 HID 接口 */
    usbd_add_interface(busid, usbd_hid_init_intf(busid, &intf_hid,
                        hid_report_desc, sizeof(hid_report_desc)));
    usbd_add_endpoint(busid, &hid_int_ep);
    
    /* 添加 CDC 接口 */
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf_cdc_ctrl));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf_cdc_data));
    usbd_add_endpoint(busid, &cdc_out_ep);
    usbd_add_endpoint(busid, &cdc_in_ep);
    
    /* 初始化 USB 设备 */
    usbd_initialize(busid, reg_base, usbd_event_handler);
    
    rt_kprintf("[USB] HID+CDC Composite device initialized\n");
}

/* ============================================================================
 * 兼容层：提供与原 hid_device.c 相同的初始化函数名
 * ============================================================================ */

void hid_device_init(uint8_t busid, uintptr_t reg_base)
{
    hid_cdc_composite_init(busid, reg_base);
}