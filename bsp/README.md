# BSP 目录说明

`bsp/` 只放平台相关实现和板级模板。平台无关 HAL 接口头文件在仓库根目录 `hal_wrapper/`，驱动和应用通过这些接口访问 GPIO/I2C/USART/SPI/TIMER/ADC，不直接依赖芯片头文件。

## 目录结构

| 子目录/文件 | 作用 | 典型修改场景 |
| --- | --- | --- |
| `board/board_config_template.h` | 新项目板级配置模板。`tools/gen_project.py` 会复制为 `projects/<NAME>/board/board_config.h`。 | 增加新的默认外设宏或示例引脚。 |
| `stm32/hal/` | STM32F1 HAL 实现，内部允许调用 CMSIS/SPL。 | 修复 GPIO/I2C/SPI/USART/ADC/TIMER 实现或增加 STM32 HAL 模块。 |
| `stm32/irq_handlers/` | STM32 统一 IRQ 入口，转发到 HAL handler，再由 `irq_event` 唤醒应用任务。 | 新增外设 IRQ 或调整中断事件桥接。 |
| `stm32/linker_scripts/` | STM32 链接脚本，保留 `.driver_list` 与 `.board_device_list` 段。 | 更换 Flash/RAM 容量或增加链接段。 |
| `stm32/startup/` | STM32 启动文件。 | 更换 MCU 或调整启动流程。 |
| `stm32/syscalls/` | STM32 newlib 最小 syscalls，`_write()` 可接入调试输出。 | 调整 printf 路由或 libc 钩子。 |
| `stm32/vendor/` | STM32F10x CMSIS + SPL 源码。 | 升级 SPL 或迁移芯片族。 |
| `mcs51/hal/` | C51 HAL 实现。保持简单本地轮询能力，不承诺 scheduler/MQTT/BLE。 | 增加 C51 可承受的 GPIO/I2C/显示等底层能力。 |

## 配置流

1. 工程 `CMakeLists.txt` 通过 `cmake/project_template.cmake` 选择平台和驱动 catalog。
2. `cmake/hal_options.cmake` 根据 `HAL_*` 选项选择 `bsp/<platform>/hal/` 中的实现源文件。
3. `projects/<NAME>/board/board_config.h` 定义引脚、总线实例、波特率、地址等板级参数。
4. `projects/<NAME>/board/board_devices.c` 用 `REGISTER_BOARD_DEVICE()` 显式注册板上设备实例。
5. 驱动在 `drivers/` 中用 `REGISTER_DRIVER()` 注册驱动实现，`devmgr_init_all()` 匹配设备实例和驱动后调用 `init(config)`。

## 维护约定

- `drivers/` 和 `common/` 不得 `#include "board_config.h"`，也不得包含 STM32/C51 芯片头。
- 芯片库调用只允许出现在 `bsp/<platform>/hal/`、平台启动文件、平台 IRQ 和 vendor 代码中。
- 新增 HAL 接口时，先在 `hal_wrapper/` 定义平台无关 API，再在目标平台的 `bsp/<platform>/hal/` 实现。
- 新增板上设备时，优先修改项目自己的 `board/board_config.h` 和 `board/board_devices.c`，不要改驱动源码。
- 修改链接脚本时必须保留 `.driver_list` 和 `.board_device_list`，否则驱动实现和板级设备实例无法自动发现。

更多规则见 `docs/architecture.md` 和 `docs/maintenance_guide.md`。
