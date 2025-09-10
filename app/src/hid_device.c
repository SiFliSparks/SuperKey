#include "hid_device.h"
#include "rtthread.h"
#include "bf0_hal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include <string.h>

#define USBD_VID           0x1234
#define USBD_PID           0x5678
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 0x0409

#define HID_EP_ADDR          0x81
#define HID_EP_SIZE          9
#define HID_EP_INTERVAL      10

#define USB_HID_CONFIG_DESC_SIZ  34

static const uint8_t hid_combined_report_desc[] = {
    0x05, 0x01,
    0x09, 0x06,
    0xA1, 0x01,
    0x85, 0x01,
    0x05, 0x07,
    0x19, 0xE0,
    0x29, 0xE7,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x08,
    0x81, 0x03,
    0x95, 0x05,
    0x75, 0x01,
    0x05, 0x08,
    0x19, 0x01,
    0x29, 0x05,
    0x91, 0x02,
    0x95, 0x01,
    0x75, 0x03,
    0x91, 0x03,
    0x95, 0x06,
    0x75, 0x08,
    0x15, 0x00,
    0x25, 0xFF,
    0x05, 0x07,
    0x19, 0x00,
    0x29, 0x65,
    0x81, 0x00,
    0xC0,

    0x05, 0x0C,
    0x09, 0x01,
    0xA1, 0x01,
    0x85, 0x02,
    0x15, 0x00,
    0x25, 0x01,
    0x09, 0xE9,
    0x09, 0xEA,
    0x09, 0xCD,
    0x75, 0x01,
    0x95, 0x03,
    0x81, 0x02,
    0x95, 0x05,
    0x81, 0x03,
    0xC0
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
    "HID Combo (KBD+Consumer, ReportID)",
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
static volatile hid_state_t g_hid_state  = HID_STATE_IDLE;
static rt_sem_t g_hid_complete_sem = RT_NULL;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buf[HID_EP_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct usbd_interface intf_hid;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    switch (event) {
    case USBD_EVENT_RESET:
    case USBD_EVENT_DISCONNECTED:
        g_configured = false;
        g_hid_state = HID_STATE_IDLE;
        if (g_hid_complete_sem) rt_sem_release(g_hid_complete_sem);
        break;
    case USBD_EVENT_CONFIGURED:
        g_configured = true;
        g_hid_state = HID_STATE_IDLE;
        rt_kprintf("USB configured: HID combo ready.\n");
        break;
    default:
        break;
    }
}

static void hid_ep_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
    g_hid_state = HID_STATE_IDLE;
    if (g_hid_complete_sem) {
        rt_sem_release(g_hid_complete_sem);
    }
}

static struct usbd_endpoint ep_hid_in = { .ep_cb = hid_ep_cb, .ep_addr = HID_EP_ADDR };

bool hid_device_ready(void) { return g_configured; }

static int hid_send(const uint8_t *data, uint32_t len)
{
    if (!g_configured) {
        rt_kprintf("[hid] Device not configured\n");
        return -1;
    }
    if (!g_hid_complete_sem) {
        rt_kprintf("[hid] HID semaphore not initialized\n");
        return -1;
    }
    if (len > HID_EP_SIZE) {
        rt_kprintf("[hid] report too large: %lu\n", (unsigned long)len);
        return -1;
    }

    if (g_hid_state == HID_STATE_BUSY) {
        rt_kprintf("[hid] Device busy, rejecting request\n");
        return -1;
    }

    memcpy(hid_buf, data, len);

    g_hid_state = HID_STATE_BUSY;
    int ret = usbd_ep_start_write(0, HID_EP_ADDR, hid_buf, len);
    if (ret < 0) {
        g_hid_state = HID_STATE_IDLE;
        rt_kprintf("[hid] Failed to start write: %d\n", ret);
        return ret;
    }

    rt_err_t sem_result = rt_sem_take(g_hid_complete_sem, rt_tick_from_millisecond(500));
    if (sem_result != RT_EOK) {
        rt_kprintf("[hid] Semaphore timeout, forcing state reset\n");
        g_hid_state = HID_STATE_IDLE;
        
        int cleared = 0;
        while (rt_sem_take(g_hid_complete_sem, 0) == RT_EOK) {
            cleared++;
            if (cleared > 10) {
                rt_kprintf("[hid] Emergency break: cleared %d signals\n", cleared);
                break;
            }
        }
        
        if (cleared > 0) {
            rt_kprintf("[hid] Cleared %d excess semaphore signals\n", cleared);
        }
        
        return -1;
    }
    
    return 0;
}

void hid_reset_semaphore(void) 
{
    if (!g_hid_complete_sem) {
        rt_kprintf("[hid] Semaphore not initialized\n");
        return;
    }
    
    g_hid_state = HID_STATE_IDLE;
    
    int cleared = 0;
    while (rt_sem_take(g_hid_complete_sem, 0) == RT_EOK) {
        cleared++;
        if (cleared > 20) {
            rt_kprintf("[hid] Emergency: too many signals, possible corruption\n");
            break;
        }
    }
    
    if (cleared > 0) {
        rt_kprintf("[hid] Reset completed: cleared %d signals\n", cleared);
    } else {
        rt_kprintf("[hid] Reset completed: semaphore was clean\n");
    }
}

bool hid_is_busy(void)
{
    return (g_hid_state == HID_STATE_BUSY);
}

int hid_get_semaphore_count(void)
{
    if (!g_hid_complete_sem) {
        return -1;
    }
    
    int count = 0;
    while (rt_sem_take(g_hid_complete_sem, 0) == RT_EOK) {
        count++;
        if (count > 10) break;
    }
    
    for (int i = 0; i < count; i++) {
        rt_sem_release(g_hid_complete_sem);
    }
    
    return count;
}

static int kbd_send_report(uint8_t modifier, uint8_t keycode)
{
    uint8_t rpt[HID_EP_SIZE] = {0};
    rpt[0] = 0x01;
    rpt[1] = modifier;
    rpt[2] = 0x00;
    rpt[3] = keycode;
    return hid_send(rpt, 9);
}

static int cons_send_report(uint8_t bits)
{
    uint8_t rpt[2];
    rpt[0] = 0x02;
    rpt[1] = bits & 0x07;
    return hid_send(rpt, sizeof(rpt));
}

void hid_kbd_send(uint8_t modifier, uint8_t keycode)
{
    kbd_send_report(modifier, keycode);
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

void hid_device_init(uint8_t busid, uintptr_t reg_base)
{
    if (g_hid_complete_sem == RT_NULL) {
        g_hid_complete_sem = rt_sem_create("hid_sem", 0, RT_IPC_FLAG_PRIO);
        if (g_hid_complete_sem == RT_NULL) {
            rt_kprintf("[hid] Failed to create semaphore\n");
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

    rt_kprintf("[hid] HID combo device initialized successfully\n");
}