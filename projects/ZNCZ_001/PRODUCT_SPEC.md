# ZNCZ-001 智能插座 — 产品功能规格

> 版本一：继电器 + 定时 + 手动/自动模式 + WiFi  
> MCU：STM32F103C8T6 | 通讯：ESP-01S | 时钟：DS1302 | 显示：OLED

---

## 1. 产品概述

基于 STM32 单片机的智能插座，通过 ESP-01S WiFi 模块与 Android Studio 制作的 APP 通信，实现远程查看/控制继电器、校时及定时开关。

---

## 2. 硬件清单

| 器件 | 型号/说明 | 驱动/接口 |
| --- | --- | --- |
| 主控 | STM32F103C8T6 | BSP + HAL |
| 显示 | OLED（I2C SSD1306） | `display_driver_t` / `"oled"` |
| 继电器 | 单路 | `relay_driver_t` / `"relay"` |
| 实时时钟 | DS1302 | `rtc_driver_t` / `"ds1302"` |
| WiFi | ESP-01S | `comm_driver_t` / `"esp8266"` + `esp8266_mqtt` |
| 按键 | 5 个（见 §3） | `input_driver_t`（需扩展 5 键） |

---

## 3. 按键定义

| 按键 | 手动模式 | 定时模式 | 全局 |
| --- | --- | --- | --- |
| 键一 | — | — | 切换 **手动 / 定时** 模式；**上电默认手动** |
| 键二 | 打开/关闭继电器 | 移动星标，选择要设置的时间项（当前时间 / 开时间 / 关时间） | — |
| 键三 | 切换 WiFi 名称界面 | 移动光标，选择时/分/秒位 | — |
| 键四 | 切换 WiFi 密码界面 | 当前位 **+1** | — |
| 键五 | 切换 WiFi 连接状态界面 | 当前位 **-1** | — |

> 模板当前 `key.c` 仅支持 3 键（key1–key3），需扩展 `board_key4_pin` / `board_key5_pin` 及扫描逻辑。

---

## 4. 显示界面

### 4.1 手动模式（默认）

- 行 1–2：继电器 **开/关** 状态
- 行 3–4：**当前时间**（来自 DS1302，格式 HH:MM:SS 或 HH:MM）

### 4.2 定时模式

- 星标指示当前编辑项：当前时间 / 继电器打开时间 / 继电器关闭时间
- 光标指示当前编辑的时/分/秒位
- 键四/键五 增减数值

### 4.3 WiFi 信息（手动模式下键三/四/五 切换）

- 界面 A：WiFi **名称（SSID）**
- 界面 B：WiFi **密码**（可部分掩码显示）
- 界面 C：**连接状态**（已连接 / 未连接 / 连接中）

---

## 5. 定时逻辑

| 项 | 说明 |
| --- | --- |
| 当前时间 | DS1302 维护；APP 可 **一键校准** 写回 RTC |
| 打开时间 | 到达后继电器 **吸合** |
| 关闭时间 | 到达后继电器 **断开** |
| 模式 | 定时模式下由软件比较 RTC 与开/关时间，自动驱动继电器 |

跨日、开时间晚于关时间等边界需在应用层定义（建议：同日区间；复杂场景可二期扩展）。

---

## 6. WiFi / APP 协议（目标）

APP 通过 ESP-01S 与 MCU 交换 JSON（建议沿用 `cJSON` + MQTT 或 TCP 透传，与现有 `esp8266_mqtt` 对齐）。

### 6.1 APP → 设备（下行）

| 命令 | 字段示例 | 行为 |
| --- | --- | --- |
| 校时 | `{"cmd":"set_time","hour":12,"minute":30,"second":0}` | 写 DS1302 |
| 设开时间 | `{"cmd":"set_on_time","hour":8,"minute":0,"second":0}` | 保存开时间 |
| 设关时间 | `{"cmd":"set_off_time","hour":22,"minute":0,"second":0}` | 保存关时间 |
| 直接控继电器 | `{"cmd":"relay","state":1}` | 手动模式下立即开/关 |
| 查询状态 | `{"cmd":"get_status"}` | 触发状态上报 |

### 6.2 设备 → APP（上行）

| 类型 | 字段示例 |
| --- | --- |
| 状态上报 | `{"relay":1,"mode":"manual","time":"12:30:45","wifi":"connected"}` |
| 定时参数 | `{"on_time":"08:00:00","off_time":"22:00:00"}` |

具体 topic / 端口以 `board_config.h` 中 `BOARD_ESP8266_MQTT_*` 为准，与 Android 端约定一致即可。

---

## 7. 应用任务划分（建议实现）

```
key_poll_task     → 消抖 → APP_EVENT_KEY → app_logic
rtc_tick_task     → 每秒读 DS1302 → APP_EVENT_TICK → 定时比较 + 刷新显示
comm_poll_task    → esp8266_mqtt_poll → APP_EVENT_COMM_RX
app_logic_task    → FSM：手动 / 定时 / WiFi 子界面 + JSON 命令处理
```

---

## 8. 实现状态（2026-05）

| 功能 | 状态 |
| --- | --- |
| OLED 显示继电器状态 + 当前时间 | ✅ |
| DS1302 RTC + APP 校时 | ✅ |
| 5 键：手动/定时模式切换 | ✅ |
| 手动模式：键二控继电器；键三四五 WiFi 界面 | ✅ |
| 定时模式：编辑开/关时间 + 自动控继电器 | ✅ |
| ESP8266 MQTT + APP JSON 协议 | ✅ |
| 四任务：key_poll / rtc_tick / comm_poll / app_logic | ✅ |

**构建指标**：Flash ~34.5 KB，RAM ~7.9 KB。

### APP JSON 协议

**下行（`ZNCZ_001/cmd`）**

```json
{"cmd":"set_time","hour":12,"minute":30,"second":0,"year":2026,"month":5,"day":25}
{"cmd":"set_on_time","hour":8,"minute":0,"second":0}
{"cmd":"set_off_time","hour":22,"minute":0,"second":0}
{"cmd":"relay","state":1}
{"cmd":"get_status"}
```

**上行（`ZNCZ_001/telemetry`）**

```json
{"relay":1,"mode":"manual","time":"12:30:45","on_time":"08:00:00","off_time":"22:00:00","wifi":"connected"}
```

---

## 9. 引脚配置（board_config.h）

| 功能 | 引脚 |
| --- | --- |
| 继电器 | PA11 |
| ESP8266 | USART3 PB10/PB11，CH_PD PB0，RST PB1 |
| DS1302 | CE PB8，DATA PB9，SCLK PB15 |
| OLED I2C | PB6/PB7 |
| 按键 1–5 | PB4，PB5，PB12，PB13，PB14 |

---

## 10. 后续可选增强

| 项 | 说明 |
| --- | --- |
| 开/关时间掉电保存 | 当前存 RAM |
| ESP8266 断线重连 | 启动时连接，无自动重试 |
| 实板联调 | 需验证引脚与 Android APP |

---

## 11. 构建与配置

见 [readme.txt](./readme.txt)。WiFi/MQTT 占位符在 `app/board_config.h` 的 `BOARD_ESP8266_*` 宏中修改。

---

*规格来源：ZNCZ-001 版本一产品说明（智能插座 WiFi 定时远程控制）。*
