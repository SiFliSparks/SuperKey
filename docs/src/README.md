---
home: true
icon: house
title: 主页
heroFullScreen: true

bgImage: /assets/image/bgImage.svg
bgImageDark: /assets/image/bgImageDark.svg
bgImageStyle:
  background-attachment: fixed
  background-size: cover
  background-position: center
  background-repeat: no-repeat
  opacity: 0.5
heroText: SuperKey-可视化宏键盘
tagline:  方寸之间 · 随心而动

heroImage: /assets/image/logo.svg
heroImageDark: /assets/image/logo-dark.svg
heroImageStyle:
  width: 130px
  height: 130px
heroAlt: SuperKey - 基于SF32的可视化宏键盘平台
actions:
  - text: 快速入门
    icon: lightbulb
    link: ./get-started/
    type: primary
  - text: 更新日志
    icon: clock-rotate-left
    link: ./custom/
    type: default
  - text: SDK文档
    icon: book
    link: https://docs.sifli.com/projects/sdk/latest/sf32lb52x/index.html
    type: default
  - text: 社区论坛
    icon: comments
    link: https://bbs.sifli.com/
    type: default

highlights:
  - header: 关于SuperKey
    description: 基于 RT-Thread 和 LVGL 图形库的嵌入式多功能桌面控制器项目，运行在 SF32LB52。上位机基于flet开发，可运行在Windows和MAC。硬件集成了三 LCD 屏幕显示、USB HID 键盘模拟、旋转编码器、RGB LED 灯效、温湿度传感器等功能，通过物理按键和旋钮实现丰富的交互操作。
    image: /assets/image/SuperKeyUI.png
    bgImage: https://theme-hope-assets.vuejs.press/bg/2-light.svg
    bgImageDark: https://theme-hope-assets.vuejs.press/bg/2-dark.svg
    highlights:
      - title: 屏幕显示系统
        icon: display
        details: 采用分层模块化设计，页面第一层级使用编码器切换，针对第一层级的功能扩展，通过按键进入第二层级

      - title: 按键与输入系统
        icon: keyboard
        details: 采用上下文切换机制，不同界面组激活不同的按键处理上下文，实现一键多用、灵活切换。

      - title: USB HID 功能
        icon: plug
        details: 支持预设快捷键、模拟键盘和自定义按键映射与存储。

      - title: LED 效果系统
        icon: palette
        details: RGB LED 效果系统支持呼吸灯、流水灯、彩虹渐变、波浪、闪烁等多种动态效果；内置 10+ 预设颜色和全局亮度调节，为按键操作提供即时视觉反馈，并可作为背景氛围灯营造桌面氛围。

      - title: 小工具系统
        icon: toolbox
        details: 实现多种小工具，通过按键逻辑可绑定到首页，快速开始使用。

      - title: 配套上位机SuperKeyHUB
        icon: desktop
        details: 基于python+flet，提供硬件监控、API信息中转，SuperKey设置和固件升级等功能

  - header: 页面与功能
    description: 5组界面 + 双层层级架构的分层模块化设计，页面第一层级使用编码器切换，针对第一层级的功能扩展，通过按键进入第二层级
    bgImage: https://theme-hope-assets.vuejs.press/bg/2-light.svg
    bgImageDark: https://theme-hope-assets.vuejs.press/bg/2-dark.svg
    bgImageStyle:
      background-size: cover
      background-position: center
      background-repeat: no-repeat
      background-attachment: fixed
      min-height: 600px
    highlights:
      - title: 时间/天气/小工具
        icon: cloud-sun
        details: 年月日周显示与全屏时钟页面，实时天气与天气预报，更有多种实用小工具等你探索。

      - title: 系统监控
        icon: chart-line
        details: 精准采集电脑CPU\GPU\内存\网络数据，系统运行情况一目了然。

      - title: 常用快捷按键
        icon: hand-pointer
        details: 媒体控制、网页控制、文本快捷键，复杂操作一触即达。

      - title: 实用工具
        icon: screwdriver-wrench
        details: 赛博木鱼、番茄钟、秒表，更多实用工具持续更新中。

      - title: 自定义按键
        icon: sliders
        details: 提供完整的键盘按键自定义功能，更有多达四组宏按键可供自由定义。

      - title: 个性化支持
        icon: arrow-up-from-bracket
        details: 提供自定义图片背景、自定义音效等多种个性化选择，同时可自行编译修改开源固件，千人千面，让你的灵感快速实现。

  - header: SuperKeyHUB
    description: 基于 Flet 框架开发的跨平台桌面应用程序，作为SuperKey的配套上位机软件。实现硬件监控数据推送、天气信息同步、自定义按键配置、LED 灯效控制及固件升级等功能，支持 Windows、macOS 和 Linux 平台。
    image: /assets/image/superkeyhub.png
    bgImage: https://theme-hope-assets.vuejs.press/bg/2-light.svg
    bgImageDark: https://theme-hope-assets.vuejs.press/bg/2-dark.svg
    highlights:
      - title: 硬件监控与天气服务
        icon: gauge-high
        details: 跨平台适配的CPU/GPU/内存/磁盘/网络实时采集，和风天气 API，200+城市，三日预报。

      - title: 自定义按键配置
        icon: keyboard
        details: 3个可编程键，预设快捷键模板，可使用键盘直接录入，多达四组按键宏连续执行。

      - title: 配置管理
        icon: floppy-disk
        details: 数据本地持久化，配置仅需一次。

      - title: 系统升级
        icon: arrow-up-from-bracket
        details: 支持系统版本检测与升级，一键固件烧录。

      - title: 设备链接
        icon: link
        details: 自动扫描设备、设备一键连接、意外断连后自动重连。

copyright: false
footer: 'Apache-2.0 Licensed | Copyright © 2025 思澈科技（南京）有限公司 | <span id="busuanzi_container_site_pv">总访问量 <span id="busuanzi_value_site_pv"></span> 次</span> | <span id="busuanzi_container_site_uv">访客数 <span id="busuanzi_value_site_uv"></span> 人</span>'
---