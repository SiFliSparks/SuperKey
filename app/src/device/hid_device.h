/**
 * @file hid_device.h
 * @brief HID设备兼容层
 * 
 * 此文件作为兼容层，实际实现已迁移到 hid_cdc_composite.h/c
 * 包含此头文件即可获得与之前完全相同的HID API
 * 
 * 新增功能：
 * - CDC虚拟串口支持（通过 hid_cdc_composite.h）
 */

#ifndef HID_DEVICE_H
#define HID_DEVICE_H

/* 包含复合设备头文件，获得所有HID和CDC API */
#include "../device/hid_cdc_composite.h"

#endif /* HID_DEVICE_H */