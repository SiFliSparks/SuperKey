# SuperKey 1.4
- 基于SF32LB52的带屏多功能宏键盘 SuperKey 嵌入式设备的固件程序，基于 RT-Thread RTOS 和 LVGL V9 图形框架，驱动三连屏 GC9107 LCD、RGB LED、旋转编码器、SHT30 温湿度传感器等外设，通过 USB HID/CDC 复合设备与上位机通信。
- 本项目目前正在持续迭代，已实现包括桌面时钟、硬件监控、天气预报、HID全功能控制、多种小工具等诸多功能。
- 欢迎对此项目提出issue，我们将听取您的意见，对项目进行功能改进和更新，也欢迎参与到项目共建中。
- 项目Demo视频见[Bilibili_带屏多功能宏键盘](https://www.bilibili.com/video/BV1XLWGzPEb6) 视频内展示的固件版本为v1.0.0。 
- 点此进入[项目文档](https://sparks.sifli.com/projects/superkey/)

## 更新内容
2026年3月6日见更新内容[文档](https://sparks.sifli.com/projects/superkey/custom/newlab.html)

## 支持硬件
- 本项目硬件已开源至立创开源硬件平台
- [sf32-keyboard](https://oshwhub.com/sifli/sf32-keyboard)

## 上位机
- 本项目需要通过上位机进行数据采集和接收。
- 请移步至[SuperKeyHub](https://github.com/OpenSiFli/SuperKeyHub)

## 功能

- **三连屏 UI 系统** — 3 × 128×128 GC9107 LCD，SPI 总线共享，独立 CS 片选
  - 多屏幕组切换（天气/性能监控/传感器/自定义/游戏/音乐）
  - L1/L2 二级页面架构
  - 运行时屏幕旋转（0°/90°/180°/270°）
- **系统性能监控** — 实时显示 CPU/GPU/内存占用率、温度、频率、显存
  - 圆弧仪表 + 动态变色（青绿/黄/红）
  - 内存进度条 + 已用/总计数值
- **天气显示** — 当前天气 + 三日预报，支持 300+ 城市
- **HID 键盘** — USB HID 复合设备，支持自定义按键映射
- **LED 灯效** — WS2812B RGB LED，呼吸/流动/彩虹/静态等效果
- **MP3 播放器** — SD 卡音乐播放，独立控制界面
- **旋转编码器** — 滚动切换、按压确认交互
- **电源管理** — 跟随 PC 休眠/唤醒，PA08 背光 GPIO 控制
- **DFU 固件更新** — 通过 CDC 串口在线更新

## 项目结构

```
SuperKey/
├── SiFli-SDK/                    # SDK 子模块
├── app/
│   ├── boards/                   # 板级配置
│   │   └── sf32lb52-superkey/
│   ├── gc9107_Multi_screen/      # GC9107 三连屏 LCD 驱动
│   ├── src/
│   │   ├── main.c                # 主程序入口 + LVGL 主循环
│   │   ├── device/               # 设备驱动层
│   │   │   ├── serial_data_handler.c  # 串口命令解析（sys_set/sys_get）
│   │   │   ├── hid_cdc_composite.c    # USB HID+CDC 复合设备
│   │   │   ├── sht30_controller.c     # 温湿度传感器
│   │   │   ├── buttons_board.c        # 按键驱动
│   │   │   └── encoder_controller.c   # 旋转编码器
│   │   ├── screen/               # 屏幕 UI 系统
│   │   │   ├── screen_core.c     # 消息队列 + 屏幕组切换
│   │   │   ├── screen_ui_manager.c    # LVGL 控件更新 + 动态变色
│   │   │   └── screen_init/      # 各屏幕组 UI 构建
│   │   ├── manager/              # 业务管理层
│   │   │   ├── power_manager.c   # 电源管理（休眠/唤醒）
│   │   │   ├── led_effects_manager.c  # LED 灯效引擎
│   │   │   ├── data_manager.c    # 数据缓存
│   │   │   └── key_manager.c     # 按键事件管理
│   │   ├── middleware/           # 中间件
│   │   │   ├── event_bus.c       # 事件总线（发布/订阅）
│   │   │   └── app_controller.c  # 应用生命周期
│   │   ├── fs/                   # 文件系统
│   │   │   ├── fs_init.c         # Flash/SD 卡挂载
│   │   │   ├── custom_key_storage.c   # 按键配置持久化
│   │   │   └── dfu_trigger.c     # DFU 升级触发
│   │   ├── mp3/                  # MP3 播放器
│   │   ├── widget/               # 小组件系统
│   │   └── version/              # 固件版本管理
│   └── project/                  # SCons 构建配置
└── docs/                         # 项目文档（VuePress）
```
## 串口协议

设备通过 UART（1Mbps）和 USB CDC 双通道接收上位机命令：

```
sys_set cpu 45.2                  # CPU 使用率
sys_set cpu_temp 65.0             # CPU 温度
sys_set cpu_freq 4938             # CPU 频率 (MHz)
sys_set gpu 30.0                  # GPU 使用率
sys_set gpu_temp 55.0             # GPU 温度
sys_set gpu_mem_used 2.3          # 显存已用 (GB)
sys_set gpu_mem_total 12.0        # 显存总计 (GB)
sys_set mem 62.5                  # 内存占用率
sys_set mem_used 20.0             # 内存已用 (GB)
sys_set mem_total 32.0            # 内存总计 (GB)
sys_set power_mode sleep          # 进入休眠
sys_set power_mode normal         # 退出休眠
sys_set lcd_rotation 90           # 屏幕旋转
sys_set led_effect breathing      # LED 灯效
sys_get version                   # 查询固件版本
sys_get power_mode                # 查询电源状态
```

## 开源协议

[Apache-2.0](LICENSE)
