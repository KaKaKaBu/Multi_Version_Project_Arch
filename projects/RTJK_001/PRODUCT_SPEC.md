# RTJK-001 人体健康监测仪 — 产品功能规格

> MCU：STM32F103C8T6 | 显示：OLED SSD1306 | 默认版本：V10

---

## 1. 版本矩阵

| 版本 | 传感器 | 通讯 |
|------|--------|------|
| V1 | MAX30102 心率/血氧 + 蜂鸣器 + LED | — |
| V2 | V1 | JDY-31 蓝牙 |
| V3 | V1 | ESP-01S WiFi/MQTT |
| V4 | V1 + DS18B20 体温 | — |
| V5 | V4 | 蓝牙 |
| V6 | V4 | WiFi |
| V7 | V4 + SU-03T 语音播报 | — |
| V8 | MAX30102 心率 + MSP20 ADC 血压 + 声光 | — |
| V9 | V8 + DS18B20 | — |
| V10 | V9 | WiFi |

---

## 2. 引脚分配

| 功能 | 引脚 |
|------|------|
| OLED / MAX30102 I2C | PB6 SCL, PB7 SDA |
| 按键 K1-K4 | PB4, PB5, PB12, PB13 |
| 蜂鸣器 | PB8 |
| LED | PC13 |
| DS18B20 | PA1 |
| MSP20 ADC | PA0 |
| ESP8266 USART3 | PB10 TX, PB11 RX |
| ESP8266 控制 | PB0 CH_PD, PB1 RST |
| JDY31 USART2 | PA2 TX, PA3 RX |
| SU03T USART1 | PA9 TX, PA10 RX |

---

## 3. 按键

| 按键 | 功能 |
|------|------|
| K1 | 切换 **自动 / 阈值** 模式 |
| K2 | 阈值模式：切换当前编辑项 |
| K3 | 阈值模式：当前项 **+** |
| K4 | 阈值模式：当前项 **-** |

---

## 4. 报警规则（自动模式）

- 心率 `< HR_min` 或 `> HR_max` → 声光报警
- 血氧 `< SpO2_min`（V1-V7）→ 声光报警
- 体温超上下限（V4+）→ 声光报警
- 收缩压超上下限（V8+）→ 声光报警
- V7：首次触发时 SU-03T 播报对应异常语音

---

## 5. 版本切换

**CMake（推荐）**

```bash
cmake -B build -DRTJK_VERSION=10
cmake --build build
```

**源码**

修改 [`app/version_config.h`](app/version_config.h) 或 CMake 传入的 `RTJK_VERSION`。

V8+ 自动启用 `HAL_ADC_ENABLE`（MSP20 血压 ADC）。

---

## 6. JSON 协议（WiFi / 蓝牙）

**上行** `RTJK_001`

```json
{"hr":72,"spo2":98,"temp":36.5,"bp_sys":120,"bp_dia":80,"alarm":0,"mode":"auto"}
```

**下行** `RTJK_001/cmd`

```json
{"cmd":"set_threshold","hr_min":60,"hr_max":100,"spo2_min":95,"temp_min":35.5,"temp_max":37.5,"bp_min":90,"bp_max":140}
{"cmd":"get_status"}
```

蓝牙版本（V2/V5）使用相同 JSON，经 JDY31 串口透传。

---

## 7. MSP20 标定

MSP20 通过 ADC 采集电压，换算系数位于 `board_config.h`：

- `BOARD_MSP20_VOLT_TO_SYS_K` / `OFFSET`
- `BOARD_MSP20_VOLT_TO_DIA_K` / `OFFSET`

需根据实板标定后调整。
