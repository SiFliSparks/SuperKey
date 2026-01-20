/**
 * @file dfu_trigger.h
 * @brief DFU模式触发模块
 * 
 * 提供手动进入DFU升级模式的功能
 */

#ifndef __DFU_TRIGGER_H__
#define __DFU_TRIGGER_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置DFU更新标志位
 * @return 0:成功, 其他:失败
 */
int dfu_trigger_set_flags(void);

/**
 * @brief 进入DFU模式（设置标志位并重启）
 * @note 此函数调用后设备将重启，不会返回
 */
void dfu_trigger_enter(void);

/**
 * @brief 仅设置DFU标志位，不重启
 * @return 0:成功, 其他:失败
 * @note 设置后下次重启将进入DFU模式
 */
int dfu_trigger_set_flags_only(void);

#ifdef __cplusplus
}
#endif

#endif /* __DFU_TRIGGER_H__ */