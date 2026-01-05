/**
 * @file mp3_player_controller.h
 * @brief MP3 播放器核心控制模块头文件
 */

#ifndef __MP3_PLAYER_CONTROLLER_H__
#define __MP3_PLAYER_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief MP3 播放器 UI 状态
 */
typedef enum {
    MP3_UI_STATE_IDLE = 0,      /**< 空闲/停止 */
    MP3_UI_STATE_PLAYING,       /**< 播放中 */
    MP3_UI_STATE_PAUSED,        /**< 暂停 */
    MP3_UI_STATE_ERROR          /**< 错误状态 */
} mp3_ui_state_t;

/**
 * @brief MP3 播放器显示数据
 */
typedef struct {
    mp3_ui_state_t state;           /**< 播放状态 */
    uint8_t volume;                 /**< 音量 (0-15) */
    uint16_t total_songs;           /**< 歌曲总数 */
    uint16_t current_index;         /**< 当前歌曲索引 (1-based) */
    char current_song[64];          /**< 当前歌曲名 */
    uint8_t progress_percent;       /**< 播放进度百分比 (0-100) */
    uint32_t current_pos_sec;       /**< 当前播放位置 (秒) */
    uint32_t total_duration_sec;    /**< 歌曲总时长 (秒) */
    bool sd_card_ready;             /**< SD 卡是否就绪 */
    bool valid;                     /**< 数据是否有效 */
} mp3_display_data_t;

/*============================================================================
 * 公共 API
 *============================================================================*/

/**
 * @brief 初始化 MP3 播放器
 * @return 0 成功, 其他失败
 */
int mp3_player_init(void);

/**
 * @brief 反初始化 MP3 播放器
 */
void mp3_player_deinit(void);

/**
 * @brief 播放
 * @return 0 成功
 */
int mp3_player_play(void);

/**
 * @brief 暂停
 * @return 0 成功
 */
int mp3_player_pause(void);

/**
 * @brief 恢复播放
 * @return 0 成功
 */
int mp3_player_resume(void);

/**
 * @brief 停止
 * @return 0 成功
 */
int mp3_player_stop(void);

/**
 * @brief 下一曲
 * @return 0 成功
 */
int mp3_player_next(void);

/**
 * @brief 上一曲
 * @return 0 成功
 */
int mp3_player_prev(void);

/**
 * @brief 切换播放/暂停状态
 * @return 0 成功
 */
int mp3_player_toggle_play_pause(void);

/**
 * @brief 增加音量
 * @return 当前音量
 */
int mp3_player_volume_up(void);

/**
 * @brief 减少音量
 * @return 当前音量
 */
int mp3_player_volume_down(void);

/**
 * @brief 设置音量
 * @param vol 音量值 (0-15)
 * @return 0 成功
 */
int mp3_player_set_volume(uint8_t vol);

/**
 * @brief 刷新音乐列表
 * @return 歌曲数量
 */
int mp3_player_refresh_list(void);

/**
 * @brief 获取播放器显示数据
 * @param data 输出数据指针
 * @return 0 成功
 */
int mp3_player_get_data(mp3_display_data_t *data);

/**
 * @brief 重新扫描音乐目录
 */
int mp3_player_rescan(void);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_PLAYER_CONTROLLER_H__ */