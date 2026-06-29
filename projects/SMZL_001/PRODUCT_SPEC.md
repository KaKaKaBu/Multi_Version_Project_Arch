# SMZL_001 睡眠监测产品规格

## 1. 产品概述

SMZL_001 是基于 STM32F103C8T6 的老年人睡眠监测项目。固件使用 `APP_VERSION` 选择 1-3 版本功能，应用层通过统一接口采集姿态、心率血氧和温度，并在 OLED/上位机显示状态。

上位机主栈为 Flutter：`SMZL_001_upper_ui_flutter/` 覆盖 Web/Android 调试入口，并通过 `version_features.json` 描述蓝牙、WiFi 和监测能力。

## 2. 版本矩阵

| APP_VERSION | 功能 |
| --- | --- |
| 1 | MPU6050 姿态/翻身检测、MAX30102 心率血氧、DS18B20 体温、OLED、按键、蜂鸣器/LED 报警 |
| 2 | 版本 1 + JDY-31 蓝牙远程监测 |
| 3 | 版本 1 + ESP8266 WiFi/MQTT 远程监测 |

## 3. 功能宏

| 宏 | 版本 |
| --- | --- |
| `VERSION_FEATURE_BLE` | 2 |
| `VERSION_FEATURE_WIFI` | 3 |
| `VERSION_FEATURE_REMOTE` | 2-3 |
| `VERSION_FEATURE_DRIVER_JDY31_BLE` | 2 |
| `VERSION_FEATURE_DRIVER_ESP8266_WIFI` | 3 |
| `VERSION_FEATURE_DRIVER_ESP8266_MQTT` | 3 |

## 4. 驱动映射

| 功能 | 驱动注册名 | 接口 |
| --- | --- | --- |
| 姿态/翻身 | `mpu6050` | `imu_sensor_t` |
| 心率/血氧 | `max30102` | `pulse_oximeter_t` |
| 体温 | `ds18b20` | `temp_hum_sensor_t` |
| OLED | `oled` | `display_driver_t` |
| 蜂鸣器 | `buzzer` | `misc_driver_t` |
| LED | `led` | `misc_driver_t` |
| 按键 | `key` | `input_driver_t` |
| WiFi/MQTT | `esp8266` | `comm_driver_t` + `esp8266_mqtt` |
| 蓝牙 | `jdy31` | `comm_driver_t` |

## 5. 本机交互

- 监测模式：周期采集翻身、心率、血氧、温度，并根据阈值触发报警。
- 阈值模式：调整心率上/下限、血氧上/下限、温度上/下限。
- 按键可进入阈值设置、选择阈值项、增减阈值，并清除翻身计数。

## 6. 远程 JSON 协议

上行遥测：

```json
{
  "type": "telemetry",
  "version": "SMZL_001",
  "version_no": 3,
  "data": {
    "heart_rate": 78,
    "spo2": 97,
    "temperature": 36.5,
    "turnover_count": 4,
    "alarm": 0,
    "thresholds": {
      "hr_upper": 100,
      "hr_lower": 50,
      "spo2_upper": 100,
      "spo2_lower": 90,
      "temp_upper": 38,
      "temp_lower": 35
    }
  }
}
```

下行命令：

```json
{"cmd":"set_threshold","params":{"hr_upper":100,"hr_lower":50,"spo2_upper":100,"spo2_lower":90,"temp_upper":38,"temp_lower":35}}
{"cmd":"clear_turnover"}
{"cmd":"get_status"}
```

JSON 由 cJSON 构建/解析，避免手工拼接。

## 7. 引脚定义

引脚以 `app/board_config.h` 为准，实际 PCB 接线变更时只修改该文件。

| 功能 | 引脚 | 版本/说明 |
| --- | --- | --- |
| OLED I2C SCL/SDA | PB6 / PB7 | I2C1，地址 `0x78` |
| MPU6050 I2C SCL/SDA | PB6 / PB7 | 与 OLED 共用 I2C1，地址 `0xD0` |
| MAX30102 I2C SCL/SDA | PB6 / PB7 | 与 OLED 共用 I2C1，地址 `0xAE` |
| DS18B20 | PB12 | 单总线体温传感器 |
| 蜂鸣器 | PB8 | 报警输出 |
| LED | PC13 | 状态/报警输出 |
| Key1/Key2/Key3/Key4 | PB4 / PB5 / PB13 / PB14 | 模式、清零/选择、加、减 |
| ESP8266 TX/RX | PB10 / PB11 | v3，USART3 |
| ESP8266 CH_PD/RST | PB0 / PB1 | v3 |
| JDY-31 TX/RX | PA2 / PA3 | v2，USART2 |
| 调试输出 | PC13 | `HAL_DEBUG_UART_ENABLE` 时启用 |

## 8. 构建

```bash
cmake -S projects/SMZL_001 -B build/SMZL_001_v3 -G Ninja -DAPP_VERSION=3
cmake --build build/SMZL_001_v3
```

## 9. 导出

```bash
python tools/export_project.py --project SMZL_001 --batch --versions 1-3 -o ../exports --clean \
  --extras readme.txt,PRODUCT_SPEC.md
```
