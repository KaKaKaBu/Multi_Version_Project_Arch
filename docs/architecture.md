# 嵌入式统一模块化架构模板

该模板基于 STM32F10x 标准外设库（SPL），用于构建资源静态分配、驱动自注册、事件驱动协作调度的嵌入式项目。

## 分层

| 层次 | 目录 | 职责 |
| --- | --- | --- |
| 应用层 | `projects/<NAME>/app` | 业务逻辑（`app_logic`）、回调、任务注册 |
| 板级配置 | `projects/<NAME>/board/board_config.h` | 引脚、外设实例、波特率、WiFi/MQTT、HAL 传输模式 |
| 板级设备表 | `projects/<NAME>/board/board_devices.c` | `REGISTER_BOARD_DEVICE` 描述板上设备实例，将项目引脚/总线配置传给驱动 |
| 版本配置 | `projects/<NAME>/app/version_config.h` | `VERSION` 宏、功能/通讯开关、应用事件 |
| 上位机多端脚手架 | `projects/<NAME>/<NAME>_upper_ui_flutter` / `projects/<NAME>/<NAME>_upper_ui` | 默认 Flutter 主上位机；仅在需要微信小程序或兼容历史项目时保留 Vue3 + Vite + uni-app + Capacitor 栈。KQZL3 Flutter 覆盖 App/Web 版本，uni-app 仅用于 v9 微信小程序，双栈各自用 `version_features.json` 记录版本功能清单 |
| 设备管理层 | `common/device_manager` | 遍历驱动注册段，统一初始化与类型安全查询 |
| 调度器层 | `common/scheduler` | 协作任务、`sched_loop` 周期 poll、事件阻塞/唤醒 |
| 应用框架 | `common/app_framework` | `app_fsm`（**ZNCZ_001** UI 状态表驱动） |
| 工具层 | `common/utils` | `comm_port`、`mem_pool`、`cJSON`/`cjson_port`、`tiny_printf`、`soft_uart`/`debug_uart` |
| 接口层 | `common/interfaces` | **20** 类标准接口（传感器、显示、执行器、通讯、血压等） |
| 驱动层 | `drivers` | 芯片/模块通用驱动，`REGISTER_DRIVER` 注册驱动实现，不 include `board_config.h` |
| HAL 接口层 | `hal_wrapper` | 平台无关 HAL 头文件与公共类型 |
| BSP 层 | `bsp/stm32/hal` / `bsp/mcs51/hal` | 各平台 HAL 实现；STM32 实现对 SPL GPIO/I2C/USART/SPI/TIMER/ADC 二次封装 |
| BSP 公共 | `bsp/<platform>/hal/hal_common.c` | 平台初始化、计时与通用等待 |
| 构建/导出 | `cmake/`、`tools/` | `hal_options`、`driver_catalog`、`gen_project.py`、`export_project.py` |

## HAL 功能开关

通过 `cmake/hal_options.cmake` 中的 CMake option 控制，编译期注入 `hal_features.h` 宏：

| 选项 | 宏 | 说明 |
| --- | --- | --- |
| `HAL_I2C_USE_SOFT` | `HAL_I2C_USE_SOFT=1` | 软件 I2C（`i2c_hal_soft.c`），不链接 `stm32f10x_i2c.c` |
| `HAL_SPI_USE_SOFT` | `HAL_SPI_USE_SOFT=1` | 软件 SPI（`spi_hal_soft.c`） |
| `HAL_SPI_USE_HW` | `HAL_SPI_USE_HW=1` | 硬件 SPI（`spi_hal_hw.c` + `stm32f10x_spi.c`），与软 SPI **互斥** |
| `HAL_SPI_ENABLE_DMA` | `HAL_SPI_ENABLE_DMA=1` | 硬件 SPI DMA TX/RX（需 `HAL_SPI_USE_HW`） |
| `HAL_USART_ENABLE_DMA` | `HAL_USART_ENABLE_DMA=1` | USART DMA TX（`usart_hal_dma.c`） |
| `HAL_ADC_ENABLE` | `HAL_ADC_ENABLE=1` | ADC 轮询采集（`adc_hal.c` + `stm32f10x_adc.c`） |
| `HAL_ADC_ENABLE_DMA` | `HAL_ADC_ENABLE_DMA=1` | ADC DMA 连续采集（需 `HAL_ADC_ENABLE`） |
| `HAL_DEBUG_UART_ENABLE` | `HAL_DEBUG_UART_ENABLE=1` | GPIO 软串口 TX 调试输出（`printf` → `_write`） |

`stm32f10x_dma.c` 在任一 DMA 选项开启时链接。

版本相关 HAL 开关（如 RTJK `VERSION>=8` 开 ADC）写在工程 `CMakeLists.txt` 的 **`include(hal_options.cmake)` 之前**，供 `export_project.py` 截取 preamble。

示例：

```bash
cmake -S . -B build -DHAL_SPI_USE_HW=ON -DHAL_SPI_ENABLE_DMA=ON -DHAL_ADC_ENABLE=ON
cmake --build build
```

## 驱动自注册

驱动文件只需要定义接口实例，并在文件底部使用 `REGISTER_DRIVER(type, instance)`。注册宏会把描述符放入 `.driver_list` 链接段，设备管理器通过链接脚本提供的 `__driver_list_start` 和 `__driver_list_end` 自动发现驱动实现。

板级设备实例由每个项目自己的 `board/board_devices.c` 显式注册：

```c
static const i2c_device_config_t board_oled_config = {
    BOARD_OLED_I2C,
    BOARD_OLED_I2C_SPEED,
    board_oled_i2c_scl,
    board_oled_i2c_sda,
    BOARD_OLED_I2C_REMAP,
    BOARD_OLED_I2C_ADDR
};
REGISTER_BOARD_DEVICE(DISPLAY, "oled", &board_oled_config);
```

`devmgr_init_all()` 会匹配板级设备实例与同名驱动实现，并调用驱动的 `init(config)`。这相当于简化版 DeviceTree/platform_data：驱动描述能力，板级表描述实例。

## 协作调度

调度器使用静态任务控制块，不分配动态内存。任务状态包括 `RUNNING`、`READY`、`BLOCKED`、`SUSPENDED`。任务通过 `task_block(events)` 等待事件，中断或驱动可调用 `event_set(events)` 唤醒匹配任务。

应用入口应首先调用 `bsp_init()` 完成 NVIC 分组与 SysTick 配置，再初始化调度器。

### sched_loop（推荐 poll 封装）

`common/scheduler/sched_loop.*` 将周期性或事件驱动的轮询封装为可注册协作任务：

```c
static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor", sensor_loop_run, 2, 500,
    SCHED_EVENT_TICK, APP_EVENT_SENSOR_READY);

void app_main(void) {
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();
    sched_loop_init();
    sched_loop_register(&sensor_loop);
    /* … */
    sched_start();
}
```

- `period_ms > 0`：按 tick 间隔执行 `run`
- `period_ms == 0`：每次被 `wait_events` 唤醒时执行  
`ZNCZ_001`、`RTJK_001` 已采用此模式，替代手写 `driver_task_t` 模板。

## BSP 错误码

HAL 层统一使用 `hal_status_t`：

- `HAL_OK`：成功
- `HAL_ERR_TIMEOUT`：等待超时
- `HAL_ERR_BUSY`：外设忙
- `HAL_ERR_NACK`：I2C 无应答
- `HAL_ERR_PARAM`：参数错误
- `HAL_ERR_IO`：IO 错误

## 板级配置

每个项目在 `board/board_config.h` 中集中定义 `hal_pin_t` 与外设实例，并在 `board/board_devices.c` 中把这些配置绑定到设备名。驱动层不得 include `board_config.h`，只能通过 `init(const void *config)` 接收板级配置。

新建项目时，`tools/gen_project.py` 会从 `bsp/board/board_config_template.h` 生成 `board/board_config.h` 骨架，并生成显式 `board/board_devices.c`。

C51 平台不承诺 scheduler/MQTT/BLE 等复杂能力；当前保留本地轮询和静态注册路径，避免把 STM32 链接段模型强加到小资源平台。

## HAL API（已移除旧版兼容接口）

统一使用配置结构体初始化：

- `i2c_hal_init(const i2c_hal_config_t *cfg)`
- `usart_hal_init(const usart_hal_config_t *cfg)` — 含 `tx_mode`（IRQ / DMA）
- `timer_hal_pwm_init(const timer_hal_pwm_config_t *cfg)`

已移除：`i2c_hal_init(I2Cx, speed)`、`i2c_hal_write_byte/read_byte`、`usart_hal_init(USARTx, baud)`、`timer_hal_pwm_init(TIM, period, prescaler)` 等旧 API。

## I2C 总线恢复

当 I2C 通信因从机挂死导致 SDA 被拉低时，可调用：

```c
i2c_hal_bus_recover(I2C1, board_oled_i2c_scl, board_oled_i2c_sda);
```

硬件/软件 I2C 后端均支持该接口。软/硬 I2C 各支持 **I2C1/I2C2 两路逻辑槽位**。

## 调试软串口（PC13 TX）

启用 `HAL_DEBUG_UART_ENABLE` 后，`bsp_init()` 会初始化 **仅 TX** 的 GPIO 位bang 串口，将 `printf` / `puts` 重定向到 `_write()` → `debug_uart` → `soft_uart`。

典型配置（Blue Pill PC13）：

```c
#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
static const hal_pin_t board_debug_uart_tx = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
#endif
```

**USART3 透传**（ESP8266 AT 对话）：`BOARD_DEBUG_UART_PASSTHROUGH_USART3 1` 可将 USART3 原始字节镜像到 PC13。

硬件连接：USB-TTL **RX** ← **PC13**，**GND** 共地，**9600 8N1**。

**引脚冲突**：若 `board_led_pin` 与 `board_debug_uart_tx` 同为 PC13，`led` 驱动会自动跳过 init/控制。

## USART DMA TX

启用 `HAL_USART_ENABLE_DMA` 后，在 `board/board_config.h` 中设置通讯模组所用 USART 的 TX 模式与 DMA 通道。**RX 仍为 IRQ 环形缓冲，无 DMA。**

```c
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_DMA
#define BOARD_ESP8266_USART_TX_DMA DMA1_Channel4
```

## 软件 SPI

启用 `HAL_SPI_USE_SOFT` 后使用 `spi_hal.h` API（`bus_id` 0/1，`SPI_HAL_BUS_MAX=2`）：

```c
spi_hal_config_t cfg = { board_spi_sck, board_spi_mosi, board_spi_miso, 0, 0, 1 };
spi_hal_init(0, &cfg);
spi_hal_write(0, data, len);
```

## 硬件 SPI

启用 `HAL_SPI_USE_HW`（与软 SPI 互斥）后：

```c
spi_hal_config_t cfg = {
    .instance = BOARD_HW_SPI1_INSTANCE,
    .sck = board_hw_spi1_sck,
    /* … */
#if HAL_SPI_ENABLE_DMA
    .tx_dma_channel = BOARD_HW_SPI1_TX_DMA,
    .rx_dma_channel = BOARD_HW_SPI1_RX_DMA,
#endif
};
spi_hal_init(0, &cfg);
spi_hal_transfer(0, tx, rx, len);
/* 或 */ spi_hal_dma_transfer(0, tx, rx, len, timeout_us);
```

## ADC 采集

启用 `HAL_ADC_ENABLE` 后轮询读取；`HAL_ADC_ENABLE_DMA` 支持连续 DMA 采集。RTJK_001 在 `APP_VERSION>=8` 时于 CMake preamble 打开 ADC，供 `msp20_bp` 血压采集。

## 通讯抽象（comm_if + comm_port）

流式与分包通讯统一为 `comm_driver_t`（`comm_if.h`）：

- `COMM_KIND_STREAM`：ESP8266、JDY-31、SU-03T 等 UART 透传
- `COMM_KIND_PACKET`：NRF24L01 等分包射频

应用层通过 `comm_port_bind(BOARD_COMM_DEVICE)` 绑定默认设备，再调用 `comm_port_send/recv`；有 USART RX 中断时可 `irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX)`。

## ESP8266 WiFi / MQTT

| 层 | 文件 | 职责 |
| --- | --- | --- |
| 驱动 | `drivers/comm/esp8266_wifi.c` | AT 指令、`comm_driver_t` |
| 协议 | `drivers/comm/esp8266_mqtt.c` | WiFi/MQTT 连接、JSON 遥测与下行解析 |

典型流程：

```c
comm_port_bind(0);
esp8266_mqtt_connect(&cfg);
esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
/* comm_loop 中 */ esp8266_mqtt_poll();
```

## 内存与 JSON

- **mem_pool**：静态池（默认 `MEM_POOL_SIZE=2048`）覆盖 `malloc/free/realloc`
- **cJSON**：裁剪版 + `cjson_port` hooks
- **tiny_printf**：减小 Flash 占用

链接时需保留 `-lm`，并通过 `-Wl,-u,cjson_port_init` 等确保初始化符号被链接。

## 显示接口

`display_driver_t`（`display_if.h`）提供 `init/clear/update/print(x,y,size,fmt,...)`。OLED 字库在 `drivers/displays/oled_font.c`，可由 `tools/gen_oled_font.py` 重新生成。

## 驱动目录与编译选用

全部迁移驱动见 `cmake/driver_catalog.cmake`。工程 **必须** 通过 catalog 变量选源（便于 `export_project.py` 解析）：

```cmake
include(${TEMPLATE_ROOT}/cmake/hal_options.cmake)
include(${TEMPLATE_ROOT}/cmake/driver_catalog.cmake)
set(DRIVER_SRCS ${DRIVER_CATALOG_ZNCZ_001})   # 或 DRIVER_CATALOG_RTJK_001
```

多版本条件（如 RTJK 1–10）写在 `driver_catalog.cmake` 内，与 `version_config.h` 宏保持一致。

## 多版本与导出

| 机制 | 说明 |
| --- | --- |
| `projects/<NAME>/` | 每产品线独立 `app/`、`board/`、`CMakeLists.txt`、`PRODUCT_SPEC.md` |
| `APP_VERSION` CACHE STRING | 统一数值版本号（禁止 `option()`）；旧项目专属版本宏已移除 |
| `driver_catalog.cmake` | 按版本 `list(APPEND …)` 选驱动 |
| `projects/<NAME>/<NAME>_upper_ui_flutter` | Flutter 主上位机目录；通过 `version_features.json` 描述固件版本→Flutter feature→构建参数 |
| `projects/<NAME>/<NAME>_upper_ui` | 可选 uni-app 小程序/历史兼容目录；通过独立 `version_features.json` 描述小程序版本→feature→路径 glob |
| `export_project.py` | 导出结构：`exports/<NAME>/KQZL3_version1/`（嵌入式裸工厂树）+ `exports/<NAME>/upper_ui/<NAME>_upper_ui_flutter/versions/<versionX>/`（Flutter 主上位机）+ 可选 `<NAME>_upper_ui/versions/<versionX>/`（uni-app 小程序） |

```bash
python tools/export_project.py --project RTJK_001 --batch --versions 1-10 -o ../exports --clean \
    --extras readme.txt,PRODUCT_SPEC.md
```

- 导出前 `export_project.py` 会在上位机目录执行对应平台构建：Flutter 项目执行 `flutter build apk --debug`（包含 `web` feature 时同时执行 `flutter build web`）；uni-app 小程序/历史项目执行 `npm run build:mp-weixin`，uni-app-only 历史项目仍可执行 H5/小程序构建。
- Flutter 与 uni-app 可分别拥有 `version_features.json`。Flutter 侧用于解析 `UPPER_VERSION` / `UPPER_FEATURES` 并决定导出哪些主上位机版本；uni-app 侧用于裁剪小程序源码与 dist。
- 双栈项目中，Flutter 主上位机只导出 Flutter 矩阵中显式覆盖的版本；uni-app 只导出包含 `mpWeixin` feature 的版本。KQZL3 因此由 Flutter 导出 v3/v4/v6/v7/v8/v10/v12/v13/v14，由 uni-app 导出 v9 微信小程序。
- Android Studio / 平台工程随各自栈保留：Flutter 的 `android/`、`web/` 保留在 `<NAME>_upper_ui_flutter/versions/<versionX>/`；uni-app 的 `android/` 与 Capacitor 配置仅用于小程序/历史兼容栈。

约定与禁止行为详见 [maintenance_guide.md §12](./maintenance_guide.md)。

## 上位机脚手架与版本特性

- `tools/gen_project.py` 在创建新项目时默认生成 Flutter 上位机（`<NAME>_upper_ui_flutter/`），调用 `flutter create --platforms=android,web` 并生成 `lib/core/version/version_capabilities.dart`、`version_features.json` 与基础 widget test。只有传入 `--with-mini-program` 或产品需求明确包含微信小程序时，才额外生成 Vue3 + Vite + uni-app + Capacitor 小程序栈（`<NAME>_upper_ui/`）。
- Flutter 侧通过 `--dart-define=UPPER_VERSION=<N>` 和 `--dart-define=UPPER_FEATURES=<list>` 注入版本与能力；页面和 service 只消费解析后的 capabilities，不在 Widget 中硬编码版本号。
- Flutter 通信日志由 `lib/core/config/debug_flags.dart` 中的 `upperUiShowCommunicationLog = kDebugMode` 控制：Debug 显示，Release/Profile 隐藏且不采集原始 TX/RX 日志。
- uni-app 侧的 `vite.config.js` 注入 `__UPPER_VERSION__`、`__UPPER_FEATURES__`、`__UPPER_FEATURE_FLAGS__`、`__UPPER_UI_DEBUG__` 四个编译期常量。模板的 `src/features/index.js` 会根据这些 flag 注册对应小程序能力（示例：`wifi`、`ble`）。
- 上位机架构只包含真实 MQTT/BLE/平台通道通信；不提供 mock transport、模拟上报、本地随机遥测或缺少远程能力时的模拟回退。
- 页面布局必须自适应窄屏：卡片头、按钮行、表单行、长 URL/topic/JSON 均需可换行，避免像素溢出。
- `version_features.json` 结构：
  - `defaultFeatures`：未显式匹配版本时启用的 feature 名称。
  - `featureGlobs`：每个 feature 对应的源码路径 glob，支持 `!` 前缀排除；主要用于 uni-app 小程序源码裁剪，Flutter 主工程通常整树复制但仍用该文件解析版本 feature。
  - `alwaysInclude`：不论版本都要导出的文件（package.json、平台配置、静态资源等）。
  - `versions.<N>.features`：覆盖某版本的 feature 列表（`N` 与固件版本号对应；`default` 作为兜底条目）。
- 新增 Flutter feature 时：
  1. 在 `lib/features/<feature>/` 下创建页面、controller 或状态对象，跨 feature 共享模型放入 `lib/core/`，共享组件放入 `lib/widgets/`。
  2. 在 Flutter 的 `version_features.json` 与版本解析层增加 feature/capabilities。
  3. 重新运行 `export_project.py`，检查 `exports/<NAME>/upper_ui/<NAME>_upper_ui_flutter/versions/<versionX>/` 是否只包含预期 Flutter 版本。
- 若项目含微信小程序，新增小程序 feature 时还需同步 `src/features/<feature>.js`、`src/features/index.js` 与 uni-app 侧 `version_features.json`，并验证 `<NAME>_upper_ui/versions/<versionX>/`。

## 参考应用

### ZNCZ_001（智能插座）

**规格**：[projects/ZNCZ_001/PRODUCT_SPEC.md](../projects/ZNCZ_001/PRODUCT_SPEC.md)

```
key 驱动 → EXTI 中断 → key_register_callback → APP_EVENT_KEY → 手动/定时/WiFi 界面
rtc_loop  → DS1302 → APP_EVENT_TICK → 定时比较
comm_loop → esp8266_mqtt_poll → APP_EVENT_COMM_RX → JSON 命令
logic_loop→ app_logic_loop → **app_fsm_dispatch**（手动/定时/WiFi 子界面）
```

UI 导航由 `app_logic.c` 内 `app_ui_transitions[]` 驱动；RTC/继电器/JSON 等业务数据仍在 `app_context_t`。按键事件由中断驱动的 `drivers/misc/key.c` 统一消抖/判定单击、双击、长按，再通过 `key_register_callback()` 通知应用，应用侧仅需在回调里转发到 `app_logic_on_key()` 并设置 `APP_EVENT_KEY`。

### RTJK_001（健康监测）

**规格**：[projects/RTJK_001/PRODUCT_SPEC.md](../projects/RTJK_001/PRODUCT_SPEC.md)

- 默认 **V10**：心率 + 血压 + 体温 + WiFi MQTT + 声光报警
- `APP_VERSION` 1–10：`version_config.h` 生成 `VERSION_FEATURE_*` + `DRIVER_CATALOG_RTJK_001` + CMake preamble（V8+ ADC）
- `sched_loop`：logic / sensor / key / comm（按版本启用）

## 维护与完成度

| 文档 | 用途 |
| --- | --- |
| [maintenance_guide.md](./maintenance_guide.md) | 驱动/BSP/多项目/export 约定、§11 限制 |
| [upper_ui_guide.md](./upper_ui_guide.md) | 上位机 feature、version_features.json、vite 常量与代码规范 |
| [project_status.md](./project_status.md) | 完成度、构建指标、缺口 |

---

*最后更新：与 RTJK_001、sched_loop、export_project、硬件 SPI/ADC HAL 同步。*
