# WXCS_001 尾气检测产品规格

## 1. 产品概述

WXCS_001 是车辆尾气/气体检测项目。STM32F103C8T6 平台固件使用 `APP_VERSION` 选择 1-3 版本功能，驱动通过 `DRIVER_CATALOG_WXCS_001` 引入，应用层通过标准接口读取传感器并控制风扇、蜂鸣器和 LED。C51 平台仅作为本地轮询样板，固定基础版功能，不承诺 scheduler、MQTT、BLE 或远程协议。

上位机主栈为 Flutter：`WXCS_001_upper_ui_flutter/` 覆盖 Web/Android 调试入口，并通过 `version_features.json` 描述各固件版本的远程能力。

## 2. 版本矩阵

| APP_VERSION | 功能 |
| --- | --- |
| 1 | MQ135 空气质量、MQ-7 CO、DS18B20 温度、OLED 显示、按键阈值设置、继电器风扇、蜂鸣器/LED 报警 |
| 2 | 版本 1 + ESP8266 WiFi/MQTT 远程遥测、模式/阈值/控制命令 |
| 3 | 版本 1 + JDY-31 蓝牙远程遥测、模式/阈值/控制命令 |

## 3. 功能宏

| 宏 | 版本 |
| --- | --- |
| `VERSION_FEATURE_WIFI` | 2 |
| `VERSION_FEATURE_BLE` | 3 |
| `VERSION_FEATURE_REMOTE` | 2-3 |
| `VERSION_FEATURE_DRIVER_ESP8266_WIFI` | 2 |
| `VERSION_FEATURE_DRIVER_ESP8266_MQTT` | 2 |
| `VERSION_FEATURE_DRIVER_JDY31_BLE` | 3 |

## 4. 驱动映射

| 功能 | 驱动注册名 | 接口 |
| --- | --- | --- |
| 空气质量 | `mq135` | `gas_sensor_t` |
| CO | `mq7_co` | `gas_sensor_t` |
| 温度 | `ds18b20` | `temp_hum_sensor_t` |
| OLED | `oled` | `display_driver_t` |
| 风扇继电器 | `relay` | `relay_driver_t` |
| 蜂鸣器 | `buzzer` | `misc_driver_t` |
| LED | `led` | `misc_driver_t` |
| 按键 | `key` | `input_driver_t` |
| WiFi/MQTT | `esp8266` | `comm_driver_t` + `esp8266_mqtt` |
| 蓝牙 | `jdy31` | `comm_driver_t` |

## 5. 本机交互

- 自动模式：温度、空气质量或 CO 超过阈值时打开风扇并触发声光报警。
- 手动模式：按键选择风扇/报警输出并切换状态。
- 阈值模式：按键选择温度、空气质量、CO 阈值并调整数值。

## 6. 远程 JSON 协议

上行遥测：

```json
{
  "type": "telemetry",
  "version": "WXCS_001",
  "version_no": 3,
  "data": {
    "temperature": 32,
    "air_quality_ppm": 460,
    "co_ppm": 80,
    "alarm": 0,
    "fan": 0,
    "mode": "AUTO",
    "thresholds": {
      "temp": 35,
      "air_quality_ppm": 500,
      "co_ppm": 100
    }
  }
}
```

下行命令：

```json
{"cmd":"set_threshold","params":{"temp":35,"air_quality_ppm":500,"co_ppm":100}}
{"cmd":"set_mode","params":{"mode":"auto"}}
{"cmd":"set_mode","params":{"mode":"manual"}}
{"cmd":"set_control","params":{"fan":1,"alarm":0}}
{"cmd":"get_status"}
```

JSON 由 cJSON 构建/解析，避免手工拼接。

## 7. 引脚定义

STM32F103 引脚以 `board/board_config.h` 为准，实际 PCB 接线变更时只修改该文件。

| 功能 | 引脚 | 版本/说明 |
| --- | --- | --- |
| OLED I2C SCL/SDA | PB6 / PB7 | I2C1，地址 `0x78` |
| MQ135 | PA0 | ADC1 Channel 0 |
| MQ-7 | PA1 | ADC1 Channel 1 |
| DS18B20 | PB12 | 单总线温度传感器 |
| 风扇继电器 | PA4 | 排风控制 |
| 蜂鸣器 | PB8 | 报警输出 |
| LED | PC13 | 状态/报警输出 |
| Key1/Key2/Key3/Key4 | PB4 / PB5 / PB13 / PB14 | 模式、选择、加、减 |
| ESP8266 TX/RX | PB10 / PB11 | v2，USART3 |
| ESP8266 CH_PD/RST | PB0 / PB1 | v2 |
| JDY-31 TX/RX | PA2 / PA3 | v3，USART2 |
| 调试输出 | PC13 | `HAL_DEBUG_UART_ENABLE` 时启用 |

C51/SDCC 轮询样板引脚以 `board/board_config_mcs51.h` 为准，只保留本机采集、按键、OLED 和输出控制。

| 功能 | 引脚 | 版本/说明 |
| --- | --- | --- |
| OLED I2C SCL/SDA | P1.0 / P1.1 | 软件 I2C，地址 `0x78` |
| ADC0832 CS/CLK/DIO | P1.2 / P1.3 / P1.4 | MQ135/MQ-7 共用外部 ADC |
| MQ135 | ADC0832 CH0 | 空气质量 |
| MQ-7 | ADC0832 CH1 | CO |
| DS18B20 | P1.5 | 单总线温度传感器 |
| Key1/Key2/Key3/Key4 | P2.0 / P2.1 / P2.2 / P2.3 | 模式、选择、加、减，本地轮询 |
| 风扇继电器 | P2.4 | 排风控制 |
| 蜂鸣器 | P2.5 | 报警输出 |
| LED | P2.6 | 状态/报警输出 |

## 8. 构建

```bash
cmake -S projects/WXCS_001 -B build/WXCS_001_v3 -G Ninja -DAPP_VERSION=3
cmake --build build/WXCS_001_v3

cmake -S projects/WXCS_001 -B build/WXCS_001_c51 -G Ninja -DTARGET_PLATFORM=mcs51_sdcc
cmake --build build/WXCS_001_c51
```

## 9. 导出

```bash
python tools/export_project.py --project WXCS_001 --batch --versions 1-3 -o ../exports --clean \
  --extras readme.txt,PRODUCT_SPEC.md
```
