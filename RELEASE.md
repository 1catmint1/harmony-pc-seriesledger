# SeriesLedger 1.0.3

本版统一应用商店名称、桌面标签、应用显示名称和主窗口标题为 SeriesLedger，保留真实 InfluxDB 1.x 连接、资源浏览、查询和结果导出功能。

2026-10-16，USB 鸿蒙 PC 的 R3 验收通过生产连接管理器连接隔离的官方 InfluxDB 1.13.1 服务。实际 SHOW MEASUREMENTS 返回 cpu 和 memory 两项，双语句查询得到三行结果，语法错误显示请求失败并清空旧表，恢复合法查询重新显示三行。隐私与开源许可窗口显示 1.0.3。录屏为真实操作的四段剪辑，时长98.28秒，无音轨；未加入合成界面。

正式 HAP 和 APP 均经官方工具验证签名与嵌入 Profile，使用 release/app_gallery 分发类型，无设备绑定和额外 ACL。与本次 R3 验收包的21项运行时内容逐字节相同，APP内HAP也匹配；签名及打包元数据单独处理。

APP SHA256：5dd4959aeb59059935f9a00633afebfa107c50e643584174d6c5b05853d92193

源码继续保留 CymaticLabs MIT、入口 GPL-3.0-or-later 和 Qt 原许可。Qt5.15.12及对应 OpenHarmony Alpha_v9 适配源码包已随仓库提供。真机应用、测试数据库和本次USB反向端口均已按绑定释放。开发者中心的上传、审核和发布状态以平台回执分别记录。
