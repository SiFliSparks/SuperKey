/**
 * @file serial_data_handler.h
 * @brief 串口数据处理器 - 支持UART和USB CDC双通道
 */

#ifndef SERIAL_DATA_HANDLER_H
#define SERIAL_DATA_HANDLER_H

#include <rtthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化串口数据处理器
 * @note 同时初始化 UART 和 CDC 双通道接收
 * @return RT_EOK 成功
 */
int serial_data_handler_init(void);

/**
 * @brief 反初始化串口数据处理器
 * @return RT_EOK 成功
 */
int serial_data_handler_deinit(void);

/**
 * @brief 发送响应字符串到上位机
 * @param response 响应字符串
 * @return 发送的字节数, <0 失败
 * @note 自动选择最后接收命令的通道进行发送
 */
int serial_send_response(const char *response);

/**
 * @brief 主动上报固件版本号
 * @return 0 成功, <0 失败
 */
int serial_report_firmware_version(void);

/**
 * @brief 检查连接状态
 * @return true 已连接, false 未连接
 */
bool serial_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_DATA_HANDLER_H */