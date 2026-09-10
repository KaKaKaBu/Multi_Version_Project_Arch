# YYYH_001 Flutter 上位机

YYYH_001 智能药盒上位机，基于 `common/flutter` 公共层实现 BLE、普通 WiFi/MQTT、华为云 IoTDA、视频监控视图、版本视图和控制页。

## 运行示例

```powershell
C:\Tools\flutter\bin\flutter.BAT pub get
C:\Tools\flutter\bin\flutter.BAT run --dart-define=UPPER_VERSION=24 --dart-define=UPPER_FEATURES=common,servo,hx711,tts,dht11,remote,wifi,cloud,app
```

BLE 版本示例：

```powershell
C:\Tools\flutter\bin\flutter.BAT run --dart-define=UPPER_VERSION=21 --dart-define=UPPER_FEATURES=common,servo,hx711,tts,dht11,remote,ble,app
```

视频版本示例：

```powershell
C:\Tools\flutter\bin\flutter.BAT run --dart-define=UPPER_VERSION=23 --dart-define=UPPER_FEATURES=common,servo,hx711,tts,dht11,remote,wifi,app,camera
```
