#!/usr/bin/env python3
from pathlib import Path
import argparse

APP_MAIN = '''#include "devmgr.h"
#include "hal_common.h"
#include "irq_event.h"
#include "scheduler.h"
#include "version_config.h"

void app_main(void)
{
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();
    sched_start();
}

int main(void)
{
    app_main();
    for (;;) {
    }
}
'''

VERSION_CONFIG = '''#ifndef VERSION_CONFIG_H
#define VERSION_CONFIG_H

#include "scheduler.h"

#define VERSION_NAME "{name}"
#define APP_EVENT_TICK SCHED_EVENT_TICK

#endif
'''

CALLBACKS = '''#include "version_config.h"

void app_callbacks_init(void)
{
}
'''

STM32F10X_CONF = '''#ifndef STM32F10X_CONF_H
#define STM32F10X_CONF_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_adc.h"
#include "misc.h"

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif

#endif
'''

README = '''{name} 项目骨架

在 CMakeLists.txt 的 DRIVER_SRCS 中选择本版本需要参与编译的驱动文件。

构建方式：
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=D:/ProjectJBL/PerfabProj/project_template/cmake/stm32-gcc-toolchain.cmake -DCMAKE_MAKE_PROGRAM=D:/ProgramFile/ST/STM32CubeCLT_1.20.0/Ninja/bin/ninja.exe
cmake --build build
'''

CMAKELISTS = '''cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project({name} C ASM)

set(TEMPLATE_ROOT ${{CMAKE_CURRENT_LIST_DIR}}/../..)
include(${{TEMPLATE_ROOT}}/cmake/hal_options.cmake)
include(${{TEMPLATE_ROOT}}/cmake/driver_catalog.cmake)

set(STM32_TOOLCHAIN_BIN "D:/ProgramFile/ST/STM32CubeCLT_1.20.0/GNU-tools-for-STM32/bin" CACHE PATH "STM32 GNU toolchain bin directory")
set(LINKER_SCRIPT ${{TEMPLATE_ROOT}}/bsp/linker_scripts/STM32F103XX_FLASH.ld)
set(STARTUP_FILE ${{TEMPLATE_ROOT}}/bsp/startup/startup_stm32f103xb.s)

if(NOT CMAKE_OBJCOPY)
    set(CMAKE_OBJCOPY ${{STM32_TOOLCHAIN_BIN}}/arm-none-eabi-objcopy.exe)
endif()

if(NOT CMAKE_SIZE)
    set(CMAKE_SIZE ${{STM32_TOOLCHAIN_BIN}}/arm-none-eabi-size.exe)
endif()

set(CPU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

set(COMMON_SRCS
    ${{TEMPLATE_ROOT}}/common/device_manager/devmgr.c
    ${{TEMPLATE_ROOT}}/common/scheduler/scheduler.c
    ${{TEMPLATE_ROOT}}/common/app_framework/app_fsm.c
    ${{TEMPLATE_ROOT}}/common/irq_event/irq_event.c
)

set(BSP_SRCS
    ${{TEMPLATE_ROOT}}/bsp/hal_wrapper/hal_common.c
    ${{TEMPLATE_ROOT}}/bsp/hal_wrapper/gpio_hal.c
    ${{BSP_HAL_I2C_SRC}}
    ${{TEMPLATE_ROOT}}/bsp/hal_wrapper/usart_hal.c
    ${{BSP_HAL_USART_DMA_SRC}}
    ${{BSP_HAL_SPI_SRC}}
    ${{BSP_HAL_SPI_DMA_SRC}}
    ${{BSP_HAL_ADC_SRC}}
    ${{BSP_HAL_ADC_DMA_SRC}}
    ${{TEMPLATE_ROOT}}/bsp/hal_wrapper/timer_hal.c
    ${{TEMPLATE_ROOT}}/bsp/irq_handlers/irq_handlers.c
)

set(DRIVER_SRCS
)

set(SPL_SRCS
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/CMSIS/system_stm32f10x.c
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/misc.c
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_gpio.c
    ${{SPL_HAL_I2C_SRC}}
    ${{SPL_HAL_SPI_SRC}}
    ${{SPL_HAL_ADC_SRC}}
    ${{SPL_HAL_DMA_SRC}}
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_rcc.c
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_tim.c
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_usart.c
)

set(APP_SRCS
    app/app_main.c
    app/app_callbacks.c
)

add_executable(${{PROJECT_NAME}}.elf
    ${{APP_SRCS}}
    ${{COMMON_SRCS}}
    ${{BSP_SRCS}}
    ${{DRIVER_SRCS}}
    ${{SPL_SRCS}}
    ${{STARTUP_FILE}}
)

set_target_properties(${{PROJECT_NAME}}.elf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    SUFFIX ""
)

target_include_directories(${{PROJECT_NAME}}.elf PRIVATE
    app
    ${{TEMPLATE_ROOT}}/common/interfaces
    ${{TEMPLATE_ROOT}}/common/driver_core
    ${{TEMPLATE_ROOT}}/common/device_manager
    ${{TEMPLATE_ROOT}}/common/scheduler
    ${{TEMPLATE_ROOT}}/common/app_framework
    ${{TEMPLATE_ROOT}}/common/irq_event
    ${{TEMPLATE_ROOT}}/bsp/board
    ${{TEMPLATE_ROOT}}/bsp/hal_wrapper
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/CMSIS
    ${{TEMPLATE_ROOT}}/bsp/stm32f10x_std_lib/Libraries/FWlib/inc
)

target_compile_definitions(${{PROJECT_NAME}}.elf PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

hal_apply_compile_definitions(${{PROJECT_NAME}}.elf)

target_compile_options(${{PROJECT_NAME}}.elf PRIVATE
    ${{CPU_FLAGS}}
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Os>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:ASM>:-x>
    $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>
)

target_link_options(${{PROJECT_NAME}}.elf PRIVATE
    ${{CPU_FLAGS}}
    -T${{LINKER_SCRIPT}}
    -Wl,--gc-sections
    -Wl,-Map=${{PROJECT_NAME}}.map
    -Wl,--print-memory-usage
)

add_custom_command(TARGET ${{PROJECT_NAME}}.elf POST_BUILD
    COMMAND ${{CMAKE_OBJCOPY}} -O binary $<TARGET_FILE:${{PROJECT_NAME}}.elf> ${{PROJECT_NAME}}.bin
    COMMAND ${{CMAKE_SIZE}} $<TARGET_FILE:${{PROJECT_NAME}}.elf>
    BYPRODUCTS ${{PROJECT_NAME}}.bin ${{PROJECT_NAME}}.map
)
'''

def create_project(root: Path, name: str) -> None:
    project_dir = root / "projects" / name
    app_dir = project_dir / "app"
    board_template = (root / "bsp" / "board" / "board_config_template.h").read_text(encoding="utf-8")
    app_dir.mkdir(parents=True, exist_ok=False)
    (app_dir / "app_main.c").write_text(APP_MAIN, encoding="utf-8")
    (app_dir / "version_config.h").write_text(VERSION_CONFIG.format(name=name), encoding="utf-8")
    (app_dir / "app_callbacks.c").write_text(CALLBACKS, encoding="utf-8")
    (app_dir / "stm32f10x_conf.h").write_text(STM32F10X_CONF, encoding="utf-8")
    (app_dir / "board_config.h").write_text(
        board_template.replace("BOARD_CONFIG_TEMPLATE_H", "BOARD_CONFIG_H"),
        encoding="utf-8",
    )
    (project_dir / "readme.txt").write_text(README.format(name=name), encoding="utf-8")
    (project_dir / "CMakeLists.txt").write_text(CMAKELISTS.format(name=name), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a project skeleton under project_template/projects")
    parser.add_argument("name", help="project version name, e.g. RTJK_001")
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    args = parser.parse_args()
    create_project(args.root, args.name)


if __name__ == "__main__":
    main()
