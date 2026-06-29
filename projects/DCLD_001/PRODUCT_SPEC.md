# DCLD_001 倒车雷达产品规格

## 1. 产品概述

DCLD_001 是基于 STM32F103C8T6 的超声波测距倒车雷达产品族。固件通过 `APP_VERSION` 选择 1-7 版本功能，公共驱动保持自注册，应用层只通过 `devmgr_get_*()` 与标准接口访问设备。

上位机主栈为 Flutter：`DCLD_001_upper_ui_flutter/` 覆盖 v1-v7 的 App/Web 主界面、MQTT/BLE/mock 通信与版本化导出；历史 `DCLD_001_upper_ui/` uni-app 工程仅作为迁移对照和回退参考保留。

## 2. 版本矩阵

| APP_VERSION | 产品编号 | 功能 |
| --- | --- | --- |
| 1 | DCLD-001 | HC-SR04 超声波测距、OLED 当前距离、声光报警、报警频率随距离变化、按键阈值设置 |
| 2 | DCLD-002 | 版本 1 + DS18B20 温度补偿 |
| 3 | DCLD-003 | 版本 2 + SU03T 语音播报 |
| 4 | DCLD-004 | 版本 2 + ESP-01S WiFi MQTT + Android APP 显示/阈值设置/异常提醒 |
| 5 | DCLD-005 | 版本 2 + JDY-31 蓝牙 APP 显示/阈值设置/异常提醒 |
| 6 | DCLD-006 | 版本 3 + ESP-01S WiFi MQTT + Android APP |
| 7 | DCLD-007 | 版本 6 + ESP32-CAM 视频流在 APP 端展示 |

## 3. 固件功能宏

| 宏 | 版本 |
| --- | --- |
| `VERSION_FEATURE_TEMP_COMP` | 2-7 |
| `VERSION_FEATURE_VOICE` | 3, 6, 7 |
| `VERSION_FEATURE_WIFI` | 4, 6, 7 |
| `VERSION_FEATURE_BLE` | 5 |
| `VERSION_FEATURE_REMOTE` | 4-7 |
| `VERSION_FEATURE_CAMERA` | 7 |

## 4. 驱动映射

| 功能 | 驱动注册名 | 接口 |
| --- | --- | --- |
| 超声波测距 | `hcsr04` | `distance_sensor_t` |
| 温度补偿 | `ds18b20` | `temp_hum_sensor_t` |
| OLED | `oled` | `display_driver_t` |
| LED | `led` | `misc_driver_t` |
| 蜂鸣器 | `buzzer` | `misc_driver_t` |
| 按键 | `key` | `input_driver_t` + `key_service` |
| WiFi | `esp8266` | `comm_driver_t` + `esp8266_mqtt` |
| 蓝牙 | `jdy31` | `comm_driver_t` |
| 语音 | `su03t` | `comm_driver_t` |

## 5. 按键交互

- Key1：自动模式 / 阈值设置界面切换。
- Key2：阈值设置界面下阈值 +5 cm。
- Key3：阈值设置界面下阈值 -5 cm。
- 自动模式下，距离低于阈值触发声光报警；距离越近，LED/蜂鸣器闪烁周期越短。

## 6. 温度补偿

HC-SR04 驱动输出按 20℃ 近似声速计算的距离。DCLD 应用层在 v2+ 中读取 DS18B20 温度并使用：

`compensated = raw * (331.3 + 0.606 * temperature_c) / 343.42`

DS18B20 采样周期为 5 秒，距离采样周期为 250 ms，避免温度转换阻塞每次测距。

## 7. 远程 JSON 协议

遥测 JSON 字段：

```json
{
  "version": "DCLD_001",
  "version_no": 7,
  "mode": "auto",
  "distance_cm": 55,
  "raw_distance_cm": 54,
  "threshold_cm": 80,
  "temperature_c": 25.5,
  "alarm": 1,
  "camera_url": "http://192.168.4.1:81/stream",
  "thresholds": { "distance": 80 }
}
```

下行命令：

```json
{"cmd":"set_threshold","value":90}
{"cmd":"set_mode","mode":"auto"}
{"cmd":"set_mode","mode":"threshold"}
{"cmd":"get_status"}
```

业务 JSON 由 cJSON 构建/解析，禁止手工拼接 JSON 字符串。

## 8. ESP32-CAM 说明

DCLD-007 不在 STM32 固件中实现摄像头驱动。ESP32-CAM 作为独立视频节点提供 HTTP/MJPEG stream，固件通过 `BOARD_ESP32_CAM_STREAM_URL` 在遥测 JSON 中发布 URL，APP 端负责展示视频流。

## 9. 引脚定义

引脚以 `app/board_config.h` 为准，实际 PCB 接线变更时只修改该文件。

| 功能 | 引脚 | 版本/说明 |
| --- | --- | --- |
| OLED I2C SCL/SDA | PB6 / PB7 | I2C1，地址 `0x78` |
| HC-SR04 TRIG/ECHO | PA0 / PA1 | 超声波测距 |
| LED | PA8 | 声光报警输出 |
| 蜂鸣器 | PB0 | 声光报警输出 |
| Key1/Key2/Key3 | PB12 / PB13 / PB14 | 模式、阈值加、阈值减 |
| DS18B20 | PA5 | v2-v7 温度补偿 |
| ESP8266 TX/RX | PB10 / PB11 | v4、v6、v7，USART3 |
| ESP8266 CH_PD/RST | PB1 / PA11 | v4、v6、v7 |
| JDY-31 TX/RX | PA2 / PA3 | v5，USART2 |
| SU03T TX/RX | PA9 / PA10 | v6-v7，WiFi 版本使用 USART1 |
| SU03T TX/RX | PA2 / PA3 | v3，非 WiFi 版本使用 USART2 |
| 调试输出 | PC13 | `HAL_DEBUG_UART_ENABLE` 时启用 |

## 10. 构建

```bash
cmake -S projects/DCLD_001 -B build/DCLD_001_v7 -G Ninja \
  -DAPP_VERSION=7 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/DCLD_001_v7
```

## 11. 导出

```bash
python tools/export_project.py --project DCLD_001 --batch --versions 1-7 -o ../exports --clean \
  --extras readme.txt,PRODUCT_SPEC.md
```
