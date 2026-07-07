# SGTZ_001 51 电子秤项目规格

SGTZ_001 是 STC89C52/C51 轮询项目，按 `APP_VERSION=1..7` 覆盖电子秤、压力检测风扇、身高体重 BMI 和语音播报版本。当前项目使用 SDCC `mcs51` 后端，默认芯片宏为 `MCS51_CHIP_STC89C52=1`，默认晶振 `MCS51_FOSC_HZ=11059200UL`。

默认构建版本为 `APP_VERSION=7`，对应“身高体重 BMI + OLED + 语音播报”产品。本地固件已实现 HX711 上电去皮、超声波测高、BMI 计算、OLED 显示、按键锁定/重测，以及 v1 风扇继电器和 v2/v7 蜂鸣器提示。v5/v6 通过 ESP8266 AT 固件连接 WiFi/MQTT；v6 的视频监控仍视为外部摄像头套餐能力，不由 STC89C52 固件承载视频流。

注意：当前模块化框架 + OLED + HX711/HCSR04 + ESP8266/MQTT 的 v5/v6 固件 ROM 约 32KB，默认 SDCC 构建按 64KB 8051 code space 链接。裸 STC89C52 片内 Flash 通常为 8KB，无法直接容纳 v5/v6；实际硬件需使用带更大 Flash 的 8051 兼容芯片，或后续做面向 8KB 的专用极限裁剪。

| APP_VERSION | 对应型号 | 已接入本地驱动 | 说明 |
| --- | --- | --- | --- |
| 1 | SGTZ-001 | HX711、拨动开关、按键、继电器、OLED | 手动/压力感应风扇；压力阈值默认 500g |
| 2 | SGTZ-002 | HX711、按键、蜂鸣器、OLED | 0-5000g 阈值报警；EEPROM 持久化待补 STC IAP 驱动 |
| 3 | SGTZ-003 | HX711、HCSR04、按键、OLED | 按 K1 计算并锁定 BMI，K2 重测 |
| 4 | SGTZ-004 | 同 v3 | 蓝牙 APP 为规格预留，当前 C51 catalog 不链接 BLE |
| 5 | SGTZ-005 | HX711、HCSR04、OLED、按键、ESP8266、MQTT | 发布体重/身高/BMI JSON；订阅消息支持 `lock`、`unlock`、`tare` |
| 6 | SGTZ-006 | 同 v5 | WiFi/MQTT 同 v5，视频监控为外部套餐能力 |
| 7 | SGTZ-007 | HX711、HCSR04、OLED、按键、蜂鸣器 | 实时 BMI，K1 锁定并触发提示，K2 重测，K3 暂停/恢复提示 |

## 51 引脚

| 功能 | 引脚 |
| --- | --- |
| OLED SCL/SDA | P1.0 / P1.1 |
| HX711 SCK/DT | P1.2 / P1.3 |
| HCSR04 TRIG/ECHO | P1.4 / P1.5 |
| K1/K2/K3/K4 | P2.0 / P2.1 / P2.2 / P2.3 |
| 风扇继电器 | P2.4 |
| 蜂鸣器 | P2.5 |
| 语音串口 TXD/RXD | P3.1 / P3.0，默认 9600 |
| ESP8266 TXD/RXD | P3.1 / P3.0，默认 9600，仅 v5/v6 |
| ESP8266 CH_PD/RST | P2.6 / P2.7 |
| 手动/自动拨动开关 | P3.2，低电平为手动 |

## 构建

```bash
cmake -S projects/SGTZ_001 -B build/SGTZ_001_c51 -G Ninja -DTARGET_PLATFORM=mcs51_sdcc
cmake --build build/SGTZ_001_c51
```

切换版本：

```bash
cmake -S projects/SGTZ_001 -B build/SGTZ_001_c51_v1 -G Ninja -DTARGET_PLATFORM=mcs51_sdcc -DMVP_APP_VERSION=1
```

如 SDCC 未加入系统 PATH，可显式传入：

```bash
cmake -S projects/SGTZ_001 -B build/SGTZ_001_c51_v5 -G Ninja -DTARGET_PLATFORM=mcs51_sdcc -DMVP_APP_VERSION=5 -DMCS51_TOOLCHAIN_BIN="C:/Program Files/SDCC/bin" -DMCS51_CHIP=STC89C52 -DMCS51_FOSC_HZ=11059200UL
```

## ESP8266/MQTT

- 51 UART0 接 ESP8266：P3.1 -> ESP RX，P3.0 <- ESP TX。
- 默认波特率为 9600，ESP8266 AT 固件需提前配置为 9600。
- 默认 WiFi/MQTT 参数在 `board/board_config_mcs51.h` 中修改。
- `BOARD_ESP8266_MQTT_BACKEND=0` 使用普通 MQTT；改为 `1` 后连接华为云 IoTDA 物模型，属性上报会封装为 `services/properties` 格式，并自动回复属性设置 `request_id`。
- 发布 Topic：`SGTZ_001/pub`；订阅 Topic：`SGTZ_001/sub`。
- 遥测 JSON 字段：`type/device/weight_g/height_cm/bmi_x10/state/locked`。
- 订阅 payload 包含 `lock`、`unlock`、`tare` 时分别执行锁定、重测、去皮。

## 校准

- 上电时称重传感器必须空载，应用层会读取一次 HX711 作为 tare。
- `BOARD_HX711_SCALE` 在 `board/board_config_mcs51.h` 中按实际砝码校准。
- HCSR04 身高按顶部安装高度 `BOARD_HEIGHT_SENSOR_MOUNT_CM=200` 计算：身高 = 安装高度 - 测得距离。
