# 模板工程完成度评估

> 评估基准：`project_template/` 当前代码与 **2026-05 导出构建回归**（11 个独立工程全部编译通过）。  
> 架构说明见 [architecture.md](./architecture.md)，维护约定见 [maintenance_guide.md](./maintenance_guide.md)。

---

## 1. 总体结论

| 维度 | 完成度 | 说明 |
| --- | --- | --- |
| 框架核心（调度 / 自注册 / devmgr / HAL） | **高（~95%）** | `sched_loop`、HAL 选项齐全，启动流程稳定 |
| 标准接口与驱动库 | **高（~90%）** | **20** 类接口、**28** 个驱动 `.c`；协议完成度因器件而异 |
| 参考 Demo（ZNCZ_001 / RTJK_001 / ZHJS_001） | **高（~90%）** | 三产品线逻辑已实现，待实板全量联调 |
| 工具链与构建 | **高（~95%）** | CMake + Ninja + `driver_catalog` + `export_project.py`（含上位机版本化导出） |
| 文档 | **高（~90%）** | 架构/维护/完成度已同步，并覆盖 uni-app 上位机、`version_features.json`、导出目录结构 |

**综合评估：模板已可用于二次开发与多版本交付。** 新建项目可按 `gen_project.py`、复制 `ZNCZ_001` / `RTJK_001` 或 `driver_all_test` 起步。

---

## 2. 构建指标（导出目录回归，2026-05）

| 目标 | Flash (text+data) | RAM (data+bss) | 说明 |
| --- | --- | --- | --- |
| `exports/ZNCZ_001` | **36 424 B** (~35.6 KB) | **8 160 B** | 7 条 `.driver_list`；`sched_loop` 四任务 |
| `exports/RTJK_001_version1` | **29 864 B** (~29.2 KB) | **7 488 B** | 6 驱动；仅心率基础版 |
| `exports/RTJK_001_version10` | **39 784 B** (~38.9 KB) | **8 512 B** | 10 驱动；含 ADC/血压/WiFi/语音相关驱动 |
| `projects/driver_all_test` | **19 684 B** (~19.2 KB) | **3 336 B** | 26 条 `.driver_list`（sg90 双实例） |
| `projects/ZHJS_001` | **14 336 B** (~14.0 KB) | **5 984 B** | 7 条 `.driver_list`；光照+红外+三路灯控 |

RTJK V1–V10 Flash 范围约 **29.9–38.0 KB**（随 `driver_catalog` 选源变化）。  
导出命令与回归步骤见 [maintenance_guide.md §10.1 / §12.6](./maintenance_guide.md)。

---

## 3. 模块完成度明细

### 3.1 框架与公共层

| 模块 | 路径 | 状态 | 备注 |
| --- | --- | --- | --- |
| 协作调度器 | `common/scheduler/scheduler.*` | ✅ | 静态任务、事件阻塞/唤醒 |
| 框架 Loop | `common/scheduler/sched_loop.*` | ✅ | **ZNCZ_001 / RTJK_001 已用**；封装周期 poll |
| 中断事件桥 | `common/irq_event/` | ✅ | USART RX → 应用事件 |
| 驱动自注册 | `common/driver_core/` | ✅ | `.driver_list` 链接段 |
| 设备管理器 | `common/device_manager/` | ✅ | **20** 类 `devmgr_get_*`（含血压） |
| 应用 FSM | `common/app_framework/app_fsm.*` | ✅ | **ZNCZ_001 已接入** UI 导航（5 状态 / 30 条转移） |
| 通讯端口抽象 | `common/utils/comm_port.*` | ✅ | `BOARD_COMM_DEVICE` 绑定 |
| 静态内存池 | `common/utils/mem_pool.*` | ✅ | 覆盖 `malloc/free/realloc` |
| cJSON 裁剪 | `common/utils/cJSON.*` + `cjson_port.*` | ✅ | `CJSON_NESTING_LIMIT=32` |
| 轻量 printf | `common/utils/tiny_printf.*` | ✅ | 替代 libc `sprintf` |
| 调试软串口 | `soft_uart` + `debug_uart` | ✅ | `HAL_DEBUG_UART_ENABLE` |

### 3.2 BSP / HAL

| 模块 | 状态 | 备注 |
| --- | --- | --- |
| GPIO | ✅ | |
| I2C 软/硬 | ✅ | 各 2 路逻辑槽位（I2C1/I2C2）；含总线恢复 |
| USART IRQ / DMA TX | ✅ | **RX 无 DMA**，IRQ 环形缓冲 |
| Timer PWM | ✅ | |
| SPI 软/硬 + DMA | ✅ | 互斥选项；`SPI_HAL_BUS_MAX=2`；RC522 需 `-DHAL_SPI_USE_HW=ON` |
| ADC 轮询 / DMA | ✅ | RTJK V8+ 通过 CMake preamble 开 `HAL_ADC_ENABLE` |
| `hal_options.cmake` | ✅ | 编译期注入 `hal_features.h`；导出由 `hal_export_utils.cmake` 扫描 |

### 3.3 驱动库（28 个 `.c` 源文件）

| 类别 | 数量 | 状态 | 备注 |
| --- | --- | --- | --- |
| 传感器 | 13 | ✅ 已迁移 | 含 `msp20_bp.c`（RTJK 血压 ADC） |
| 执行器 | 3 | ✅ | sg90（双实例）、stepmotor、relay |
| 通讯 | 6 | ✅ / ⚠️ | 见下表 |
| 显示 | 3 | ✅ | OLED 字库 `oled_font.c` + `gen_oled_font.py` |
| 杂项 | 3 | ✅ | led / buzzer / key |

**通讯驱动分级：**

| 驱动 | 注册类型 | 完成度 | 说明 |
| --- | --- | --- | --- |
| esp8266_wifi | COMM (STREAM) | ✅ 高 | AT + 768B RX 流 |
| esp8266_mqtt | —（协议层） | ✅ 高 | WiFi/MQTT + cJSON 遥测/命令 |
| jdy31_ble | COMM (STREAM) | ⚠️ 中 | 透传框架；RTJK V2/V5 |
| su03t_voice | COMM (STREAM) | ⚠️ 中 | 语音模块透传；RTJK V7 |
| nrf24l01 | COMM (PACKET) | ⚠️ 中 | 无 IRQ，需轮询 `comm_port_recv` |
| l76k_gnss | GNSS | ⚠️ 中 | NMEA 框架 |

**传感器（参考工程相关）：**

| 驱动 | 接口 | 完成度 | 说明 |
| --- | --- | --- | --- |
| max30102 | pulse_oximeter | ✅ 高 | RTJK 心率 |
| msp20_bp | blood_pressure | ✅ 高 | RTJK V8+；ADC 采集 |
| ds18b20 | temp_hum_sensor | ✅ 中 | RTJK V4–7/V9+ |
| ds1302_rtc | rtc | ✅ 中 | ZNCZ 定时 |

### 3.4 参考应用 ZNCZ_001（智能插座）

**产品规格**：[projects/ZNCZ_001/PRODUCT_SPEC.md](../projects/ZNCZ_001/PRODUCT_SPEC.md)

| 能力 | 状态 |
| --- | --- |
| 手动模式：继电器 + 时间 / WiFi 界面 | ✅ |
| 定时模式：开/关时间编辑 + 自动控继电器 | ✅ |
| 5 键 UI + 消抖 | ✅ |
| DS1302 RTC | ✅ |
| ESP8266 MQTT JSON | ✅ |
| `sched_loop` 四任务 + `app_fsm` UI 状态机 | ✅ |

### 3.5 参考应用 RTJK_001（健康监测）

**产品规格**：[projects/RTJK_001/PRODUCT_SPEC.md](../projects/RTJK_001/PRODUCT_SPEC.md)

| 能力 | 状态 |
| --- | --- |
| `RTJK_VERSION` 1–10 条件编译 + `driver_catalog` 选驱动 | ✅ |
| 心率（MAX30102）、阈值/自动模式、OLED UI | ✅ |
| 体温（DS18B20）、血压（MSP20 ADC，V8+） | ✅ |
| WiFi MQTT / BLE JSON 远程查看与设阈（按版本） | ✅ |
| 声光报警、SU03T 语音（V7+） | ✅ |
| `sched_loop` 多任务 + `export_project.py` 十版导出 | ✅ |

---

## 4. 工具链

| 工具 | 路径 | 状态 |
| --- | --- | --- |
| `gen_project.py` | `tools/` | ✅ 生成应用骨架 + Uni-app/Capacitor 上位机 scaffold（含 `src/features/`、`version_features.json`） |
| `export_project.py` | `tools/` | ✅ 多版本独立目录导出，额外构建并裁剪对应版本的上位机代码；见 maintenance §12 |
| `resolve_export.cmake` | `tools/cmake/` | ✅ HAL + driver_catalog 解析 |
| `driver_catalog.cmake` | `cmake/` | ✅ 工程/版本选源唯一入口 |

---

## 5. 已知缺口与后续建议

| 优先级 | 项 | 说明 |
| --- | --- | --- |
| **中** | 实板联调 | 两参考工程主要外设需硬件验证 |
| 中 | 掉电保存 | 定时/阈值等默认 RAM，未持久化 |
| 中 | ESP8266 断线重连 | 启动连接为主，重连策略不完整 |
| 低 | `gen_project.py` | 未生成 `app_logic` / `comm_port` / `sched_loop` 骨架 |
| 低 | `app_fsm` 在 RTJK_001 复用 | ZNCZ 已有样例，可按需移植 |

---

## 6. 文档索引

| 文档 | 用途 |
| --- | --- |
| [architecture.md](./architecture.md) | 分层、HAL、sched_loop、通讯/JSON、导出概览 |
| [maintenance_guide.md](./maintenance_guide.md) | 新增驱动、禁止行为、export 约定、§11 限制 |
| [projects/ZNCZ_001/PRODUCT_SPEC.md](../projects/ZNCZ_001/PRODUCT_SPEC.md) | 智能插座功能与 JSON |
| [projects/RTJK_001/PRODUCT_SPEC.md](../projects/RTJK_001/PRODUCT_SPEC.md) | 健康监测十版、引脚、JSON |
| 仓库根 `嵌入式统一模块化架构技术说明.md` | 原始设计目标 |

---

*最后更新：同步上位机脚手架条件编译、`version_features.json`、嵌入式+上位机一体化导出（2026-06）。*
