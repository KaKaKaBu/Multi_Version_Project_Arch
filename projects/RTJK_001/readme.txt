RTJK_001 — 人体健康监测仪
================================

本工程覆盖 10 个固件版本（`APP_VERSION=1..10`），通过 CMake 条件与 `version_config.h` 功能宏在单一工程内切换心率、血氧、血压、温度、BLE、WiFi、语音等组合。下文说明目录结构、版本矩阵、板级配置以及构建与导出流程，便于在 `project_template` 模板中维护或派生新版本。

1. 目录速览
------------

```
projects/RTJK_001/
├── CMakeLists.txt         # 设置 APP_VERSION、HAL 选项、驱动清单
├── PRODUCT_SPEC.md        # 功能规格、通信协议与 UI 交互
├── readme.txt             # 本文档
├── app/
│   ├── app_main.c         # 启动顺序、sched_loop/task 注册
│   ├── app_logic.c        # 业务逻辑（模式切换、遥测格式化等）
│   ├── app_callbacks.c    # device_manager 回调、comm_port 事件
│   └── version_config.h   # APP_VERSION 功能矩阵与 APP_EVENT_*
├── board/
│   └── board_config.h     # 项目级引脚/USART/I2C/MQTT 配置
└── RTJK_001_upper_ui/     # Uni-app + Vite + Capacitor 上位机
```

> `cmake/driver_catalog.cmake` 和 `cmake/hal_options.cmake` 由模板复用；本项目只需在 `CMakeLists.txt` 里选择版本对应的驱动与 HAL 模块。

2. 版本切换与 HAL 前置条件
------------------------------

1. **CMake 变量**：`APP_VERSION=1..10`（默认 10）。必须为 `CACHE STRING`，切勿用 `option()`，否则 CLion 会把数值视为布尔量。
2. **CLion**：Settings → CMake → CMake options 添加 `-DAPP_VERSION=10`，或在 CMake Profiles 指定。
3. **手工覆盖**：`app/version_config.h` 中的 `#define APP_VERSION` 仅在 CMake 未定义时生效；保持与构建参数一致。

> `CMakeLists.txt` 中：
>
> - `if(APP_VERSION GREATER_EQUAL 8) set(HAL_ADC_ENABLE ON ...)` —— V8+ 自动开启 ADC/HAL ADC DMA 选项（供 MSP20 血压传感器）。
> - `set(DRIVER_SRCS ${DRIVER_CATALOG_RTJK_001})` —— 驱动列表由 `driver_catalog.cmake` 根据版本追加。

3. 功能矩阵（依据 `version_config.h`）
---------------------------------------

| 版本 | SPO2 | 温度 | BLE | WiFi/MQTT | 语音 | 血压 | 备注 |
| ---- | ---- | ---- | --- | --------- | ---- | ---- | ---- |
| 1    | ✓    | ✗    | ✗  | ✗         | ✗    | ✗    | 基础心率 + OLED/蜂鸣/LED/按键 |
| 2    | ✓    | ✗    | ✓  | ✗         | ✗    | ✗    | JDY31 BLE 切换为主要通信 |
| 3    | ✓    | ✗    | ✗  | ✓         | ✗    | ✗    | 引入 ESP8266 WiFi/MQTT |
| 4    | ✓    | ✓    | ✗  | ✗         | ✗    | ✗    | 增加 DS18B20 温度 |
| 5    | ✓    | ✓    | ✓  | ✗         | ✗    | ✗    | BLE + 温度 |
| 6    | ✓    | ✓    | ✗  | ✓         | ✗    | ✗    | WiFi + 温度 |
| 7    | ✓    | ✓    | ✗  | ✗         | ✓    | ✗    | 启用语音播报（SU03T） |
| 8    | ✗    | ✗    | ✗  | ✗         | ✗    | ✓    | MSP20 血压（需 HAL_ADC_ENABLE=ON） |
| 9    | ✗    | ✓    | ✗  | ✗         | ✗    | ✓    | 血压 + 温度 |
| 10   | ✓    | ✓    | ✗  | ✓         | ✗    | ✓    | 默认出货：心率/血压/温度/WiFi + 声光报警 |

通用特性：OLED 显示、蜂鸣器、LED、按键、脉搏读取始终启用。`BOARD_COMM_DEVICE` 会根据版本在 `board_config.h` 中切换 `esp8266`/`jdy31`/`none`。

4. 板级配置入口（`board/board_config.h`）
----------------------------------------

- **通信模块**：`BOARD_ESP8266_*` WiFi/MQTT 参数、`BOARD_JDY31_*` BLE UART 以及 `BOARD_COMM_DEVICE` 决定 `comm_port` 默认实例。
- **传感器/执行器引脚**：所有 GPIO/I2C/USART 引脚、Remap、外设实例均在此定义，驱动不得写死寄存器。
- **版本特性**：根据 `VERSION_FEATURE_*` 选择通信设备或语音模块，同时保持与 `version_config.h` 功能宏一致。
- **调试串口**：如需软串口日志，开启 `HAL_DEBUG_UART_ENABLE` 并提供 `board_debug_uart_tx`。

> 若扩充硬件（如改用 USART3 或新增 SPI 设备），请同步更新 `board_config.h`、`driver_catalog.cmake`（选择对应驱动）、以及必要时的 HAL 选项。

5. 构建流程
------------

```
cmake -S . -B build -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE=../../cmake/stm32-gcc-toolchain.cmake \
  -DCMAKE_MAKE_PROGRAM="<Ninja路径>/ninja.exe" \
  -DAPP_VERSION=10
cmake --build build
```

- 工具链：`STM32CubeCLT` 提供的 arm-none-eabi 与 Ninja，路径可在 CMake cache 中覆盖。
- 产物：`${PROJECT_NAME}.elf` + `.bin`（位于 `build/` 或 IDE 生成目录）。
- 常见故障：若 `APP_VERSION` 越界或被解析为 `ON/OFF`，CMake 会在版本校验阶段报错。

6. 导出与批量版本验证
-------------------------

```
python ../../tools/export_project.py --project RTJK_001 --version 10 -o ../../exports
python ../../tools/export_project.py --project RTJK_001 --batch --versions 1-10 -o ../../exports --clean \
  --extras readme.txt,PRODUCT_SPEC.md
```

- 导出脚本会解析 `driver_catalog.cmake` 与 `hal_options.cmake`，为每个版本生成可独立构建的目录并复制上位机。
- 修改 `CMakeLists.txt`、`driver_catalog.cmake` 或上位机 `version_features.json` 后，务必重新执行批量导出并在导出目录内跑一遍 `cmake --build`。

7. 快速排障提示
----------------

1. **驱动缺失**：确认 `version_config.h` 的 `VERSION_FEATURE_*` 与 `driver_catalog.cmake` 条件一致（例如 V8+ 必须包含 `msp20_bp.c`）。
2. **通信无输出**：检查 `comm_port_bind(0)` 后 `comm_port_driver()->name` 是否与 `BOARD_COMM_DEVICE` 一致；WiFi 版本需确保 `esp8266_mqtt_connect()` 返回 0。
3. **ADC 采样异常**：V8+ 需在 CMake cache 中看到 `HAL_ADC_ENABLE`、`HAL_ADC_ENABLE_DMA` 为 `ON`，并在 `board_config.h` 中配置 `BOARD_ADC_*` 引脚/通道。

WiFi/MQTT 账号与主题定义在 `board_config.h` → `BOARD_ESP8266_*` 宏；更多协议字段请参考 `PRODUCT_SPEC.md`。
