# SGTZ_001 电子秤上位机

Flutter 上位机用于 SGTZ_001 电子秤、压力检测风扇、身高体重 BMI、WiFi/蓝牙 APP 和视频监控套餐版本。

## 功能

- V1 查看压力、手动/自动模式和风扇状态。
- V2 查看重量、阈值和报警状态。
- V3/V4/V5/V6/V7 查看体重、身高、BMI、锁定状态和体型判断。
- V4 通过蓝牙 SPP 透传 JSON，V5/V6 通过 ESP8266 MQTT 收发 JSON。
- V6 显示外部 ESP32-CAM 上报的局域网 MJPEG 视频流。

## MQTT

- Broker：`121.40.131.194:1883`
- Client ID：`SGTZ_001_upper`
- 命令 Topic：`SGTZ_001/sub`
- 遥测 Topic：`SGTZ_001/pub`

固件遥测示例：

```json
{"type":"telemetry","device":"SGTZ_001","weight_g":65000,"height_cm":172,"bmi_x10":220,"state":"NORMAL","locked":1}
```
