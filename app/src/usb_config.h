/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

/* ================ USB common Configuration ================ */

#include <rtthread.h>

#define CONFIG_USB_PRINTF(...) rt_kprintf(__VA_ARGS__)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* data align size when use dma or use dcache */
#define CONFIG_USB_ALIGN_SIZE 4

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))

/* ================= USB Device Stack Configuration ================ */

/* Ep0 in and out transfer buffer */
#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 256
#endif

/* enable advance desc register api - 必须启用 */
#define CONFIG_USBDEV_ADVANCE_DESC

/* ================= USB Device Port Configuration ================*/

#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif

#ifndef CONFIG_USBDEV_EP_NUM
#define CONFIG_USBDEV_EP_NUM 4
#endif

/* ================= USB Host Stack Configuration ================ */
/* 
 * 重要说明：由于CherryUSB的架构问题，某些底层文件会无条件包含USB Host头文件
 * 因此我们必须定义这些宏以避免编译错误，即使我们不使用USB Host功能
 */

/* USB Host相关配置 - 定义为最小值以节省内存 */
#ifndef CONFIG_USBHOST_MAX_ENDPOINTS
#define CONFIG_USBHOST_MAX_ENDPOINTS 2
#endif

#ifndef CONFIG_USBHOST_DEV_NAMELEN
#define CONFIG_USBHOST_DEV_NAMELEN 16
#endif

#ifndef CONFIG_USBHOST_MAX_INTF_ALTSETTINGS
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 1
#endif

#ifndef CONFIG_USBHOST_MAX_INTERFACES
#define CONFIG_USBHOST_MAX_INTERFACES 1
#endif

#ifndef CONFIG_USBHOST_MAX_EHPORTS
#define CONFIG_USBHOST_MAX_EHPORTS 1
#endif

#ifndef CONFIG_USBHOST_MAX_BUS
#define CONFIG_USBHOST_MAX_BUS 1
#endif

/* 其他可能需要的USB Host配置 */
#ifndef CONFIG_USBHOST_MAX_EXTHUBS
#define CONFIG_USBHOST_MAX_EXTHUBS 1
#endif

#ifndef CONFIG_USBHOST_MAX_CDC_ACM_CLASS
#define CONFIG_USBHOST_MAX_CDC_ACM_CLASS 1
#endif

#ifndef CONFIG_USBHOST_MAX_HID_CLASS
#define CONFIG_USBHOST_MAX_HID_CLASS 1
#endif

#ifndef CONFIG_USBHOST_MAX_MSC_CLASS
#define CONFIG_USBHOST_MAX_MSC_CLASS 1
#endif

#ifndef CONFIG_USBHOST_MAX_AUDIO_CLASS
#define CONFIG_USBHOST_MAX_AUDIO_CLASS 1
#endif

#ifndef CONFIG_USBHOST_MAX_VIDEO_CLASS
#define CONFIG_USBHOST_MAX_VIDEO_CLASS 1
#endif

/* ---------------- MUSB Configuration for SiFli ---------------- */
#define CONFIG_USB_MUSB_SIFLI

/* ================= MUSB 端点配置 ================ */
/* 这是解决编译错误的关键配置 */
#ifndef CONFIG_USB_MUSB_EP_NUM
#define CONFIG_USB_MUSB_EP_NUM 8
#endif

/* 对于 SF32LB52x 系列，根据具体芯片型号配置 */
#ifdef SOC_SF32LB52X
    /* SF32LB52x 通常使用全速模式 */
    #undef CONFIG_USB_HS
#endif

#ifdef SOC_SF32LB58X
    /* SF32LB58x 支持高速模式 */
    #define CONFIG_USB_HS
#endif

/* ================= HID Device Configuration ================ */
#ifndef CONFIG_USBDEV_HID_MAX_INTF
#define CONFIG_USBDEV_HID_MAX_INTF 1
#endif

/* HID Descriptor Type - CherryUSB内部已定义，这里只是为了兼容性 */
#ifndef HID_DESCRIPTOR_TYPE_HID
#define HID_DESCRIPTOR_TYPE_HID 0x21
#endif

/* USB Memory Alignment Macros */
#ifndef USB_MEM_ALIGNX
#define USB_MEM_ALIGNX __attribute__((aligned(CONFIG_USB_ALIGN_SIZE)))
#endif

/* Helper macros - 仅在CherryUSB未定义时才定义 */
#ifndef WBVAL
#define WBVAL(x) (x & 0xFF), ((x >> 8) & 0xFF)
#endif

#ifndef DBVAL
#define DBVAL(x) (x & 0xFF), ((x >> 8) & 0xFF), ((x >> 16) & 0xFF), ((x >> 24) & 0xFF)
#endif

#endif /* CHERRYUSB_CONFIG_H */