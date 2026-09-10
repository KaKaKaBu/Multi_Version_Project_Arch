# FJXT_001 车窗防夹系统产品规格

## 1. 产品概述

FJXT_001 是基于 STM32F103C8T6 的汽车车门车窗防夹控制项目。固件通过 `APP_VERSION=1..10` 选择功能版本，应用层只写车窗业务，板级引脚集中在 `board/board_config.h`，驱动由 `driver_catalog.cmake` 按版本选择。

系统由 OLED、ULN2003 步进电机、红外对管防夹检测、蜂鸣器、报警灯、按键，以及可选蓝牙、WiFi、ESP-CAM、云平台和串口透传 TTS 语音模块组成。

## 2. 版本矩阵

| APP_VERSION | 产品编号 | 功能 |
| --- | --- | --- |
| 1 | FJXT-001 / S248N | STM32、红外对管防夹检测、电机控制、固定行程一键开关、点动开关、OLED、声光提醒、按键 |
| 2 | FJXT-002 / S248B | 版本 1 + 蓝牙 APP |
| 3 | FJXT-003 / S248W | 版本 1 + WiFi APP |
| 4 | FJXT-004 / S248CAM | 版本 3 + ESP32-CAM 独立上报地址/视频流链接，APP 显示视频 |
| 5 | FJXT-005 / S248云I | 版本 1 + 华为云 IoTDA APP |
| 6 | FJXT-006 / S249N | 版本 1 + 串口透传 TTS 语音播报 |
| 7 | FJXT-007 / S249B | 版本 6 + 蓝牙 APP |
| 8 | FJXT-008 / S249W | 版本 6 + WiFi APP |
| 9 | FJXT-009 / S249CAM | 版本 8 + ESP32-CAM 独立上报地址/视频流链接，APP 显示视频 |
| 10 | FJXT-010 / S249云I | 版本 6 + 华为云 IoTDA APP |

FJXT-011 是 STC89C52/C51 版本，需求与 STM32 版本差异较大：只要求按键控制 ULN2003 正反转和到位/夹到停转。当前项目先完成 STM32 版本 1-10，C51 版本可在确认后作为 `TARGET_PLATFORM=mcs51_sdcc` 分支补齐。

## 3. 功能宏

| 宏 | 版本 | 说明 |
| --- | --- | --- |
| `VERSION_FEATURE_BLE` | 2, 7 | JDY-31 蓝牙透传控制 |
| `VERSION_FEATURE_WIFI` | 3, 4, 8, 9 | ESP8266 WiFi/MQTT |
| `VERSION_FEATURE_CAMERA` | 4, 9 | APP 显示 ESP32-CAM 视频面板；视频链接由 ESP32-CAM 独立上报 |
| `VERSION_FEATURE_CLOUD` | 5, 10 | ESP8266 华为云 IoTDA MQTT 通道 |
| `VERSION_FEATURE_REMOTE` | 2-5, 7-10 | APP 远程查看与控制 |
| `VERSION_FEATURE_VOICE` | 6-10 | 串口透传 TTS 语音播报 |

## 4. 驱动映射

| 功能 | 驱动注册名 | 接口 |
| --- | --- | --- |
| OLED | `oled` | `display_driver_t` |
| 按键 | `key` | `input_driver_t` + `key_service` |
| ULN2003 步进电机 | `stepmotor` | `stepper_driver_t` |
| 蜂鸣器 | `buzzer` | `misc_driver_t` |
| 报警灯 | `led` | `misc_driver_t` |
| 蓝牙 | `jdy31` | `comm_driver_t` |
| WiFi/云平台 | `esp8266` | `comm_driver_t` + `esp8266_mqtt` |
| 语音播报 | `tts_uart` | 串口透传 TTS，`comm_driver_t` |

## 5. 默认引脚资源

| 功能 | 默认资源                               |
| --- |------------------------------------|
| OLED SSD1306 | I2C1 PB6/PB7                       |
| Key1 一键开 | PB2，低有效                            |
| Key2 一键关 | PB3，低有效，需关闭 JTAG                   |
| Key3 开一点停 | PB4，低有效，需关闭 JTAG                   |
| Key4 关一点停 | PB5，低有效                            |
| 红外对管防夹检测 | PA7，低有效，上拉输入                      |
| ULN2003 IN1-IN4 | PB12/PB13/PB14/PB15                |
| Buzzer | PB8                                |
| Alarm LED | PA6                                |
| JDY-31 蓝牙 | USART2 PA2/PA3                     |
| ESP8266 WiFi/云平台 | USART3 PB10/PB11，CH_PD=PB1，RST=PB0 |
| 串口透传 TTS 语音 | USART1 PA9/PA10，9600 8N1，GB2312 文本 |

## 6. 控制逻辑

- 一键开：电机向打开方向运行 `BOARD_STEPMOTOR_FULL_TRAVEL_DEGREE` 后停止并提示完成。
- 一键关：电机向关闭方向运行 `BOARD_STEPMOTOR_FULL_TRAVEL_DEGREE` 后停止并提示完成。
- 开一点停：从当前位置向打开方向运行 `BOARD_STEPMOTOR_NUDGE_DEGREE`。
- 关一点停：从当前位置向关闭方向运行 `BOARD_STEPMOTOR_NUDGE_DEGREE`。
- 防夹：关闭过程中红外对管检测到有物/手，进入 `Anti Pinch`，电机反向打开 `BOARD_STEPMOTOR_REVERSE_DEGREE` 后停止。
- 声光提醒：打开完成、关闭完成、防夹触发时蜂鸣器和报警灯闪烁。
- 语音提醒：版本 6-10 在上述提醒时同步发送语音命令。

## 7. 远程 JSON 协议

下行控制：

```json
{"cmd":"get_status"}
{"cmd":"open"}
{"cmd":"close"}
{"cmd":"open_step"}
{"cmd":"close_step"}
{"cmd":"stop"}
```

ESP32-CAM 独立向普通 MQTT Topic `FJXT_001` 上报视频链接，STM32 不上报、不转发视频字段：

```json
{"device_id":"esp32-ov3660","cam_ip":"10.132.56.102","mjpeg_url":"http://10.132.56.102:8080/stream","mjpeg_port":8080}
```

遥测：

```json
{
  "type": "telemetry",
  "version": "FJXT_001",
  "version_no": 9,
  "data": {
    "state": "Anti Pinch",
    "pinch": 1,
    "alarm": 1,
    "cloud": 0
  }
}
```

## 8. 构建

```bash
cmake -S projects/FJXT_001 -B build/FJXT_001_v1 -G Ninja -DAPP_VERSION=1 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/FJXT_001_v1 -j1

cmake -S projects/FJXT_001 -B build/FJXT_001_v9 -G Ninja -DAPP_VERSION=9 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/FJXT_001_v9 -j1
```
