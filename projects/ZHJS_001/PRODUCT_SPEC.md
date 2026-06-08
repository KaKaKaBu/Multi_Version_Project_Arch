# ZHJS-001 智慧照明 — 产品功能规格

> 版本一：光照 + 红外×3 + 照明×3 + 自动/手动模式  
> MCU：STM32F103C8T6 | 显示：OLED（SSD1306 I2C）

---

## 1. 功能概述

| 序号 | 功能 |
| --- | --- |
| 1 | STM32F103C8T6 数据处理 |
| 2 | OLED 实时显示：光照强度%、有无人、灯光挡位 |
| 3 | GL5506 光敏电阻检测室外光照（ADC） |
| 4 | E18-D80NK×3 检测室内有无人（GPIO 电平，任一路有效即“有人”） |
| 5 | 键一：切换自动 / 手动模式 |
| 6 | 手动：键二切换设备 1–3；键三开关；键四循环挡位 0–3 |
| 7 | 自动：按光照分档控灯；人离开后约 10 s 全关 |

---

## 2. 自动模式挡位

| 环境光照（%） | 挡位 | 说明 |
| --- | --- | --- |
| ≥ 85% | 0 | 关灯 |
| 55% < 光照 < 85% | 1 | 1 档 |
| 25% < 光照 ≤ 55% | 2 | 2 档 |
| < 25% | 3 | 3 档 |

- 检测到有人：按上表对 **三路照明同步** 设挡。
- 无人：启动 **10 s** 延时，到期三路全关。
- 再次有人：取消延时并重新分档。

---

## 3. 手动模式按键

| 按键 | 功能 |
| --- | --- |
| 键一 | 自动 ↔ 手动 |
| 键二 | 循环选中灯 1 / 2 / 3 |
| 键三 | 当前灯开/关（开时若挡位为 0 则置 1 档） |
| 键四 | 当前灯挡位 +1（0→1→2→3→0；挡位 0 时关） |

---

## 4. 硬件与驱动

| 器件 | 驱动名 | 接口 |
| --- | --- | --- |
| OLED | `oled` | `display_driver_t` |
| GL5506 | `gl5506` | `analog_probe_t`（0–100%） |
| E18×3 | `presence` | `analog_probe_t`（0/1） |
| 照明×3 | `light1`–`light3` | `relay_driver_t` |
| 按键×4 | `key` | `input_driver_t` |

CMake：`DRIVER_CATALOG_ZHJS_001`（`cmake/driver_catalog.cmake`）。

---

## 5. 引脚（board_config.h）

| 功能 | 引脚 |
| --- | --- |
| GL5506 ADC | PA0 |
| 照明 1–3 | PB3 / PB4 / PB5 |
| OLED I2C | PB6 / PB7 |
| 按键 1–4 | PB8 / PB9 / PB10 / PB11 |
| E18 1–3 | PB12 / PB13 / PB14 |

极性可在板级修改：`BOARD_E18_PIR_ACTIVE_LOW`、`BOARD_GL5506_INVERT`。

---

## 6. 软件任务（sched_loop）

| Loop | 周期 | 职责 |
| --- | --- | --- |
| `sensor` | 200 ms | 采样光照/红外，自动模式控灯 |
| `key` | 1 ms tick | 4 键消抖 → `app_logic_on_key` |
| `app_logic` | 事件 | OLED 刷新 |

---

## 7. 构建

```bash
cd project_template/projects/ZHJS_001
cmake -S . -B build -G Ninja ^
  -DCMAKE_TOOLCHAIN_FILE=../../cmake/stm32-gcc-toolchain.cmake
cmake --build build
```

构建指标（模板内编译，2026-06）：Flash ~**14.0 KB**，RAM ~**5.4 KB**，`.driver_list` **7** 条（oled、gl5506、presence、light1–3、key）。

导出：`python ../../tools/export_project.py --project ZHJS_001 -o ../../exports --extras readme.txt,PRODUCT_SPEC.md`

---

*规格：ZHJS-001 版本一；与 `projects/ZHJS_001` 固件同步。*
