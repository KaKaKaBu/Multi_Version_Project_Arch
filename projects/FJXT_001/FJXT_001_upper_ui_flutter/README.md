# FJXT_001 车窗防夹上位机

Flutter 上位机用于 FJXT_001 车窗防夹系统的蓝牙、WiFi 和云平台版本。

## 功能

- 查看车窗状态、打开/关闭到位、防夹检测和声光提醒状态。
- 下发一键打开、一键关闭、开一点停、关一点停、停止命令。
- CAM 版本显示 ESP-CAM 上报的局域网 IP 和 stream 链接。

## MQTT

- Broker：`121.40.131.194:1883`
- Client ID：`FJXT_001_upper`
- 命令 Topic：`FJXT_001`
- 遥测 Topic：`FJXT_001/web`

CAM 信息上报示例：

```json
{"cmd":"camera_info","params":{"ip":"192.168.1.88","stream":"http://192.168.1.88:81/stream"}}
```
