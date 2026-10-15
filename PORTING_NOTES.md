# 适配依据与工程结构

本适配借鉴了本机已有的 Qt/OpenHarmony PC 工程，主要参考：

- `ohos_CodeCountTool/harmony_pc`：Qt Widgets + ArkTS XComponent + QPA 启动链路。
- `ohos_KeePassXC/harmony_pc`：项目内 `qtforharmony_sdk` 和 arm64-v8a native library 打包方式。
- `firebird/ohos`：Qt for OpenHarmony SDK 的目录化引用方式。

本项目采用同一条启动链路：

```text
EntryAbility.ets
  -> pages/Index.ets
  -> XComponent(libraryname: plugins_platforms_qopenharmony)
  -> qpa.startQtApplication(this)
  -> libentry.so
  -> Qt QApplication / MainWindow
```

Qt 业务代码不依赖原项目的 .NET、WinForms、InfluxData.Net 或 ScintillaNET；网络层改为 Qt Network，UI 改为 Qt Widgets，鸿蒙层只承担窗口宿主和生命周期转发。这样桌面 Qt 代码与 HarmonyOS 宿主解耦，后续也可以复用同一套 C++ 源码构建其他 Qt 桌面目标。

