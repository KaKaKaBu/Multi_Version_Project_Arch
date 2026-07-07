# ZNCZ_001 智能插座上位机

Flutter 上位机用于 ZNCZ_001 智能插座 WiFi/MQTT 版本，支持继电器控制、定时开关配置、DS1302 校时和状态查看。

## 功能

- V1 查看压力、手动/自动模式和风扇状态。
- V2 查看重量、阈值和报警状态。
- V3/V4/V5/V6/V7 查看体重、身高、BMI、锁定状态和体型判断。
- V4 通过蓝牙 SPP 透传 JSON，V5/V6 通过 ESP8266 MQTT 收发 JSON。
- V6 显示外部 ESP32-CAM 上报的局域网 MJPEG 视频流。

## MQTT

- Broker：`121.40.131.194:1883`
- Client ID：`yskj_zncz_001_panel_<随机后缀>`
- 命令 Topic：`ZNCZ_001/web`
- 遥测 Topic：`ZNCZ_001`

固件遥测示例：

```json
{"relay":1,"mode":"manual","time":"12:30:45","on_time":"08:00:00","off_time":"22:00:00","wifi":"connected"}
```
