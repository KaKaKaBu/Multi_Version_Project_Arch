# YYYH_001 智能药盒产品规格

YYYH_001 是智能药盒项目，固件使用 `APP_VERSION=1..24` 选择版本能力。当前实现统一纳入 STM32F103C8T6 固件框架；用户要求的“彩屏”统一按 OLED 实现，RTC 统一使用 DS1302，语音提醒统一使用串口 TTS 模块。早期 51 套餐在本规格中保留版本定义，当前固件以 STM32 兼容实现覆盖对应业务能力。

## 版本矩阵

| 版本 | 编号 | 功能组合 | 通信/备注 |
|---:|---|---|---|
| 1 | YYYH-001 / C20N | 3 次定时、校时、吃药检测、药品分类、药量显示、声光提醒、OLED | 本地版 |
| 2 | YYYH-002 / C20B | 版本 1 + 蓝牙 APP | JDY-31 蓝牙 |
| 3 | YYYH-003 / C20W | 版本 1 + WiFi APP | ESP8266 普通 MQTT |
| 4 | YYYH-004 / C20CAM | 版本 3 + 视频监控 | 视频由 ESP32-CAM/上位机负责，STM32 不上报视频字段 |
| 5 | YYYH-005 / S80N | STM32、DS1302、吃药检测、3 次定时、舵机开药盒、声光提醒、OLED | 本地版 |
| 6 | YYYH-006 / S80B | 版本 5 + 蓝牙 APP | JDY-31 蓝牙 |
| 7 | YYYH-007 / S80W | 版本 5 + WiFi APP | ESP8266 普通 MQTT |
| 8 | YYYH-008 / S80CAM | 版本 7 + 视频监控 | 视频由 ESP32-CAM/上位机负责 |
| 9 | YYYH-009 / S80云I | 版本 5 + 华为云 IoTDA | ESP8266 MQTT/HMAC |
| 10 | YYYH-010 / S81N | 版本 5 + HX711 剩余药量称重 | 本地版 |
| 11 | YYYH-011 / S81B | 版本 10 + 蓝牙 APP | JDY-31 蓝牙 |
| 12 | YYYH-012 / S81W | 版本 10 + WiFi APP | ESP8266 普通 MQTT |
| 13 | YYYH-013 / S81CAM | 版本 12 + 视频监控 | 视频由 ESP32-CAM/上位机负责 |
| 14 | YYYH-014 / S81云I | 版本 10 + 华为云 IoTDA | ESP8266 MQTT/HMAC |
| 15 | YYYH-015 / S82N | 版本 10 + TTS 语音提醒 | 本地版 |
| 16 | YYYH-016 / S82B | 版本 15 + 蓝牙 APP | JDY-31 蓝牙 |
| 17 | YYYH-017 / S82W | 版本 15 + WiFi APP | ESP8266 普通 MQTT |
| 18 | YYYH-018 / S82CAM | 版本 17 + 视频监控 | 视频由 ESP32-CAM/上位机负责 |
| 19 | YYYH-019 / S82云I | 版本 15 + 华为云 IoTDA | ESP8266 MQTT/HMAC |
| 20 | YYYH-020 / S83N | 版本 15 + DHT11 温湿度 | 本地版 |
| 21 | YYYH-021 / S83B | 版本 20 + 蓝牙 APP | JDY-31 蓝牙 |
| 22 | YYYH-022 / S83W | 版本 20 + WiFi APP | ESP8266 普通 MQTT |
| 23 | YYYH-023 / S83CAM | 版本 22 + 视频监控 | 视频由 ESP32-CAM/上位机负责 |
| 24 | YYYH-024 / S83云I | 版本 20 + 华为云 IoTDA | ESP8266 MQTT/HMAC |

## 固件功能

- 通过 DS1302 读取和设置实时时钟，内置 3 组吃药定时；每组定时包含启用、小时、分钟、剂量、药品分类。
- 到达启用定时时间后，蜂鸣器和 LED 声光提醒；STM32 版本同时驱动 SG90 舵机打开药盒。
- 红外对管取药检测注册为 `presence`，检测到取药后停止报警、关闭药盒，并按剂量扣减对应药品分类数量。
- HX711 版本读取剩余药量重量，低于 `BOARD_LOW_MEDICINE_WEIGHT_G` 时触发低药量提醒。
- TTS 版本通过 USART1 发送 GB2312 文本，播报“吃药时间到了，请取药”和“药物已不足，请准备药物”。
- DHT11 版本采集药盒温湿度并随状态显示/上报。

## 按键逻辑

| 按键 | 普通模式 | 定时设置 | 药量设置 | 手动模式 |
|---|---|---|---|---|
| K1 | 切换模式 | 切换到药量设置 | 切换到手动 | 返回普通 |
| K2 | - | 切换字段/定时组 | 切换药品分类 | 打开/关闭药盒 |
| K3 | - | 当前字段加/启用切换 | 药量加 | 报警开关 |
| K4 | 停止报警 | 当前字段减/启用切换 | 药量减 | 停止报警 |

## 引脚定义

| 外设 | 引脚 | 说明 |
|---|---|---|
| OLED SCL/SDA | PB6 / PB7 | I2C1，地址 `0x78` |
| DS1302 CE/DAT/SCLK | PB15 / PB9 / PB8 | RTC |
| 红外取药检测 | PA7 | 低电平有效 |
| LED | PA6 | 高电平触发 |
| 蜂鸣器 | PB12 | 低电平触发 |
| SG90 舵机 | PA0 / TIM2_CH1 | 版本 5+ |
| HX711 SCK/DT | PA11 / PA12 | 版本 10+ |
| DHT11 | PA5 | 版本 20+ |
| TTS TX/RX | PA9 / PA10 | USART1，版本 15+ |
| JDY-31 TX/RX | PA2 / PA3 | USART2，版本 2/6/11/16/21 |
| ESP8266 TX/RX | PB10 / PB11 | USART3，WiFi/华为云版本 |
| ESP8266 CH_PD/RST | PB0 / PB1 | WiFi/华为云版本 |
| K1/K2/K3/K4 | PB2/PB3/PB4/PB5 | 低电平按下，关闭 JTAG |

## 通信协议

普通 WiFi/BLE 版本收发紧凑 JSON。常用命令：

```json
{"cmd":"get_status"}
{"cmd":"open_box"}
{"cmd":"close_box"}
{"cmd":"ack_take"}
{"cmd":"set_time","year":2026,"month":7,"day":8,"hour":8,"minute":30,"second":0}
{"cmd":"set_timer","index":1,"enabled":1,"hour":8,"minute":0,"dose":1,"category":0}
{"cmd":"set_count","index":1,"value":10}
```

状态/物模型字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `version_no` | int | 固件版本号 |
| `alarm` | bool/int | 吃药提醒状态 |
| `box_open` | bool/int | 药盒舵机打开状态 |
| `taken` | bool/int | 最近一次取药检测 |
| `timer_index` | int | 当前选中定时，1-3 |
| `timer_enabled` | bool/int | 当前定时是否启用 |
| `timer_hour` | int | 当前定时小时 |
| `timer_minute` | int | 当前定时分钟 |
| `timer_dose` | int | 当前剂量 |
| `medicine1` | int | 第 1 类药品数量 |
| `medicine2` | int | 第 2 类药品数量 |
| `medicine3` | int | 第 3 类药品数量 |
| `weight_g` | float | 剩余药量重量，版本 10+ |
| `low_medicine` | bool/int | 低药量提醒，版本 10+ |
| `temperature` | float | 药盒温度，版本 20+ |
| `humidity` | float | 药盒湿度，版本 20+ |

## 华为云配置

- Broker: `7e87c47089.st1.iotda-device.cn-east-3.myhuaweicloud.com`
- Port: `1883`
- Service ID: `YYYH`
- STM32 Device ID: `6a476514e094d615924ed7ef_YYYH_stm32`
- STM32 Device Secret: `YYYH_stm32`
- Property report topic: `$oc/devices/6a476514e094d615924ed7ef_YYYH_stm32/sys/properties/report`
- Property set topic: `$oc/devices/6a476514e094d615924ed7ef_YYYH_stm32/sys/properties/set/#`
- Custom uplink: `/jiabailie/M2M/YYYH_stm32/up`
- Custom downlink: `/jiabailie/M2M/YYYH_stm32/down`

## 上位机 Flutter

- 工程路径：`projects/YYYH_001/YYYH_001_upper_ui_flutter`
- 公共层：`common/flutter`
- 已接入：BLE、普通 WiFi/MQTT、华为云 IoTDA、ESP32-CAM MJPEG 视频视图、版本视图和控制页。
- 常用命令：`get_status`、`open_box`、`close_box`、`ack_take`、`set_time`、`set_timer`、`set_count`。

## 构建验证

已使用 Ninja + `arm-none-eabi-gcc` 构建代表版本：

```powershell
cmake -G Ninja -S projects\YYYH_001 -B build\YYYH_001_v1_ninja -DMVP_APP_VERSION=1
cmake --build build\YYYH_001_v1_ninja

cmake -G Ninja -S projects\YYYH_001 -B build\YYYH_001_v24_ninja -DMVP_APP_VERSION=24
cmake --build build\YYYH_001_v24_ninja
```

已覆盖编译版本：1、5、10、15、20、21、23、24。
