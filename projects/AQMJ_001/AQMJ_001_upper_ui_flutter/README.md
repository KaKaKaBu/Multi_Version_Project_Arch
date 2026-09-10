# AQMJ_001 Flutter 上位机

AQMJ_001 智能门禁上位机，基于 `common/flutter` 公共层实现 BLE、普通 WiFi/MQTT、华为云 IoTDA、视频监控视图、版本视图和控制页。

功能入口：

- 监测页：主人在家/外出、设防撤防、人体检测、报警、门铃时间和当前版本能力。
- 控制页：模拟门铃、清除报警、设防撤防、在家外出、灯光和留言控制。
- 视频页：视频监控版本显示 MJPEG 画面。

构建时可通过 `--dart-define` 指定版本：

```powershell
flutter build apk --dart-define=UPPER_VERSION=15 --dart-define=UPPER_FEATURES=common,remote,wifi,cloud,app,light,message
```
