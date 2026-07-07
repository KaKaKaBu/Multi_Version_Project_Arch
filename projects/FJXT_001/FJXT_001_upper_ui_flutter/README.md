# FJXT_001 车窗防夹上位机

Flutter 上位机用于 FJXT_001 车窗防夹系统的蓝牙、WiFi 和云平台版本。

## 功能

- 查看车窗状态、红外对管防夹检测和声光提醒状态。
- 下发一键打开、一键关闭、开一点停、关一点停、停止命令。
- CAM 版本显示 ESP-CAM 上报的局域网 IP 和 stream 链接。
- 连接入口保留普通 MQTT、华为云 IoTDA 和 BLE 三种能力，按版本特性启用。

## MQTT

- Broker：`121.40.131.194:1883`
- Client ID：`FJXT_001_upper`
- 命令 Topic：`FJXT_001`
- 遥测 Topic：`FJXT_001/web`

CAM 信息上报示例：

```json
{"cmd":"camera_info","params":{"ip":"192.168.1.88","stream":"http://192.168.1.88:81/stream"}}
```

## 华为云 IoTDA

- Host：`wss://7e87c47089.st1.iotda-device.cn-east-3.myhuaweicloud.com:443/mqtt`
- App Device ID：`6a476514e094d615924ed7ef_FJXT_app`
- App Device Secret：`FJXT_app`
- STM32 Device ID：`6a476514e094d615924ed7ef_FJXT_stm32`
- Service ID：`FJXT`
- App 发布控制 Topic：`/jiabailie/M2M/FJXT_app/up`
- App 订阅状态 Topic：`/jiabailie/M2M/FJXT_app/down`

上位机仍向业务层发送普通 FJXT 命令，华为云 Transport 会自动封装为：

```json
{"message":{"cmd":"open"}}
```

遥测兼容 IoTDA `services[].properties`、`message`、`params` 和普通 FJXT `data` 格式。
