# ZHJG_001 智能井盖产品规格

## 1. 产品概述

ZHJG_001 是基于 STM32F103C8T6 的智能井盖监测产品族。固件通过 `APP_VERSION=1..3` 选择功能版本，驱动通过 `REGISTER_DRIVER(...)` 自注册并由 `driver_catalog.cmake` 按版本选源，应用层只通过 `devmgr_get_*()` 与标准接口访问设备。

产品用于井下沼气/甲烷风险、水位异常和井盖倾斜异常监测。v2 增加 GPS 与 A7670C 短信报警，v3 增加 ESP-01S WiFi/MQTT 与 Flutter 上位机实时监控、阈值设置和异常提醒。

## 2. 版本矩阵

| APP_VERSION | 产品编号 | 功能 |
| --- | --- | --- |
| 1 | ZHJG-001 | MQ-4 沼气检测、水位检测、MPU6050 倾斜检测、OLED、声光报警、4 键阈值设置 |
| 2 | ZHJG-002 | 版本 1 + L76K GPS 定位 + A7670C 短信报警，短信包含经纬度 |
| 3 | ZHJG-003 | 版本 1 + L76K GPS 定位 + ESP-01S WiFi/MQTT + Flutter APP 实时数据、阈值设置、异常提醒 |

## 3. 固件功能宏

| 宏 | 版本 | 说明 |
| --- | --- | --- |
| `VERSION_FEATURE_METHANE` | 1-3 | MQ-4 沼气/甲烷 ppm |
| `VERSION_FEATURE_WATER_LEVEL` | 1-3 | 模拟水位百分比 |
| `VERSION_FEATURE_TILT` | 1-3 | MPU6050 倾角 |
| `VERSION_FEATURE_OLED` | 1-3 | OLED 本地显示 |
| `VERSION_FEATURE_BUZZER` | 1-3 | 蜂鸣器报警 |
| `VERSION_FEATURE_LED` | 1-3 | 报警灯/LED |
| `VERSION_FEATURE_KEYS` | 1-3 | 4 键本地设置 |
| `VERSION_FEATURE_GPS` | 2-3 | L76K GNSS |
| `VERSION_FEATURE_SMS` | 2 | A7670C 短信报警 |
| `VERSION_FEATURE_WIFI` | 3 | ESP-01S WiFi |
| `VERSION_FEATURE_REMOTE` | 3 | MQTT 远程控制/遥测 |

项目不使用 `ZHJG_VERSION` / `ZHJG_HAS_*` 作为主版本规则；版本输入统一为 `APP_VERSION`。

## 4. 驱动映射

| 功能 | 驱动注册名 | 接口 | 文件 |
| --- | --- | --- | --- |
| MQ-4 沼气 | `mq4_methane` | `gas_sensor_t` | `drivers/sensors/mq4_methane.c` |
| 水位 | `water_level` | `analog_probe_t` | `drivers/sensors/water_level.c` |
| 倾斜 | `mpu6050` | `imu_sensor_t` | `drivers/sensors/mpu6050.c` |
| OLED | `oled` | `display_driver_t` | `drivers/displays/oled_ssd1306.c` |
| 按键 | `key` | `input_driver_t` + `key_service` | `drivers/misc/key.c` |
| 蜂鸣器 | `buzzer` | `misc_driver_t` | `drivers/misc/buzzer.c` |
| 报警灯 | `led` | `misc_driver_t` | `drivers/misc/led.c` |
| GPS | `l76k` | `gnss_driver_t` | `drivers/comm/l76k_gnss.c` |
| 短信 | `a7670c` | `comm_driver_t` + `a7670c_sms_*` | `drivers/comm/a7670c_sms.c` |
| WiFi/MQTT | `esp8266` | `comm_driver_t` + `esp8266_mqtt_*` | `drivers/comm/esp8266_wifi.c`, `drivers/comm/esp8266_mqtt.c` |

## 5. 默认引脚资源

| 功能 | 默认资源                               |
| --- |------------------------------------|
| OLED SSD1306 | I2C1 PB6/PB7                       |
| MPU6050 | I2C1 PB6/PB7                       |
| MQ-4 | PA0 / ADC_Channel_0                |
| 水位 | PA1 / ADC_Channel_1                |
| Key1-Key4 | PB4 / PB5 / PB12 / PB13，内部上拉       |
| Buzzer | PB8                                |
| Alarm LED/light | PA6                                |
| GPS L76K | USART2 PA2/PA3                     |
| A7670C | USART1 PA9/PA10                    |
| ESP-01S/ESP8266 | USART3 PB10/PB11，CH_PD=PB0，RST=PB1 |

实际 PCB 接线如有变化，仅修改 `app/board_config.h`，驱动中不写死引脚。

## 6. 按键交互

- Key1：自动监测 / 阈值设置模式切换。
- Key2：阈值设置模式下切换阈值项：沼气、水位、倾角。
- Key3：增加当前选中阈值。
- Key4：减少当前选中阈值。

默认阈值：沼气 1000 ppm，水位 70%，倾角 15°。

## 7. 报警规则

任一条件满足即进入报警状态：

- `methane_ppm > methane_threshold_ppm`
- `water_level_percent > water_threshold_percent`
- `tilt_degree > tilt_threshold_degree`

自动模式下报警灯与蜂鸣器按 300 ms 周期闪烁；阈值设置模式下关闭报警输出但仍刷新传感器数据。

## 8. SMS 报警格式

v2 报警时调用 A7670C 文本短信流程：`AT`、`ATE0`、`AT+CPIN?`、`AT+CMGF=1`、`AT+CSCS="GSM"`、`AT+CMGS`、正文、`Ctrl+Z`。

短信正文使用 ASCII 字段，避免在第一版驱动中引入复杂编码转换：

```text
ZHJG_001 ALARM CH4:1200ppm Water:82% Tilt:18 GPS:1 30.123456 120.123456
```

短信触发有冷却时间，持续报警不会高频发送。

## 9. MQTT JSON 协议

v3 MQTT 配置：

- Client ID：`ZHJG_001`
- 命令 Topic：`ZHJG_001`
- 遥测 Topic：`ZHJG_001/web`

遥测 JSON 由 cJSON 构建，字段包含 `type`、`version`、`version_no`、`data.methane_ppm`、`data.water_level_percent`、`data.tilt_degree`、`data.alarm`、`data.alarms`、`data.thresholds` 与 `data.gps`。

下行命令：

```json
{"cmd":"get_status"}
{"cmd":"set_threshold","params":{"methane_ppm":900,"water_level_percent":75,"tilt_degree":12}}
{"cmd":"set_mode","params":{"mode":"auto"}}
{"cmd":"set_mode","params":{"mode":"threshold"}}
```

业务 JSON 禁止用 `sprintf`/`strcat`/`strcpy` 拼接，必须通过 cJSON 构建/解析。

## 10. 构建

```bash
cmake -S projects/ZHJG_001 -B build/ZHJG_001_v1 -G Ninja -DAPP_VERSION=1 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/ZHJG_001_v1

cmake -S projects/ZHJG_001 -B build/ZHJG_001_v2 -G Ninja -DAPP_VERSION=2 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/ZHJG_001_v2

cmake -S projects/ZHJG_001 -B build/ZHJG_001_v3 -G Ninja -DAPP_VERSION=3 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/ZHJG_001_v3
```

## 11. 导出

```bash
python tools/export_project.py --project ZHJG_001 --batch --versions 1-3 -o ../exports --clean --extras readme.txt,PRODUCT_SPEC.md
```
