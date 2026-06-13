# BSP 目录说明

`project_template/bsp/` 提供所有与 MCU 相关的公共资产：启动文件、链接脚本、板级封装 HAL、IRQ 转发以及 STM32 标准外设库。应用工程不直接修改这些文件，而是通过 `projects/<NAME>/app/board_config.h`、`cmake/hal_options.cmake` 等入口选择功能。本指南帮助维护者在新增硬件、启用 HAL 模块或迁移 MCU 时快速定位应修改的文件。

## 1. 目录结构与职责

| 子目录/文件 | 作用 | 典型修改场景 |
| --- | --- | --- |
| `board/board_config_template.h` | 板级引脚、USART/I2C/SPI/ADC/调试串口默认模板。工程在生成时会复制到 `projects/<NAME>/app/board_config.h` 并做定制。 | 新项目建模、补充新的外设宏或示例配置。 |
| `hal_wrapper/` | 对 SPL 寄存器与 GPIO 进行统一封装，暴露 `*_hal_*` API。包含 GPIO/EXTI/I2C/SPI/USART/TIMER/ADC/软件调试串口等模块，以及 `hal_common.c` 的 `bsp_init()`。 | 1) 新增 HAL 模块（如 DMA RX） 2) 修复底层时序问题 3) 扩展 `hal_features.h`。 |
| `irq_handlers/irq_handlers.c` | 所有中断入口统一定义于此，并调用对应 HAL handler，再由 `irq_event` 唤醒任务。驱动层禁止自定义 NVIC ISR。 | 新增外设 IRQ、将 HAL handler 映射到 `irq_event_post_from_isr()`。 |
| `linker_scripts/STM32F103XX_FLASH.ld` | 链接脚本，定义 Flash/RAM 布局、`.driver_list` 段 (`KEEP(*(.driver_list))`)、向 `devmgr` 暴露 `__driver_list_start/end`。 | 迁移至其它容量的 F103/F105 或添加额外段（如 EEPROM 仿真）。 |
| `startup/startup_stm32f103xb.s` | 启动向量表、弱符号、`Reset_Handler` 调用 `SystemInit` → `main`。 | 更换 MCU 或调整堆栈/数据段初始化逻辑。 |
| `syscalls/syscalls.c` | 实现 `_write/_sbrk/_close` 等最小化 syscalls，`printf` 通过 soft UART/USART 输出。 | 调整调试输出、替换内存分配钩子。 |
| `stm32f10x_std_lib/` | STM32F10x SPL + CMSIS 来源，供硬件 I2C/SPI/USART/ADC 等 HAL 使用。 | 升级 SPL 版本或迁移到其它芯片族时整体替换。 |

> `cmake/hal_options.cmake` 会根据 CMake 选项决定 `hal_wrapper` 中哪些 `.c` 文件参与编译。例如 `HAL_I2C_USE_SOFT=ON` 时选择 `i2c_hal_soft.c`，否则链接 `i2c_hal_hw.c` + 对应 SPL 源码。

## 2. HAL 特性开关与配置流

1. **CMake 开关**：`cmake/hal_options.cmake` 定义 `HAL_*` 选项并根据组合 `list(APPEND BSP_HAL_*_SRC ...)`。项目 `CMakeLists.txt` 必须在包含该文件之前处理与版本相关的前置条件（例如 `RTJK_VERSION>=8` 时 `set(HAL_ADC_ENABLE ON FORCE)`）。
2. **编译期宏**：`hal_wrapper/hal_features.h` 读取上述选项，提供默认值并允许 `board_config.h` 覆盖（如对特定项目启用 `HAL_DEBUG_UART_ENABLE`）。
3. **板级引脚**：驱动层只读 `board_config.h` 的 `hal_pin_t` 与宏。若启用软件 I2C，`board_config` 必须切换至 `GPIO_HAL_MODE_OUT_OD`，硬件 I2C 则使用 `GPIO_HAL_MODE_AF_OD`。
4. **IRQ 链路**：HAL 模块在初始化时调用 `irq_event_bind()`、`irq_event_post_from_isr()`，真正的中断函数位于 `irq_handlers.c`，保持项目之间的一致性。

## 3. 常见维护任务速查

1. **新增 HAL 模块**（例如硬件 RNG）
   - 在 `hal_wrapper/` 添加 `rng_hal.c/.h`，提供 `hal_status_t` 返回值与 `hal_common` 依赖。
   - 更新 `cmake/hal_options.cmake` 以便按需编译，并在 `hal_features.h` 增加默认开关。
   - 若模块需要 IRQ，扩展 `irq_handlers.c` 并在 `syscalls` 或 `hal_common` 中处理底层初始化。

2. **更换 MCU / Flash 布局**
   - 替换 `startup/`、`linker_scripts/`、`stm32f10x_std_lib/` 为新器件版本。
   - 检查 `hal_wrapper/*` 中的寄存器定义是否与新芯片一致。
   - 保持 `.driver_list` 段定义与符号名称不变，以免破坏 `devmgr` 自注册机制。

3. **调试串口或日志**
   - 启用 `HAL_DEBUG_UART_ENABLE`（CMake 或 `board_config`），并在模板中提供 `board_debug_uart_tx`。
   - `_write()`（`syscalls.c`）会路由到 soft UART，确保 `bsp_init()` 在 `app_main()` 顶部调用。

4. **引入 DMA/定时中断**
   - HAL 层内通过 `*_hal_config_t` 配置 DMA 通道或定时器参数；IRQ 源在 `irq_handlers.c` 中统一注册。
   - 应用若需要通过事件唤醒，利用 `irq_event_bind(source, APP_EVENT_*)`。

## 4. 贡献约定

- 不在 `common/` 或 `drivers/` 中直接 `#include "board_config.h"`；所有板级差异应留在 `projects/<NAME>/app/`。
- HAL 封装应返回 `hal_status_t` 并避免忙等待；长延迟放到应用任务或专用 `sched_loop`。
- 任何修改 `linker_scripts/`、`startup/`、`hal_wrapper/` 核心逻辑的提交，都应在 `docs/maintenance_guide.md` 与相关项目 README 中同步记录，以便 `export_project.py` 重新验证。

如需更深入的架构约定或维护流程，请参考 `docs/architecture.md` 与 `docs/maintenance_guide.md`。
