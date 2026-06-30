# ZNLYJ_001 智能晾衣架产品规格

## 1. 产品概述

ZNLYJ_001 是基于 STM32F103C8T6 的智能晾衣架项目。固件使用 `APP_VERSION` 选择 1-4 版本功能，应用层统一通过 `devmgr_get_*()` 获取传感器、步进电机、显示和通讯驱动。

上位机主栈为 Flutter：`ZNLYJ_001_upper_ui_flutter/` 覆盖 Web/Android 调试入口，并通过 `version_features.json` 描述光照、重量、蓝牙和 WiFi 能力。

## 2. 版本矩阵

| APP_VERSION | 功能 |
| --- | --- |
| 1 | DHT11 温湿度、E18 人体检测、红外衣物检测、步进电机开合、OLED、按键、蜂鸣器报警 |
| 2 | 版本 1 + GL5506 光照检测 + HX711 重量检测 |
| 3 | 版本 2 + JDY-31 蓝牙远程控制 |
| 4 | 版本 2 + ESP8266 WiFi/MQTT 远程控制 |

## 3. 功能宏

| 宏 | 版本 |
| --- | --- |
| `VERSION_FEATURE_LIGHT` | 2-4 |
| `VERSION_FEATURE_WEIGHT` | 2-4 |
| `VERSION_FEATURE_BLE` | 3 |
| `VERSION_FEATURE_WIFI` | 4 |
| `VERSION_FEATURE_REMOTE` | 3-4 |
| `VERSION_FEATURE_DRIVER_GL5506_LIGHT` | 2-4 |
| `VERSION_FEATURE_DRIVER_HX711` | 2-4 |
| `VERSION_FEATURE_DRIVER_JDY31_BLE` | 3 |
| `VERSION_FEATURE_DRIVER_ESP8266_WIFI` | 4 |
| `VERSION_FEATURE_DRIVER_ESP8266_MQTT` | 4 |

## 4. 驱动映射

| 功能 | 驱动注册名 | 接口 |
| --- | --- | --- |
| 温湿度 | `dht11` | `temp_hum_sensor_t` |
| 人体检测 | `e18` | `analog_probe_t` |
| 衣物检测 | `ir_clothes` | `analog_probe_t` |
| 光照 | `gl5506` | `analog_probe_t` |
| 重量 | `hx711` | `weight_sensor_t` |
| 晾衣架电机 | `stepmotor` | `stepper_driver_t` |
| OLED | `oled` | `display_driver_t` |
| 蜂鸣器 | `buzzer` | `misc_driver_t` |
| 按键 | `key` | `input_driver_t` |
| WiFi/MQTT | `esp8266` | `comm_driver_t` + `esp8266_mqtt` |
| 蓝牙 | `jdy31` | `comm_driver_t` |

## 5. 控制逻辑

- 自动模式：无人、温度高于阈值、湿度低于阈值时允许打开晾衣架；v2+ 还要求光照达到阈值。
- 手动模式：按键或远程命令直接开合晾衣架。
- 阈值模式：调整温度、湿度阈值；v2+ 增加光照和重量阈值。
- 报警：湿度过高且有人/有衣物，或 v2+ 重量超限时触发蜂鸣器。

## 6. 远程 JSON 协议

上行遥测：

```json
{
  "type": "telemetry",
  "version": "ZNLYJ_001",
  "version_no": 4,
  "data": {
    "temperature": 31,
    "humidity": 48,
    "rack_open": 1,
    "human_present": 0,
    "clothes_present": 1,
    "alarm": 0,
    "light": 70,
    "weight_kg": 4.8,
    "weight_overload": 0,
    "thresholds": {
      "temp": 30,
      "humidity": 60,
      "light": 50,
      "weight": 5
    }
  }
}
```

下行命令：

```json
{"cmd":"set_threshold","params":{"temp":30,"humidity":60,"light":50,"weight":5}}
{"cmd":"set_rack","params":{"open":1}}
{"cmd":"set_mode","params":{"mode":"auto"}}
{"cmd":"set_mode","params":{"mode":"manual"}}
{"cmd":"get_status"}
```

JSON 由 cJSON 构建/解析，避免手工拼接。

## 7. 引脚定义

引脚以 `board/board_config.h` 为准，实际 PCB 接线变更时只修改该文件。

| 功能 | 引脚 | 版本/说明 |
| --- | --- | --- |
| OLED I2C SCL/SDA | PB6 / PB7 | I2C1，地址 `0x78` |
| DHT11 | PA0 | 温湿度单总线 |
| GL5506 | PA1 | v2-v4，ADC1 Channel 1 |
| E18 人体检测 | PB2 | 低电平有效 |
| 红外衣物检测 | PB3 | GPIO 输入 |
| 步进电机 A/B/C/D | PA4 / PA5 / PA6 / PA7 | 晾衣架开合 |
| 蜂鸣器 | PB8 | 报警输出 |
| HX711 SCK/DT | PA11 / PA12 | v2-v4，重量检测 |
| Key1/Key2/Key3/Key4 | PB4 / PB5 / PB12 / PB13 | 模式、开合/选择、加、减 |
| ESP8266 TX/RX | PB10 / PB11 | v4，USART3 |
| ESP8266 CH_PD/RST | PB0 / PB1 | v4 |
| JDY-31 TX/RX | PA2 / PA3 | v3，USART2 |
| 调试输出 | PC13 | `HAL_DEBUG_UART_ENABLE` 时启用 |

## 8. 构建

```bash
cmake -S projects/ZNLYJ_001 -B build/ZNLYJ_001_v4 -G Ninja -DAPP_VERSION=4
cmake --build build/ZNLYJ_001_v4
```

## 9. 导出

```bash
python tools/export_project.py --project ZNLYJ_001 --batch --versions 1-4 -o ../exports --clean \
  --extras readme.txt,PRODUCT_SPEC.md
```
