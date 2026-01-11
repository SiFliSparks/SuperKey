/**
 * @file mp3_player_controller.c
 * @brief MP3 播放器核心控制模块
 * 
 * 修复版本 - 使用 mp3ctrl 接口实现真正的音频播放
 * 
 * 功能:
 * - 扫描 SD 卡音乐文件
 * - 播放/暂停/停止/上一曲/下一曲
 * - 音量控制
 * - 后台播放支持
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include <string.h>
#include "../mp3/mp3_player_controller.h"

/* 条件编译 */
#if defined(BSP_USING_RTTHREAD) && !defined(CFG_BOOTLOADER)

/*============================================================================
 * 配置宏
 *============================================================================*/

#define MP3_MUSIC_DIR           "/sdcard/music"     /* 音乐文件目录 */
#define MP3_MAX_FILENAME_LEN    64                  /* 最大文件名长度 */
#define MP3_THREAD_STACK_SIZE   4096                /* 线程栈大小 - 增大以支持MP3解码 */
#define MP3_THREAD_PRIORITY     10                  /* 线程优先级 - 提高优先级 */
#define MP3_MSG_QUEUE_SIZE      8                   /* 消息队列大小 */
#define MP3_DEFAULT_VOLUME      4                   /* 默认音量 */
#define MP3_MAX_VOLUME          15                  /* 最大音量 */

/*============================================================================
 * 类型定义
 *============================================================================*/

/* 播放器命令 */
typedef enum {
    MP3_CMD_PLAY = 0,
    MP3_CMD_PAUSE,
    MP3_CMD_RESUME,
    MP3_CMD_STOP,
    MP3_CMD_NEXT,
    MP3_CMD_PREV,
    MP3_CMD_SONG_END,       /* 歌曲播放完毕 */
} mp3_cmd_t;

/* 播放器内部状态 */
typedef enum {
    MP3_STATE_IDLE = 0,
    MP3_STATE_PLAYING,
    MP3_STATE_PAUSED,
    MP3_STATE_STOPPED,
} mp3_state_t;

/* 音乐节点 */
typedef struct mp3_music_node {
    char filename[MP3_MAX_FILENAME_LEN];
    struct mp3_music_node *next;
    struct mp3_music_node *prev;
} mp3_music_node_t;

/* 播放器控制块 */
typedef struct {
    bool initialized;
    bool sd_card_ready;
    
    mp3_state_t state;
    uint8_t volume;
    
    mp3_music_node_t *music_list;       /* 音乐链表头 */
    mp3_music_node_t *current_song;     /* 当前播放歌曲 */
    uint16_t total_songs;
    uint16_t current_index;
    
    /* 播放进度跟踪 */
    uint32_t current_pos_sec;           /* 当前播放位置(秒) */
    uint32_t total_duration_sec;        /* 当前歌曲总时长(秒) */
    
    rt_thread_t thread;
    rt_mq_t msg_queue;
    rt_mutex_t mutex;
} mp3_player_t;

/*============================================================================
 * 全局变量
 *============================================================================*/

static mp3_player_t g_mp3_player = {0};

/* 前置声明 */
static void do_play(void);
static void do_pause(void);
static void do_resume(void);
static void do_stop(void);
static void do_next(void);
static void do_prev(void);
int mp3_player_stop(void);
int mp3_player_get_data(mp3_display_data_t *data);

/*============================================================================
 * 音频播放接口 - 使用 mp3ctrl (参考官方例程)
 *============================================================================*/

#include "audio_mp3ctrl.h"
#include "audio_server.h"

/* MP3 控制句柄 */
static mp3ctrl_handle g_mp3_handle = NULL;

/* 播放回调 - 处理播放进度和歌曲结束 */
static int mp3_play_callback_func(audio_server_callback_cmt_t cmd,
                                   void *callback_userdata, uint32_t reserved)
{
    (void)callback_userdata;
    
    switch (cmd) {
        case as_callback_cmd_play_to_end:
            rt_kprintf("[MP3_AUDIO] Song finished, auto next\n");
            /* 发送歌曲结束命令 */
            {
                mp3_cmd_t cmd_end = MP3_CMD_SONG_END;
                if (g_mp3_player.msg_queue) {
                    rt_mq_send(g_mp3_player.msg_queue, &cmd_end, sizeof(cmd_end));
                }
            }
            break;
            
        case as_callback_cmd_user:
            /* 播放进度回调 - reserved 参数包含当前播放秒数 */
            g_mp3_player.current_pos_sec = reserved;
            break;
            
        default:
            break;
    }
    
    return 0;
}

static int audio_play_file(const char *filepath)
{
    char full_path[128];
    rt_snprintf(full_path, sizeof(full_path), "%s/%s", MP3_MUSIC_DIR, filepath);
    
    rt_kprintf("[MP3_AUDIO] Playing: %s\n", full_path);
    
    /* 如果有正在播放的，先关闭 */
    if (g_mp3_handle != NULL) {
        mp3ctrl_close(g_mp3_handle);
        g_mp3_handle = NULL;
    }
    
    /* 获取歌曲信息(总时长) */
    mp3_info_t mp3_info = {0};
    if (mp3ctrl_getinfo(full_path, &mp3_info) == 0) {
        g_mp3_player.total_duration_sec = mp3_info.total_time_in_seconds;
        rt_kprintf("[MP3_AUDIO] Duration: %d sec (channels=%d, samplerate=%d)\n",
                   mp3_info.total_time_in_seconds, mp3_info.channels, mp3_info.samplerate);
    } else {
        g_mp3_player.total_duration_sec = 0;
        rt_kprintf("[MP3_AUDIO] Failed to get song info\n");
    }
    
    /* 重置当前播放位置 */
    g_mp3_player.current_pos_sec = 0;
    
    /* 打开并播放 */
    g_mp3_handle = mp3ctrl_open(AUDIO_TYPE_LOCAL_MUSIC, full_path, 
                                 mp3_play_callback_func, NULL);
    if (g_mp3_handle == NULL) {
        rt_kprintf("[MP3_AUDIO] Failed to open: %s\n", full_path);
        return -1;
    }
    
    int ret = mp3ctrl_play(g_mp3_handle);
    rt_kprintf("[MP3_AUDIO] mp3ctrl_play returned: %d\n", ret);
    
    return ret;
}

static int audio_stop(void)
{
    rt_kprintf("[MP3_AUDIO] Stopping\n");
    
    if (g_mp3_handle != NULL) {
        mp3ctrl_close(g_mp3_handle);
        g_mp3_handle = NULL;
    }
    
    return 0;
}

static int audio_pause(void)
{
    rt_kprintf("[MP3_AUDIO] Pausing\n");
    
    if (g_mp3_handle != NULL) {
        return mp3ctrl_pause(g_mp3_handle);
    }
    
    return -1;
}

static int audio_resume(void)
{
    rt_kprintf("[MP3_AUDIO] Resuming\n");
    
    if (g_mp3_handle != NULL) {
        return mp3ctrl_resume(g_mp3_handle);
    }
    
    return -1;
}

static int audio_set_volume(uint8_t vol)
{
    rt_kprintf("[MP3_AUDIO] Setting volume: %d\n", vol);
    return audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, vol);
}

/*============================================================================
 * 音乐列表管理
 *============================================================================*/

/* 检查是否为支持的音频文件 */
static bool is_audio_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    
    /* 跳过 macOS 隐藏文件 */
    if (filename[0] == '.' || strncmp(filename, "._", 2) == 0) {
        return false;
    }
    
    return (strcasecmp(ext, ".mp3") == 0 || 
            strcasecmp(ext, ".wav") == 0);
}

/* 添加歌曲到链表 */
static int add_song_to_list(const char *filename)
{
    mp3_music_node_t *node = rt_malloc(sizeof(mp3_music_node_t));
    if (!node) return -1;
    
    rt_memset(node, 0, sizeof(mp3_music_node_t));
    rt_strncpy(node->filename, filename, MP3_MAX_FILENAME_LEN - 1);
    
    /* 插入到链表尾部 */
    if (!g_mp3_player.music_list) {
        g_mp3_player.music_list = node;
        node->next = node;
        node->prev = node;
    } else {
        mp3_music_node_t *tail = g_mp3_player.music_list->prev;
        tail->next = node;
        node->prev = tail;
        node->next = g_mp3_player.music_list;
        g_mp3_player.music_list->prev = node;
    }
    
    g_mp3_player.total_songs++;
    return 0;
}

/* 清空音乐列表 */
static void clear_music_list(void)
{
    if (!g_mp3_player.music_list) return;
    
    mp3_music_node_t *head = g_mp3_player.music_list;
    mp3_music_node_t *node = head;
    
    do {
        mp3_music_node_t *next = node->next;
        rt_free(node);
        node = next;
    } while (node != head);
    
    g_mp3_player.music_list = NULL;
    g_mp3_player.current_song = NULL;
    g_mp3_player.total_songs = 0;
    g_mp3_player.current_index = 0;
}

/* 扫描音乐目录 */
static int scan_music_directory(void)
{
    DIR *dir;
    struct dirent *entry;
    
    clear_music_list();
    
    dir = opendir(MP3_MUSIC_DIR);
    if (!dir) {
        rt_kprintf("[MP3] Cannot open music directory: %s\n", MP3_MUSIC_DIR);
        g_mp3_player.sd_card_ready = false;
        return -1;
    }
    
    g_mp3_player.sd_card_ready = true;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && is_audio_file(entry->d_name)) {
            add_song_to_list(entry->d_name);
        }
    }
    
    closedir(dir);
    
    rt_kprintf("[MP3] Found %d songs\n", g_mp3_player.total_songs);
    
    if (g_mp3_player.total_songs > 0) {
        g_mp3_player.current_song = g_mp3_player.music_list;
        g_mp3_player.current_index = 1;
    }
    
    return g_mp3_player.total_songs;
}

/* 获取歌曲索引 */
static uint16_t get_song_index(mp3_music_node_t *node)
{
    if (!node || !g_mp3_player.music_list) return 0;
    
    uint16_t index = 1;
    mp3_music_node_t *p = g_mp3_player.music_list;
    
    while (p != node && index <= g_mp3_player.total_songs) {
        p = p->next;
        index++;
    }
    
    return index;
}

/*============================================================================
 * 播放控制
 *============================================================================*/

static void do_play(void)
{
    if (!g_mp3_player.current_song) {
        if (g_mp3_player.music_list) {
            g_mp3_player.current_song = g_mp3_player.music_list;
            g_mp3_player.current_index = 1;
        } else {
            return;
        }
    }
    
    audio_play_file(g_mp3_player.current_song->filename);
    g_mp3_player.state = MP3_STATE_PLAYING;
    g_mp3_player.current_index = get_song_index(g_mp3_player.current_song);
    
    rt_kprintf("[MP3] Now playing [%d/%d]: %s\n", 
               g_mp3_player.current_index,
               g_mp3_player.total_songs,
               g_mp3_player.current_song->filename);
}

static void do_pause(void)
{
    if (g_mp3_player.state == MP3_STATE_PLAYING) {
        audio_pause();
        g_mp3_player.state = MP3_STATE_PAUSED;
    }
}

static void do_resume(void)
{
    if (g_mp3_player.state == MP3_STATE_PAUSED) {
        audio_resume();
        g_mp3_player.state = MP3_STATE_PLAYING;
    }
}

static void do_stop(void)
{
    audio_stop();
    g_mp3_player.state = MP3_STATE_STOPPED;
}

static void do_next(void)
{
    if (!g_mp3_player.current_song) return;
    
    g_mp3_player.current_song = g_mp3_player.current_song->next;
    g_mp3_player.current_index = get_song_index(g_mp3_player.current_song);
    
    if (g_mp3_player.state == MP3_STATE_PLAYING || 
        g_mp3_player.state == MP3_STATE_PAUSED) {
        audio_stop();
        do_play();
    }
}

static void do_prev(void)
{
    if (!g_mp3_player.current_song) return;
    
    g_mp3_player.current_song = g_mp3_player.current_song->prev;
    g_mp3_player.current_index = get_song_index(g_mp3_player.current_song);
    
    if (g_mp3_player.state == MP3_STATE_PLAYING || 
        g_mp3_player.state == MP3_STATE_PAUSED) {
        audio_stop();
        do_play();
    }
}

/*============================================================================
 * 播放器线程
 *============================================================================*/

static void mp3_player_thread_entry(void *param)
{
    mp3_cmd_t cmd;
    
    rt_kprintf("[MP3] Player thread started\n");
    
    /* 初始扫描 */
    scan_music_directory();
    
    while (1) {
        if (rt_mq_recv(g_mp3_player.msg_queue, &cmd, sizeof(cmd), 
                       RT_WAITING_FOREVER) == RT_EOK) {
            
            rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
            
            switch (cmd) {
                case MP3_CMD_PLAY:
                    do_play();
                    break;
                case MP3_CMD_PAUSE:
                    do_pause();
                    break;
                case MP3_CMD_RESUME:
                    do_resume();
                    break;
                case MP3_CMD_STOP:
                    do_stop();
                    break;
                case MP3_CMD_NEXT:
                    do_next();
                    break;
                case MP3_CMD_PREV:
                    do_prev();
                    break;
                case MP3_CMD_SONG_END:
                    /* 自动播放下一首 */
                    do_next();
                    break;
            }
            
            rt_mutex_release(g_mp3_player.mutex);
        }
    }
}

/*============================================================================
 * 公共 API
 *============================================================================*/

/**
 * @brief 初始化 MP3 播放器
 */
int mp3_player_init(void)
{
    if (g_mp3_player.initialized) {
        return 0;
    }
    
    rt_kprintf("[MP3] Initializing player...\n");
    
    /* 初始化互斥锁 */
    g_mp3_player.mutex = rt_mutex_create("mp3_mtx", RT_IPC_FLAG_PRIO);
    if (!g_mp3_player.mutex) {
        rt_kprintf("[MP3] Failed to create mutex\n");
        return -1;
    }
    
    /* 初始化消息队列 */
    g_mp3_player.msg_queue = rt_mq_create("mp3_mq", sizeof(mp3_cmd_t), 
                                           MP3_MSG_QUEUE_SIZE, RT_IPC_FLAG_FIFO);
    if (!g_mp3_player.msg_queue) {
        rt_kprintf("[MP3] Failed to create message queue\n");
        rt_mutex_delete(g_mp3_player.mutex);
        return -1;
    }
    
    /* 创建播放器线程 */
    g_mp3_player.thread = rt_thread_create("mp3_player",
                                            mp3_player_thread_entry,
                                            NULL,
                                            MP3_THREAD_STACK_SIZE,
                                            MP3_THREAD_PRIORITY,
                                            10);
    if (!g_mp3_player.thread) {
        rt_kprintf("[MP3] Failed to create thread\n");
        rt_mq_delete(g_mp3_player.msg_queue);
        rt_mutex_delete(g_mp3_player.mutex);
        return -1;
    }
    
    g_mp3_player.volume = MP3_DEFAULT_VOLUME;
    audio_set_volume(g_mp3_player.volume);  /* 设置初始音量 */
    g_mp3_player.state = MP3_STATE_IDLE;
    g_mp3_player.initialized = true;
    
    rt_thread_startup(g_mp3_player.thread);
    
    rt_kprintf("[MP3] Player initialized\n");
    return 0;
}

/**
 * @brief 反初始化 MP3 播放器
 */
void mp3_player_deinit(void)
{
    if (!g_mp3_player.initialized) return;
    
    mp3_player_stop();
    
    if (g_mp3_player.thread) {
        rt_thread_delete(g_mp3_player.thread);
    }
    if (g_mp3_player.msg_queue) {
        rt_mq_delete(g_mp3_player.msg_queue);
    }
    if (g_mp3_player.mutex) {
        rt_mutex_delete(g_mp3_player.mutex);
    }
    
    clear_music_list();
    
    g_mp3_player.initialized = false;
}

/**
 * @brief 播放
 */
int mp3_player_play(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd = MP3_CMD_PLAY;
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 暂停
 */
int mp3_player_pause(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd = MP3_CMD_PAUSE;
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 恢复播放
 */
int mp3_player_resume(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd = MP3_CMD_RESUME;
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 停止
 */
int mp3_player_stop(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd = MP3_CMD_STOP;
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 下一曲
 */
int mp3_player_next(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd = MP3_CMD_NEXT;
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 上一曲
 */
int mp3_player_prev(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd = MP3_CMD_PREV;
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 切换播放/暂停
 */
int mp3_player_toggle_play_pause(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    mp3_cmd_t cmd;
    
    switch (g_mp3_player.state) {
        case MP3_STATE_PLAYING:
            cmd = MP3_CMD_PAUSE;
            break;
        case MP3_STATE_PAUSED:
            cmd = MP3_CMD_RESUME;
            break;
        default:
            cmd = MP3_CMD_PLAY;
            break;
    }
    
    return rt_mq_send(g_mp3_player.msg_queue, &cmd, sizeof(cmd));
}

/**
 * @brief 增加音量
 */
int mp3_player_volume_up(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
    
    if (g_mp3_player.volume < MP3_MAX_VOLUME) {
        g_mp3_player.volume++;
        audio_set_volume(g_mp3_player.volume);
    }
    
    rt_mutex_release(g_mp3_player.mutex);
    return g_mp3_player.volume;
}

/**
 * @brief 减少音量
 */
int mp3_player_volume_down(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
    
    if (g_mp3_player.volume > 0) {
        g_mp3_player.volume--;
        audio_set_volume(g_mp3_player.volume);
    }
    
    rt_mutex_release(g_mp3_player.mutex);
    return g_mp3_player.volume;
}

/**
 * @brief 设置音量
 */
int mp3_player_set_volume(uint8_t vol)
{
    if (!g_mp3_player.initialized) return -1;
    
    if (vol > MP3_MAX_VOLUME) vol = MP3_MAX_VOLUME;
    
    rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
    g_mp3_player.volume = vol;
    audio_set_volume(vol);
    rt_mutex_release(g_mp3_player.mutex);
    
    return 0;
}

/**
 * @brief 获取当前音量
 */
int mp3_player_get_volume(void)
{
    return g_mp3_player.volume;
}

/**
 * @brief 刷新音乐列表
 */
int mp3_player_refresh_list(void)
{
    if (!g_mp3_player.initialized) return -1;
    
    rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
    
    bool was_playing = (g_mp3_player.state == MP3_STATE_PLAYING);
    if (was_playing) {
        audio_stop();
    }
    
    scan_music_directory();
    
    rt_mutex_release(g_mp3_player.mutex);
    
    return g_mp3_player.total_songs;
}

/**
 * @brief 获取播放器显示数据
 */
int mp3_player_get_data(mp3_display_data_t *data)
{
    if (!data) return -1;
    
    rt_memset(data, 0, sizeof(mp3_display_data_t));
    
    if (!g_mp3_player.initialized) {
        data->valid = false;
        return -1;
    }
    
    rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
    
    /* 状态映射 */
    switch (g_mp3_player.state) {
        case MP3_STATE_PLAYING:
            data->state = MP3_UI_STATE_PLAYING;
            break;
        case MP3_STATE_PAUSED:
            data->state = MP3_UI_STATE_PAUSED;
            break;
        default:
            data->state = MP3_UI_STATE_IDLE;
            break;
    }
    
    data->volume = g_mp3_player.volume;
    data->total_songs = g_mp3_player.total_songs;
    data->current_index = g_mp3_player.current_index;
    data->sd_card_ready = g_mp3_player.sd_card_ready;
    data->valid = true;
    
    /* 获取播放进度和时长 */
    data->current_pos_sec = g_mp3_player.current_pos_sec;
    data->total_duration_sec = g_mp3_player.total_duration_sec;
    
    /* 计算进度百分比 */
    if (g_mp3_player.total_duration_sec > 0) {
        data->progress_percent = (uint8_t)((g_mp3_player.current_pos_sec * 100) / g_mp3_player.total_duration_sec);
        if (data->progress_percent > 100) {
            data->progress_percent = 100;
        }
    } else {
        data->progress_percent = 0;
    }
    
    if (g_mp3_player.current_song) {
        rt_strncpy(data->current_song, g_mp3_player.current_song->filename, 
                   sizeof(data->current_song) - 1);
    } else {
        rt_strncpy(data->current_song, "No music", sizeof(data->current_song) - 1);
    }
    
    rt_mutex_release(g_mp3_player.mutex);
    
    return 0;
}

/**
 * @brief 重新扫描音乐目录（SD卡热插拔时调用）
 */
int mp3_player_rescan(void)
{
    if (!g_mp3_player.initialized) {
        return -1;
    }
    
    rt_kprintf("[MP3] Rescanning music directory...\n");
    
    /* 停止当前播放 */
    if (g_mp3_player.state == MP3_STATE_PLAYING) {
        do_stop();
    }
    
    /* 重新扫描 */
    rt_mutex_take(g_mp3_player.mutex, RT_WAITING_FOREVER);
    int count = scan_music_directory();
    rt_mutex_release(g_mp3_player.mutex);
    
    return count;
}

#endif /* BSP_USING_RTTHREAD && !CFG_BOOTLOADER */