# ZNCZ-001 智能插座 — 产品功能规格

> 版本一：继电器 + 定时 + 手动/自动模式 + WiFi  
> MCU：STM32F103C8T6 | 通讯：ESP-01S | 时钟：DS1302 | 显示：OLED

---

## 1. 产品概述

基于 STM32 单片机的智能插座，通过 ESP-01S WiFi 模块与 Android APP（MQTT JSON）通信，实现远程查看/控制继电器、校时及定时开关。

固件采用 **协作调度 + `sched_loop`** 组织任务，UI 导航由 **`app_fsm`** 状态表驱动（`app/app_logic.c` 内 `app_ui_transitions[]`）。

---

## 2. 硬件清单

| 器件 | 型号/说明 | 驱动/接口 |
| --- | --- | --- |
| 主控 | STM32F103C8T6 | BSP + HAL |
| 显示 | OLED（I2C SSD1306） | `display_driver_t` / `"oled"` |
| 继电器 | 单路 | `relay_driver_t` / `"relay"` |
| 实时时钟 | DS1302 | `rtc_driver_t` / `"ds1302"` |
| WiFi | ESP-01S | `comm_driver_t` / `"esp8266"` + `esp8266_mqtt` |
| 按键 | 5 个（见 §3） | `input_driver_t` / `"key"` |
| 指示灯 | LED（可选） | `misc_driver_t` / `"led"` |

驱动选用：`cmake/driver_catalog.cmake` → `DRIVER_CATALOG_ZNCZ_001`。

---

## 3. 按键定义

| 按键 | 手动主界面 | WiFi 子界面 | 定时模式 | 全局 |
| --- | --- | --- | --- | --- |
| 键一 | — | — | — | 切换 **手动 / 定时**；**上电默认手动主界面** |
| 键二 | 继电器开/关 | **返回手动主界面** | 切换编辑项（当前/开/关时间） | — |
| 键三 | 进入 WiFi SSID 页 | 切到 SSID 页 | 切换时/分/秒位 | — |
| 键四 | 进入 WiFi 密码页 | 切到密码页 | 当前位 **+1** | — |
| 键五 | 进入 WiFi 状态页 | 切到状态页 | 当前位 **-1** | — |

消抖：50 ms（`app_main.c` → `key_loop`）。

---

## 4. 显示界面

### 4.1 手动模式 — 主界面（`APP_FSM_STATE_MANUAL_MAIN`）

- 标题：`Mode:MANUAL`
- 继电器 **ON/OFF**
- **当前时间** HH:MM:SS（DS1302）

### 4.2 手动模式 — WiFi 子界面

| FSM 状态 | 显示内容 |
| --- | --- |
| `MANUAL_WIFI_SSID` | SSID（`BOARD_ESP8266_WIFI_SSID`） |
| `MANUAL_WIFI_PASS` | 密码（`BOARD_ESP8266_WIFI_PASS`） |
| `MANUAL_WIFI_STATUS` | `connected` / `offline`（MQTT 就绪） |

底部提示：`K2:Back`。

### 4.3 定时模式（`APP_FSM_STATE_TIMER`）

- 标题：`Mode:TIMER`
- 三行可编辑时间：`*Cur` / ` On` / `Off`（星标 = 当前字段，`>` = 当前 digit）
- 行末：继电器当前 ON/OFF（由开/关时间与 RTC 比较得出）

---

## 5. 定时逻辑

| 项 | 说明 |
| --- | --- |
| 当前时间 | DS1302；APP `set_time` 可写 RTC |
| 打开时间 | 默认 08:00:00；到达区间内 **吸合** |
| 关闭时间 | 默认 22:00:00；到达区间外 **断开** |
| 跨日 | 若 `on_time > off_time`，视为跨午夜区间（见 `app_timer_should_relay_on()`） |
| 存储 | 开/关时间当前在 **RAM**，掉电丢失 |

---

## 6. WiFi / APP 协议（MQTT JSON）

Topic 与凭据见 `app/board_config.h` 中 `BOARD_ESP8266_MQTT_*`（默认订阅 `ZNCZ_001/cmd`，发布 `ZNCZ_001/telemetry`）。

### 6.1 APP → 设备（下行）

| 命令 | 字段示例 | 行为 |
| --- | --- | --- |
| 校时 | `{"cmd":"set_time","hour":12,"minute":30,"second":0,"year":2026,"month":5,"day":25}` | 写 DS1302，刷新显示，上报状态 |
| 设开时间 | `{"cmd":"set_on_time","hour":8,"minute":0,"second":0}` | 更新开时间；定时模式下重算继电器 |
| 设关时间 | `{"cmd":"set_off_time","hour":22,"minute":0,"second":0}` | 更新关时间；定时模式下重算继电器 |
| 直接控继电器 | `{"cmd":"relay","state":1}` | FSM 切回 **手动主界面** 并设继电器 |
| 查询状态 | `{"cmd":"get_status"}` | 发布 telemetry JSON |

### 6.2 设备 → APP（上行）

```json
{
  "relay": 1,
  "mode": "manual",
  "time": "12:30:45",
  "on_time": "08:00:00",
  "off_time": "22:00:00",
  "wifi": "connected"
}
```

`mode` 为 `manual` 或 `timer`（由 `app_ui_fsm.state` 判定）。

---

## 7. 软件架构

### 7.1 任务（`sched_loop`）

| Loop 名 | 周期 | 职责 |
| --- | --- | --- |
| `logic_loop` | 事件驱动 | `app_logic_loop`：JSON、刷新 OLED |
| `comm_loop` | 事件驱动 | `esp8266_mqtt_poll`、原始 UART 排空 |
| `key_loop` | 1 ms  tick 轮询 | 5 键消抖 → `app_logic_on_key` → `APP_EVENT_KEY` |
| `rtc_loop` | 1000 ms | `app_logic_on_second_tick`：读 RTC、定时控继电器 |

### 7.2 UI 状态机（`app_fsm`）

| 状态 ID | 含义 |
| --- | --- |
| 0 | 手动主界面 |
| 1 | WiFi SSID |
| 2 | WiFi 密码 |
| 3 | WiFi 连接状态 |
| 4 | 定时模式 |

事件：`KEY1`–`KEY5`（与物理键一致）、`COMM_RELAY`（MQTT 遥控）。  
转移表：`app_logic.c` → `app_ui_transitions[]`（约 30 条）。

业务数据（RTC、开/关时间、继电器）保存在 `app_context_t`；FSM 仅负责 **界面与模式导航**。

---

## 8. 实现状态（2026-05）

| 功能 | 状态 |
| --- | --- |
| OLED 显示 + 双模式 UI | ✅ |
| DS1302 + APP 校时 | ✅ |
| 5 键 + `app_fsm` 导航 | ✅ |
| 定时开/关 + 自动继电器 | ✅ |
| ESP8266 MQTT JSON | ✅ |
| `sched_loop` 四任务 | ✅ |
| `export_project.py` 独立导出 | ✅ |

**构建指标**（`projects/ZNCZ_001` 模板内编译）：Flash ~**37.0 KB**，RAM ~**8.2 KB**。

### APP JSON 示例

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

## 9. 上位机（uni-app + Vite + Capacitor）

- 目录：`projects/ZNCZ_001/ZNCZ_001_upper_ui/`
- 技术栈：Vue3 + uni-app + Vite，内置 `src/features/`（`common` / `wifi` / `ble` 示例）与 `version_features.json`，可根据固件版本裁剪功能。
- 目标平台：
  1. **H5/Web**：`npm run dev:h5` / `npm run build:h5`
  2. **微信小程序**：`npm run dev:mp-weixin` / `npm run build:mp-weixin`
  3. **Android（Capacitor）**：`npx cap sync android`，可通过 `npx cap open android` 打开 Android Studio。

### 9.1 功能模块

| 模块 | 功能 | 数据来源 |
| --- | --- | --- |
| 实时状态卡片 | 显示 `relay/mode/time/on_time/off_time/wifi`，支持一键刷新 | MQTT `ZNCZ_001/telemetry` |
| 手动控制面板 | 继电器开/关、模式显示、即时状态 | `relay`, `get_status` |
| 定时配置向导 | 编辑开/关时间（时/分/秒）并提交 | `set_on_time`, `set_off_time` |
| 校时工具 | 选择日期时间并发送 `set_time` | `set_time` |
| WiFi/云连接页 | 展示 SSID/密码/连接状态、重试按钮 | `wifi` 字段 + `get_status` |

### 9.2 交互流程

1. **连接设置**：启动 H5/Android，填写 MQTT Broker 地址/Topic（默认读取 `src/config/mqtt.ts`）。
2. **数据刷新**：应用启动即发送 `get_status`，所有控制操作完成后再次请求状态，保持 UI 与固件同步。
3. **命令映射**：UI 下发 JSON 与 §6 命令一致，服务端/固件无需额外转换。
4. **版本裁剪**：修改 `version_features.json`，配合 `export_project.py` 仅导出对应 feature 的源码与构建产物。

### 9.3 构建与同步

```
# H5 / 小程序
npm run build:h5
npm run build:mp-weixin

# Android 同步
npx cap sync android

# 打开 Android Studio
npx cap open android
```

H5 产物位于 `dist/build/h5`，微信小程序位于 `dist/build/mp-weixin`，Android 资产目录由 Capacitor 自动指向 `dist/build/h5`。

---

## 10. 引脚配置（board_config.h）

| 功能 | 引脚 |
| --- | --- |
| 继电器 | PA11 |
| ESP8266 | USART3 PB10/PB11，CH_PD PB0，RST PB1 |
| DS1302 | CE PB8，DATA PB9，SCLK PB15 |
| OLED I2C | PB6/PB7 |
| 按键 1–5 | PB4，PB5，PB12，PB13，PB14 |
| 调试软串口 TX（可选） | PC13，`HAL_DEBUG_UART_ENABLE` |

---

## 11. 后续可选增强

| 项 | 说明 |
| --- | --- |
| 开/关时间掉电保存 | Flash / EEPROM |
| ESP8266 断线重连 | 周期性重连策略 |
| 实板 + Android APP 联调 | 引脚与 JSON 字段对齐 |

---

## 12. 构建与导出

见 [readme.txt](./readme.txt)。

- WiFi/MQTT：`app/board_config.h` → `BOARD_ESP8266_*`
- 独立交付目录：`python ../../tools/export_project.py --project ZNCZ_001 -o ../../exports --extras readme.txt,PRODUCT_SPEC.md`

---

*规格来源：ZNCZ-001 版本一（智能插座 WiFi 定时远程控制）；软件架构与 2026-05 代码同步。*
