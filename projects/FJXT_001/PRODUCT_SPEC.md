# FJXT_001 车窗防夹系统产品规格

## 1. 产品概述

FJXT_001 是基于 STM32F103C8T6 的汽车车门车窗防夹控制项目。固件通过 `APP_VERSION=1..10` 选择功能版本，应用层只写车窗业务，板级引脚集中在 `board/board_config.h`，驱动由 `driver_catalog.cmake` 按版本选择。

系统由 OLED、ULN2003 步进电机、到位开关、防夹检测、蜂鸣器、报警灯、按键，以及可选蓝牙、WiFi、ESP-CAM、云平台和 JR6001/SU03T 语音模块组成。

## 2. 版本矩阵

| APP_VERSION | 产品编号 | 功能 |
| --- | --- | --- |
| 1 | FJXT-001 / S248N | STM32、防夹检测、电机控制、开/关到位、一键开关、点动开关、OLED、声光提醒、按键 |
| 2 | FJXT-002 / S248B | 版本 1 + 蓝牙 APP |
| 3 | FJXT-003 / S248W | 版本 1 + WiFi APP |
| 4 | FJXT-004 / S248CAM | 版本 3 + ESP-CAM 地址/视频流链接上报 |
| 5 | FJXT-005 / S248云I | 版本 1 + 云平台 APP |
| 6 | FJXT-006 / S249N | 版本 1 + JR6001/SU03T 语音播报 |
| 7 | FJXT-007 / S249B | 版本 6 + 蓝牙 APP |
| 8 | FJXT-008 / S249W | 版本 6 + WiFi APP |
| 9 | FJXT-009 / S249CAM | 版本 8 + ESP-CAM 地址/视频流链接上报 |
| 10 | FJXT-010 / S249云I | 版本 6 + 云平台 APP |

FJXT-011 是 STC89C52/C51 版本，需求与 STM32 版本差异较大：只要求按键控制 ULN2003 正反转和到位/夹到停转。当前项目先完成 STM32 版本 1-10，C51 版本可在确认后作为 `TARGET_PLATFORM=mcs51_sdcc` 分支补齐。

## 3. 功能宏

| 宏 | 版本 | 说明 |
| --- | --- | --- |
| `VERSION_FEATURE_BLE` | 2, 7 | JDY-31 蓝牙透传控制 |
| `VERSION_FEATURE_WIFI` | 3, 4, 8, 9 | ESP8266 WiFi/MQTT |
| `VERSION_FEATURE_CAMERA` | 4, 9 | 解析 ESP-CAM 局域网地址和 stream 链接 |
| `VERSION_FEATURE_CLOUD` | 5, 10 | 云平台 MQTT 通道 |
| `VERSION_FEATURE_REMOTE` | 2-5, 7-10 | APP 远程查看与控制 |
| `VERSION_FEATURE_VOICE` | 6-10 | JR6001/SU03T 语音播报 |

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
| 语音播报 | `su03t` | `comm_driver_t` |

## 5. 默认引脚资源

| 功能 | 默认资源 |
| --- | --- |
| OLED SSD1306 | I2C1 PB6/PB7 |
| Key1 一键开 | PA4，低有效 |
| Key2 一键关 | PA5，低有效 |
| Key3 开一点停 | PA6，低有效 |
| Key4 关一点停 | PA7，低有效 |
| 打开到位开关 | PA0，低有效 |
| 关闭到位开关 | PA1，低有效 |
| 防夹/门道检测 | PA8，低有效 |
| ULN2003 IN1-IN4 | PB12/PB13/PB14/PB15 |
| Buzzer | PB8 |
| Alarm LED | PB9 |
| JDY-31 蓝牙 | USART2 PA2/PA3 |
| ESP8266 WiFi/云平台 | USART3 PB10/PB11，CH_PD=PA11，RST=PA12 |
| JR6001/SU03T 语音 | USART1 PA9/PA10 |

## 6. 控制逻辑

- 一键开：电机向打开方向运动，直到打开到位开关有效。
- 一键关：电机向关闭方向运动，直到关闭到位开关有效。
- 开一点停：从当前位置向打开方向运行 `BOARD_STEPMOTOR_NUDGE_DEGREE`。
- 关一点停：从当前位置向关闭方向运行 `BOARD_STEPMOTOR_NUDGE_DEGREE`。
- 防夹：关闭过程中检测到有物/手，进入 `Anti Pinch`，电机反向打开 `BOARD_STEPMOTOR_REVERSE_DEGREE` 或直到打开到位。
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

CAM 版本只预留 ESP-CAM 信息解析，不在 MCU 内处理视频流：

```json
{"cmd":"camera_info","params":{"ip":"192.168.1.88","stream":"http://192.168.1.88:81/stream"}}
```

遥测：

```json
{
  "type": "telemetry",
  "version": "FJXT_001",
  "version_no": 9,
  "data": {
    "state": "Anti Pinch",
    "open_limit": 0,
    "close_limit": 0,
    "pinch": 1,
    "alarm": 1,
    "camera": 1,
    "cloud": 0,
    "camera_ip": "192.168.1.88",
    "camera_stream": "http://192.168.1.88:81/stream"
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
