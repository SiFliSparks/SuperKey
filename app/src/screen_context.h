#ifndef SCREEN_CONTEXT_H
#define SCREEN_CONTEXT_H

#include "key_manager.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化所有屏幕上下文
 * 
 * 注册各个屏幕组的按键处理上下文，包括：
 * - GROUP_1: 时间/天气/股票界面
 * - GROUP_2: CPU/内存/网络监控界面  
 * - GROUP_3: HID功能选择界面
 * - GROUP_4: 实用工具界面（木鱼/番茄钟/全屏图片）- 新增
 * 
 * @return 0成功，负数失败
 */
int screen_context_init_all(void);

/**
 * @brief 清理所有屏幕上下文
 * 
 * 注销所有已注册的按键处理上下文，包括L1和L2层级
 * 
 * @return 0成功，负数失败
 */
int screen_context_deinit_all(void);

/**
 * @brief 为指定屏幕组激活对应的按键上下文
 * 
 * @param group 要激活的屏幕组（包括新增的GROUP_4）
 * @return 0成功，负数失败
 */
int screen_context_activate_for_group(screen_group_t group);

/**
 * @brief 停用所有L1层级的按键上下文
 * 
 * @return 0成功，负数失败
 */
int screen_context_deactivate_all(void);

/**
 * @brief 为L2层级激活对应的按键上下文
 * 
 * 会先停用所有L1层级上下文，然后激活指定的L2上下文
 * 支持新增的木鱼、番茄钟、全屏图片L2组
 * 
 * @param l2_group L2层级组
 * @return 0成功，负数失败
 */
int screen_context_activate_for_level2(screen_l2_group_t l2_group);

/**
 * @brief 停用所有L2层级的按键上下文
 * 
 * @return 0成功，负数失败
 */
int screen_context_deactivate_level2(void);

/**
 * @brief 初始化背景呼吸灯效果系统
 * 
 * 启动蓝色背景呼吸灯，支持按键特效结束后自动恢复
 * 应该在系统启动完成后调用
 * 
 * @return 0成功，负数失败
 */
int screen_context_init_background_breathing(void);

/**
 * @brief 清理背景呼吸灯效果系统
 * 
 * 停止背景呼吸灯，应该在系统关闭时调用
 * 
 * @return 0成功，负数失败
 */
int screen_context_cleanup_background_breathing(void);

/**
 * @brief 在主循环中调用，处理背景恢复
 * 
 * 检查并处理背景呼吸灯的恢复请求
 */
void screen_context_process_background_restore(void);

/**
 * @brief 手动恢复背景呼吸灯效果
 * 
 * 紧急情况下使用，比如按键特效失效时手动恢复背景呼吸灯
 * 
 * @return 0成功，负数失败
 */
int screen_context_restore_background_breathing(void);

/**
 * @brief 木鱼敲击事件处理 - 新增
 * 
 * 处理木鱼图片点击事件：
 * - 计数器+1
 * - 触发LED特效（金色短暂闪烁）
 * - 更新显示
 * - 播放音效（如果启用）
 * 
 * @return 0成功，负数失败
 */
int screen_context_handle_muyu_tap(void);

/**
 * @brief 重置木鱼计数器事件处理 - 新增
 * 
 * 重置当前会话的木鱼敲击计数
 * 
 * @return 0成功，负数失败
 */
int screen_context_handle_muyu_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_CONTEXT_H */