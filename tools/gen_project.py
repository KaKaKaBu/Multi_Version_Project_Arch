#!/usr/bin/env python3
from pathlib import Path
import argparse
import json
import os
import subprocess
import sys


def run_cmd(cmd, cwd, env=None):
    print(f"[gen_project] Running: {' '.join(cmd)} (cwd={cwd})")
    subprocess.run(cmd, cwd=cwd, check=True, env=env, shell=False)

APP_MAIN = '''#include "devmgr.h"
#include "hal_common.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"

static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor", sensor_loop_run, 2, 1000,
    SCHED_EVENT_TICK, APP_EVENT_SENSOR_READY);

static sched_loop_t app_loop = SCHED_LOOP_DEF(
    "logic", app_logic_loop_run, 1, 500,
    SCHED_EVENT_TICK, 0);

static sched_loop_t comm_loop = SCHED_LOOP_DEF(
    "comm", comm_loop_run, 3, 0,
    APP_EVENT_COMM_RX, 0);

void app_main(void)
{
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();

    app_logic_init();

    sched_loop_init();
    sched_loop_register(&sensor_loop);
    sched_loop_register(&app_loop);
    sched_loop_register(&comm_loop);

    sched_start();
}

int main(void)
{
    app_main();
    for (;;) {
    }
}
'''


def scaffold_upper_ui_features(upper_dir: Path) -> None:
    src_dir = upper_dir / "src"
    features_dir = src_dir / "features"
    features_dir.mkdir(parents=True, exist_ok=True)

    (src_dir / "main.js").write_text(UPPER_MAIN_JS, encoding="utf-8")
    (upper_dir / "vite.config.js").write_text(UPPER_VITE_CONFIG, encoding="utf-8")
    (features_dir / "index.js").write_text(UPPER_FEATURE_INDEX_JS, encoding="utf-8")
    (features_dir / "common.js").write_text(UPPER_FEATURE_COMMON_JS, encoding="utf-8")
    (features_dir / "wifi.js").write_text(UPPER_FEATURE_WIFI_JS, encoding="utf-8")
    (features_dir / "ble.js").write_text(UPPER_FEATURE_BLE_JS, encoding="utf-8")
    (upper_dir / "version_features.json").write_text(UPPER_VERSION_FEATURES_JSON, encoding="utf-8")

UPPER_MAIN_JS = '''import { createSSRApp } from "vue";
import App from "./App.vue";
import { setupFeatures } from "./features";

export function createApp() {
    const app = createSSRApp(App);
    setupFeatures(app);
    return {
        app,
    };
}
'''

UPPER_VITE_CONFIG = '''import { defineConfig } from 'vite'
import uni from '@dcloudio/vite-plugin-uni'

function resolveFeatureContext() {
  const list = (process.env.UPPER_FEATURES || '')
    .split(',')
    .map((item) => item.trim())
    .filter(Boolean)
  const flags = {}
  for (const key of list) {
    flags[key] = true
  }
  return { list, flags }
}

const ctx = resolveFeatureContext()

export default defineConfig({
  plugins: [
    uni(),
  ],
  define: {
    __UPPER_VERSION__: JSON.stringify(process.env.UPPER_VERSION || ''),
    __UPPER_FEATURES__: JSON.stringify(ctx.list),
    __UPPER_FEATURE_FLAGS__: JSON.stringify(ctx.flags),
  },
})
'''

UPPER_FEATURE_INDEX_JS = '''import { setupCommonFeature } from "./common";
import { setupWifiFeature } from "./wifi";
import { setupBleFeature } from "./ble";

const registry = {
    common: setupCommonFeature,
    wifi: setupWifiFeature,
    ble: setupBleFeature,
};

function resolveFlags() {
    if (typeof __UPPER_FEATURE_FLAGS__ !== "undefined") {
        return __UPPER_FEATURE_FLAGS__;
    }
    return {};
}

export function setupFeatures(app) {
    const flags = resolveFlags();
    Object.entries(registry).forEach(([key, installer]) => {
        if (flags[key]) {
            installer(app);
        }
    });
}
'''

UPPER_FEATURE_COMMON_JS = '''export function setupCommonFeature(app) {
    if (process.env.NODE_ENV !== "production") {
        console.info("[upper-ui] common feature enabled");
    }
}
'''

UPPER_FEATURE_WIFI_JS = '''export function setupWifiFeature(app) {
    if (process.env.NODE_ENV !== "production") {
        console.info("[upper-ui] wifi feature enabled");
    }
}
'''

UPPER_FEATURE_BLE_JS = '''export function setupBleFeature(app) {
    if (process.env.NODE_ENV !== "production") {
        console.info("[upper-ui] ble feature enabled");
    }
}
'''

UPPER_VERSION_FEATURES_JSON = '''{
  "_comment": "Edit feature lists per firmware version. defaultFeatures applies when a version entry is missing.",
  "defaultFeatures": ["common"],
  "alwaysInclude": [
    "package.json",
    "package-lock.json",
    "vite.config.js",
    "uni.scss",
    "index.html",
    "capacitor.config.json",
    "version_features.json",
    "src/main.js",
    "src/App.vue",
    "src/manifest.json",
    "src/pages.json",
    "src/static/**"
  ],
  "featureGlobs": {
    "common": [
      "src/features/common/**",
      "src/features/index.js",
      "src/pages/**"
    ],
    "wifi": [
      "src/features/wifi/**"
    ],
    "ble": [
      "src/features/ble/**"
    ]
  },
  "versions": {
    "default": {
      "features": ["common"]
    },
    "3": {
      "features": ["common", "wifi"]
    }
  }
}
'''

VERSION_CONFIG = '''#ifndef VERSION_CONFIG_H
#define VERSION_CONFIG_H

#include "scheduler.h"

#define VERSION_NAME "{name}"

/* Application events (extend per project needs) */
#define APP_EVENT_TICK          SCHED_EVENT_TICK
#define APP_EVENT_SENSOR_READY  0x01
#define APP_EVENT_COMM_RX       0x02
#define APP_EVENT_UI_REFRESH    0x04

#endif
'''

CALLBACKS = '''#include "version_config.h"
#include "scheduler.h"
#include "debug_log.h"

#define APP_RX_BUFFER_SIZE 256U

static unsigned char app_last_rx_buffer[APP_RX_BUFFER_SIZE];
static unsigned short app_last_rx_len;

void app_comm_rx_callback(const unsigned char *data, unsigned short len)
{
    unsigned short i;
    unsigned short copy_len = len;

    if (copy_len > APP_RX_BUFFER_SIZE) {
        copy_len = APP_RX_BUFFER_SIZE;
    }

    for (i = 0; i < copy_len; ++i) {
        app_last_rx_buffer[i] = data[i];
    }

    app_last_rx_buffer[copy_len] = '\0';
    app_last_rx_len = copy_len;

    DEBUG_LOG_APP_CB("[APP_CB] rx len=%u\r\n", (unsigned int)copy_len);
    event_set(APP_EVENT_COMM_RX);
}

const unsigned char *app_get_last_rx(unsigned short *len)
{
    if (len != 0) {
        *len = app_last_rx_len;
    }

    return app_last_rx_buffer;
}

void app_clear_last_rx(void)
{
    app_last_rx_len = 0U;
}

void app_callbacks_init(void)
{
    /* Bind comm callbacks in app_logic_init if needed */
}
'''

APP_LOGIC_H = '''#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include "scheduler.h"
#include "version_config.h"

typedef struct {
    uint8_t sensor_updated;
    uint8_t ui_dirty;
    /* TODO: extend with real application state */
} app_context_t;

extern app_context_t g_app;

void app_logic_init(void);
void sensor_loop_run(sched_event_t events, void *ctx);
void app_logic_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);

#endif
'''

APP_LOGIC_C = '''#include "app_logic.h"
#include "devmgr.h"
#include "display_if.h"
#include "comm_if.h"
#include "tiny_printf.h"

#include <string.h>

app_context_t g_app;

static const display_driver_t *display_drv;

void app_logic_init(void)
{
    memset(&g_app, 0, sizeof(g_app));
    display_drv = devmgr_get_display("oled");
    if (display_drv) {
        display_drv->init();
        display_drv->clear();
    }
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    /* TODO: read sensors and update g_app */
    g_app.sensor_updated = 1;
}

void app_logic_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (!g_app.sensor_updated && !g_app.ui_dirty) {
        return;
    }

    g_app.sensor_updated = 0;
    g_app.ui_dirty = 0;

    if (display_drv) {
        display_drv->print(0, 0, 1, "__PROJECT_NAME__ ready");
    }
}

void comm_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    /* TODO: poll communication driver or process incoming bytes */
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

## 快速开始
1. 根据产品版本在 `cmake/driver_catalog.cmake` 中新增 `DRIVER_CATALOG_{name}` 并在本工程 `CMakeLists.txt` 的 `DRIVER_SRCS` 中引用。
2. 更新 `app/board_config.h` 引脚、通讯外设、WiFi/MQTT 参数。
3. 在 `app/app_logic.c` 中实现实际的传感器采集、UI 状态机与通讯处理。

## 构建方式
```bash
cmake -S . -B build -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE=D:/ProjectJBL/PerfabProj/project_template/cmake/stm32-gcc-toolchain.cmake \
  -DCMAKE_MAKE_PROGRAM=D:/ProgramFile/ST/STM32CubeCLT_1.20.0/Ninja/bin/ninja.exe
cmake --build build
```
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
    ${{TEMPLATE_ROOT}}/common/scheduler/sched_loop.c
    ${{TEMPLATE_ROOT}}/common/app_framework/app_fsm.c
    ${{TEMPLATE_ROOT}}/common/irq_event/irq_event.c
    ${{TEMPLATE_ROOT}}/common/utils/tiny_printf.c
    ${{TEMPLATE_ROOT}}/common/utils/cJSON.c
    ${{TEMPLATE_ROOT}}/common/utils/cjson_port.c
    ${{TEMPLATE_ROOT}}/common/utils/mem_pool.c
    ${{TEMPLATE_ROOT}}/common/utils/debug_uart.c
    ${{TEMPLATE_ROOT}}/common/utils/soft_uart.c
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
    # TODO: set to DRIVER_CATALOG_{name} after adding catalog entry
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
    app/app_logic.c
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
    ${{TEMPLATE_ROOT}}/common/utils
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

target_link_libraries(${{PROJECT_NAME}}.elf PRIVATE m)

target_link_options(${{PROJECT_NAME}}.elf PRIVATE
    ${{CPU_FLAGS}}
    -T${{LINKER_SCRIPT}}
    -Wl,--gc-sections
    -Wl,-Map=${{PROJECT_NAME}}.map
    -Wl,--print-memory-usage
    -Wl,-u,malloc
    -Wl,-u,_malloc_r
    -Wl,-u,cjson_port_init
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
    (app_dir / "app_logic.h").write_text(APP_LOGIC_H, encoding="utf-8")
    (app_dir / "app_logic.c").write_text(APP_LOGIC_C.replace("__PROJECT_NAME__", name), encoding="utf-8")
    (app_dir / "app_callbacks.c").write_text(CALLBACKS, encoding="utf-8")
    (app_dir / "stm32f10x_conf.h").write_text(STM32F10X_CONF, encoding="utf-8")
    (app_dir / "board_config.h").write_text(
        board_template.replace("BOARD_CONFIG_TEMPLATE_H", "BOARD_CONFIG_H"),
        encoding="utf-8",
    )
    (project_dir / "readme.txt").write_text(README.format(name=name), encoding="utf-8")
    (project_dir / "CMakeLists.txt").write_text(CMAKELISTS.format(name=name), encoding="utf-8")

    upper_dir = project_dir / f"{name}_upper_ui"
    if not upper_dir.exists():
        npm_env = os.environ.copy()
        npm_env.setdefault("npm_config_cache", str(root.parent / "npm-cache"))
        run_cmd([
            "cmd", "/c", "npx", "--yes", "degit", "dcloudio/uni-preset-vue#vite", upper_dir.name
        ], cwd=project_dir, env=npm_env)
        subprocess.run(["cmd", "/c", "npm", "install"], cwd=upper_dir, check=True, env=npm_env)
        subprocess.run(["cmd", "/c", "npm", "install", "@capacitor/core", "@capacitor/cli", "@capacitor/android"], cwd=upper_dir, check=True, env=npm_env)
        scaffold_upper_ui_features(upper_dir)
        subprocess.run(["cmd", "/c", "npm", "run", "build:h5"], cwd=upper_dir, check=True, env=npm_env)
        subprocess.run(["cmd", "/c", "npm", "run", "build:mp-weixin"], cwd=upper_dir, check=True, env=npm_env)
        app_name = name.replace('_', ' ')
        app_id = f"com.jbltech.{name.lower()}"
        run_cmd(["cmd", "/c", "npx", "--yes", "cap", "init", app_name, app_id], cwd=upper_dir, env=npm_env)
        config_path = upper_dir / "capacitor.config.json"
        if config_path.exists():
            data = config_path.read_text(encoding="utf-8")
            data = data.replace('"webDir": "dist"', '"webDir": "dist/build/h5"')
            config_path.write_text(data, encoding="utf-8")
        run_cmd(["cmd", "/c", "npx", "--yes", "cap", "add", "android"], cwd=upper_dir, env=npm_env)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a project skeleton under project_template/projects")
    parser.add_argument("name", help="project version name, e.g. RTJK_001")
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    args = parser.parse_args()
    create_project(args.root, args.name)


if __name__ == "__main__":
    main()
