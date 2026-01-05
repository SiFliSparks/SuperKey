/**
 * @file mp3_screen_context.h
 * @brief MP3 页面按键上下文处理头文件
 * 
 * 修改版本 - 支持编码器控制音量和切歌:
 * - 新增编码器模式枚举
 * - 新增模式查询接口
 * - 新增编码器事件处理接口 (供screen.c调用)
 */

#ifndef __MP3_SCREEN_CONTEXT_H__
#define __MP3_SCREEN_CONTEXT_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MP3 编码器控制模式
 */
typedef enum {
    MP3_ENCODER_MODE_NONE = 0,      /**< 无模式 - 编码器控制屏幕切换 */
    MP3_ENCODER_MODE_TRACK,         /**< 切歌模式 - 编码器控制上下曲 */
    MP3_ENCODER_MODE_VOLUME,        /**< 音量模式 - 编码器控制音量 */
} mp3_encoder_mode_t;

/**
 * @brief 初始化 MP3 屏幕上下文
 * @return 0 成功
 */
int mp3_screen_context_init(void);

/**
 * @brief 反初始化 MP3 屏幕上下文
 */
void mp3_screen_context_deinit(void);

/**
 * @brief 激活 MP3 屏幕上下文
 */
void mp3_screen_context_activate(void);

/**
 * @brief 停用 MP3 屏幕上下文
 */
void mp3_screen_context_deactivate(void);

/**
 * @brief 检查上下文是否激活
 * @return true 激活
 */
bool mp3_screen_context_is_active(void);

/**
 * @brief MP3 页面按键处理函数
 * @param key_id 按键ID (0-3)
 * @param event 按键事件 (0=单击)
 * 
 * 按键布局:
 *   KEY0 (左板块): 单击进入/退出切歌模式
 *   KEY1 (中板块): 单击播放/暂停
 *   KEY2 (右板块): 单击进入/退出音量模式
 *   KEY3 (旋钮):   单击返回上一组
 */
void mp3_screen_key_handler(uint8_t key_id, uint8_t event);

/**
 * @brief 获取当前编码器模式
 * @return 当前模式
 */
mp3_encoder_mode_t mp3_screen_context_get_encoder_mode(void);

/**
 * @brief 检查编码器是否在MP3控制模式
 * @return true 如果编码器正在控制音量或切歌
 */
bool mp3_screen_context_is_encoder_active(void);

/**
 * @brief 检查编码器是否在MP3控制模式 (供screen.c调用)
 * @return true 如果在切歌或音量模式
 * 
 * 此函数类似于 widget_is_in_setting_mode()，用于让 screen.c 的
 * screen_encoder_event_handler 判断是否拦截编码器事件
 */
bool mp3_is_in_encoder_mode(void);

/**
 * @brief MP3 编码器事件处理 (供screen.c调用)
 * @param delta 编码器增量 (+1/-1 或更大值)
 * @return 0 成功处理, -1 未处理
 * 
 * 此函数类似于 widget_handle_encoder()，根据当前模式执行切歌或音量调节
 */
int mp3_handle_encoder(int delta);

/**
 * @brief MP3 编码器事件处理 (别名，供screen.c调用)
 * @param delta 编码器增量 (+1/-1)
 * @return 0 成功处理, -1 未处理
 */
int mp3_screen_context_handle_encoder(int delta);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_SCREEN_CONTEXT_H__ */