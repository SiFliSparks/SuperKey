/**
 * @file widget_context.h
 * @brief 小工具按键上下文处理 - 头文件
 */

#ifndef WIDGET_CONTEXT_H
#define WIDGET_CONTEXT_H

#include "button.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化小工具上下文
 * @return 0成功, <0失败
 */
int widget_context_init(void);

/**
 * @brief 反初始化小工具上下文
 */
void widget_context_deinit(void);

/**
 * @brief Group1第三板块(key_idx=2)按键处理
 * @param key_idx 按键索引
 * @param action 按键动作
 * @return 0成功, -1未处理
 */
int widget_handle_group1_key(int key_idx, button_action_t action);

/**
 * @brief L2小工具选择器按键处理
 * @param key_idx 按键索引
 * @param action 按键动作
 * @return 0成功
 */
int widget_handle_selector_key(int key_idx, button_action_t action);

/**
 * @brief 小工具编码器事件处理
 * @param delta 编码器增量 (+1右/-1左)
 * @return 0成功, -1未处理
 */
int widget_handle_encoder(int delta);

/**
 * @brief 获取当前小工具是否在设置模式
 * @return true在设置模式, false正常模式
 */
bool widget_is_in_setting_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* WIDGET_CONTEXT_H */
