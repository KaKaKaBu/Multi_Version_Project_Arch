#!/usr/bin/env python3
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional


@dataclass
class ProjectCreateOptions:
    name: str
    version_count: int = 1
    upper_ui_types: List[str] = field(default_factory=lambda: ["flutter"])
    app_package_name: Optional[str] = None
    app_title: Optional[str] = None
    log: Optional[Callable[[str], None]] = None


def command(*args):
    if os.name == "nt":
        return ["cmd", "/c", *args]
    return list(args)


def run_cmd(cmd, cwd, env=None, log: Optional[Callable[[str], None]] = None):
    logger = log or print
    logger(f"[gen_project] Running: {' '.join(cmd)} (cwd={cwd})")
    process = subprocess.Popen(
        cmd,
        cwd=cwd,
        env=env,
        shell=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    assert process.stdout is not None
    for line in process.stdout:
        logger(line.rstrip())
    returncode = process.wait()
    if returncode != 0:
        raise subprocess.CalledProcessError(returncode, cmd)


def render_template(text: str, **values: str) -> str:
    for key, value in values.items():
        text = text.replace(f"__{key.upper()}__", value)
    return text


def dart_package_name(name: str) -> str:
    value = re.sub(r"[^a-zA-Z0-9_]", "_", name).lower()
    if not re.match(r"^[a-z_]", value):
        value = f"app_{value}"
    return value


def dart_class_name(name: str) -> str:
    parts = re.split(r"[^a-zA-Z0-9]+", name)
    result = "".join(part[:1].upper() + part[1:].lower() for part in parts if part)
    return result or "Generated"


def default_app_package_name(name: str) -> str:
    package_tail = re.sub(r"[^a-zA-Z0-9_]", "", name).lower() or "app"
    if not re.match(r"^[a-zA-Z]", package_tail):
        package_tail = f"app{package_tail}"
    return f"com.jbltech.{package_tail}"


def default_app_title(name: str) -> str:
    return f"{name} 上位机"


def derive_flutter_org(app_package_name: str) -> str:
    parts = app_package_name.split(".")
    if len(parts) <= 1:
        return app_package_name
    return ".".join(parts[:-1])


def replace_json_fields(path: Path, replacements: Dict[str, str]) -> None:
    if not path.exists():
        return
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return
    for key, value in replacements.items():
        data[key] = value
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def patch_flutter_android_identity(ui_dir: Path, app_package_name: str, app_title: str) -> None:
    gradle_patterns = {
        r'namespace\s*=\s*"[^"]+"': f'namespace = "{app_package_name}"',
        r'applicationId\s*=\s*"[^"]+"': f'applicationId = "{app_package_name}"',
        r'namespace\s+"[^"]+"': f'namespace "{app_package_name}"',
        r'applicationId\s+"[^"]+"': f'applicationId "{app_package_name}"',
    }
    for gradle_path in [ui_dir / "android" / "app" / "build.gradle.kts", ui_dir / "android" / "app" / "build.gradle"]:
        if not gradle_path.exists():
            continue
        text = gradle_path.read_text(encoding="utf-8")
        for pattern, replacement in gradle_patterns.items():
            text = re.sub(pattern, replacement, text)
        gradle_path.write_text(text, encoding="utf-8")

    manifest_path = ui_dir / "android" / "app" / "src" / "main" / "AndroidManifest.xml"
    if manifest_path.exists():
        text = manifest_path.read_text(encoding="utf-8")
        text = re.sub(r'android:label="[^"]+"', f'android:label="{app_title}"', text)
        manifest_path.write_text(text, encoding="utf-8")

    package_path = Path(*app_package_name.split("."))
    kotlin_root = ui_dir / "android" / "app" / "src" / "main" / "kotlin"
    main_activity_candidates = list(kotlin_root.glob("**/MainActivity.kt")) if kotlin_root.exists() else []
    if main_activity_candidates:
        old_activity = main_activity_candidates[0]
        text = old_activity.read_text(encoding="utf-8")
        text = re.sub(r"^package\s+[\w.]+", f"package {app_package_name}", text, flags=re.MULTILINE)
        new_dir = kotlin_root / package_path
        new_dir.mkdir(parents=True, exist_ok=True)
        new_activity = new_dir / "MainActivity.kt"
        new_activity.write_text(text, encoding="utf-8")
        if old_activity != new_activity:
            old_activity.unlink()
            for parent in old_activity.parent.parents:
                if parent == kotlin_root:
                    break
                try:
                    parent.rmdir()
                except OSError:
                    break

    replace_json_fields(ui_dir / "web" / "manifest.json", {"name": app_title, "short_name": app_title})
    index_path = ui_dir / "web" / "index.html"
    if index_path.exists():
        text = index_path.read_text(encoding="utf-8")
        text = re.sub(r'<meta name="apple-mobile-web-app-title" content="[^"]*">', f'<meta name="apple-mobile-web-app-title" content="{app_title}">', text)
        text = re.sub(r"<title>.*?</title>", f"<title>{app_title}</title>", text, flags=re.DOTALL)
        index_path.write_text(text, encoding="utf-8")


def patch_uniapp_identity(upper_dir: Path, app_package_name: str, app_title: str) -> None:
    replace_json_fields(upper_dir / "capacitor.config.json", {"appId": app_package_name, "appName": app_title})
    manifest_path = upper_dir / "src" / "manifest.json"
    if manifest_path.exists():
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
        data["name"] = app_title
        data["description"] = f"{app_title}"
        data.setdefault("mp-weixin", {})
        manifest_path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    pages_path = upper_dir / "src" / "pages.json"
    if pages_path.exists():
        data = json.loads(pages_path.read_text(encoding="utf-8"))
        for page in data.get("pages", []):
            page.setdefault("style", {})["navigationBarTitleText"] = app_title
        data.setdefault("globalStyle", {})["navigationBarTitleText"] = app_title
        pages_path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

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


def scaffold_upper_ui_features(upper_dir: Path, version_count: int) -> None:
    src_dir = upper_dir / "src"
    features_dir = src_dir / "features"
    features_dir.mkdir(parents=True, exist_ok=True)

    (src_dir / "main.js").write_text(UPPER_MAIN_JS, encoding="utf-8")
    (upper_dir / "vite.config.js").write_text(UPPER_VITE_CONFIG, encoding="utf-8")
    (features_dir / "index.js").write_text(UPPER_FEATURE_INDEX_JS, encoding="utf-8")
    (features_dir / "common.js").write_text(UPPER_FEATURE_COMMON_JS, encoding="utf-8")
    (features_dir / "wifi.js").write_text(UPPER_FEATURE_WIFI_JS, encoding="utf-8")
    (features_dir / "ble.js").write_text(UPPER_FEATURE_BLE_JS, encoding="utf-8")
    (upper_dir / "version_features.json").write_text(
        build_upper_version_features_json(
            version_count,
            always_include=UNIAPP_ALWAYS_INCLUDE,
            feature_globs=UNIAPP_FEATURE_GLOBS,
        ),
        encoding="utf-8",
    )

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
    __UPPER_UI_DEBUG__: JSON.stringify(process.env.NODE_ENV !== 'production'),
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

def build_upper_version_features_json(version_count: int, always_include: Optional[List[str]] = None, feature_globs: Optional[Dict[str, List[str]]] = None) -> str:
    versions = {"default": {"features": ["common"]}}
    for version in range(1, version_count + 1):
        versions[str(version)] = {"features": ["common"]}

    data = {
        "_comment": "Edit feature lists per firmware version. defaultFeatures applies when a version entry is missing.",
        "defaultFeatures": ["common"],
        "versions": versions,
    }
    if always_include is not None:
        data["alwaysInclude"] = always_include
    if feature_globs is not None:
        data["featureGlobs"] = feature_globs
    return json.dumps(data, ensure_ascii=False, indent=2) + "\n"


UNIAPP_ALWAYS_INCLUDE = [
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
    "src/static/**",
]

UNIAPP_FEATURE_GLOBS = {
    "common": [
        "src/features/common/**",
        "src/features/index.js",
        "src/pages/**",
    ],
    "wifi": ["src/features/wifi/**"],
    "ble": ["src/features/ble/**"],
}

UPPER_VERSION_FEATURES_JSON = build_upper_version_features_json(
    1,
    always_include=UNIAPP_ALWAYS_INCLUDE,
    feature_globs=UNIAPP_FEATURE_GLOBS,
)

VERSION_CONFIG = '''#ifndef VERSION_CONFIG_H
#define VERSION_CONFIG_H

#include "scheduler.h"

#ifndef APP_VERSION
#define APP_VERSION 1
#endif

#define VERSION_NAME "{name}"

/* Output all product capabilities through VERSION_FEATURE_* macros. */
#define VERSION_FEATURE_DISPLAY 1
#define VERSION_FEATURE_COMM    0

/* Enable concrete drivers here, then driver_features.h defaults the rest to 0. */
#include "driver_features.h"

/* Application events (extend per project needs) */
#define APP_EVENT_TICK          SCHED_EVENT_TICK
#define APP_EVENT_SENSOR_READY  0x01
#define APP_EVENT_COMM_RX       0x02
#define APP_EVENT_UI_REFRESH    0x04

#endif
'''

BOARD_DEVICES_C = '''#include "board_config.h"
#include "driver_core.h"
#include "driver_configs.h"

#if defined(BOARD_OLED_I2C)
static const i2c_device_config_t board_oled_config = {
    BOARD_OLED_I2C,
    BOARD_OLED_I2C_SPEED,
    board_oled_i2c_scl,
    board_oled_i2c_sda,
    BOARD_OLED_I2C_REMAP,
    BOARD_OLED_I2C_ADDR
};
REGISTER_BOARD_DEVICE(DISPLAY, "oled", &board_oled_config);
#endif

#if defined(KEY_DRIVER_PIN_TABLE)
static const gpio_input_driver_config_t board_key_config = {
    0,
    KEY_DRIVER_PIN_TABLE,
    KEY_DRIVER_BUTTON_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);
#elif defined(BOARD_KEY_COUNT)
static const hal_pin_t board_key_pins[] = {
#if BOARD_KEY_COUNT >= 1U
    board_key1_pin,
#endif
#if BOARD_KEY_COUNT >= 2U
    board_key2_pin,
#endif
#if BOARD_KEY_COUNT >= 3U
    board_key3_pin,
#endif
#if BOARD_KEY_COUNT >= 4U
    board_key4_pin,
#endif
#if BOARD_KEY_COUNT >= 5U
    board_key5_pin,
#endif
};
static const gpio_input_driver_config_t board_key_config = {
    board_key_pins,
    0,
    BOARD_KEY_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);
#endif

#if defined(BOARD_ESP8266_USART)
static const esp8266_driver_config_t board_esp8266_config = {
    {
        BOARD_ESP8266_USART,
        BOARD_ESP8266_BAUDRATE,
        board_esp8266_tx,
        board_esp8266_rx,
        BOARD_ESP8266_USART_REMAP,
        BOARD_ESP8266_USART_TX_MODE,
#if HAL_USART_ENABLE_DMA && defined(BOARD_ESP8266_USART_TX_DMA)
        BOARD_ESP8266_USART_TX_DMA,
#else
        0U,
#endif
#if defined(BOARD_ESP8266_USART_ID)
        BOARD_ESP8266_USART_ID
#else
        0U
#endif
    },
    board_esp8266_ch_pd_pin,
    board_esp8266_rst_pin,
#if defined(BOARD_ESP8266_DEBUG_TRACE_ENABLE)
    BOARD_ESP8266_DEBUG_TRACE_ENABLE
#else
    0U
#endif
};
REGISTER_BOARD_DEVICE(COMM, "esp8266", &board_esp8266_config);
#endif

#if defined(BOARD_JDY31_USART)
static const usart_device_config_t board_jdy31_config = {
    BOARD_JDY31_USART,
    BOARD_JDY31_BAUDRATE,
    board_jdy31_tx,
    board_jdy31_rx,
    BOARD_JDY31_USART_REMAP,
    BOARD_JDY31_USART_TX_MODE,
#if HAL_USART_ENABLE_DMA && defined(BOARD_JDY31_USART_TX_DMA)
    BOARD_JDY31_USART_TX_DMA,
#else
    0U,
#endif
    0U
};
REGISTER_BOARD_DEVICE(COMM, "jdy31", &board_jdy31_config);
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
2. 更新 `board/board_config.h` 引脚、通讯外设、WiFi/MQTT 参数。
3. 在 `app/app_logic.c` 中实现实际的传感器采集、UI 状态机与通讯处理。

## 构建方式

确保 `arm-none-eabi-gcc` 在 PATH 中，或设置 `STM32_TOOLCHAIN_BIN` 环境变量指向工具链 bin 目录。

```bash
# 使用 Ninja（推荐）
cmake -S . -B build -G Ninja
cmake --build build

# 或使用 Unix Makefiles
cmake -S . -B build
cmake --build build
```
'''

FLUTTER_PUBSPEC = '''name: __PACKAGE_NAME__
description: __APP_TITLE__ built with Flutter.
publish_to: 'none'
version: 1.0.0+1

environment:
  sdk: ^3.10.0

dependencies:
  flutter:
    sdk: flutter

dev_dependencies:
  flutter_test:
    sdk: flutter
  flutter_lints: ^6.0.0

flutter:
  uses-material-design: true
'''

FLUTTER_ANALYSIS_OPTIONS = '''include: package:flutter_lints/flutter.yaml

linter:
  rules:
    avoid_print: true
'''

FLUTTER_MAIN_DART = '''import 'package:flutter/material.dart';

import 'app.dart';

void main() {
  runApp(const __CLASS_NAME__App());
}
'''

FLUTTER_APP_DART = '''import 'package:flutter/material.dart';

import 'core/version/version_capabilities.dart';

class __CLASS_NAME__App extends StatelessWidget {
  const __CLASS_NAME__App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: '__APP_TITLE__',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xff0f8f70)),
        useMaterial3: true,
      ),
      home: const _HomePage(),
    );
  }
}

class _HomePage extends StatelessWidget {
  const _HomePage();

  @override
  Widget build(BuildContext context) {
    final capabilities = resolveUpperFeatures();
    final features = capabilities.features.isEmpty
        ? 'common'
        : capabilities.features.join(', ');

    return Scaffold(
      appBar: AppBar(title: const Text('__APP_TITLE__')),
      body: Center(
        child: Card(
          child: Padding(
            padding: const EdgeInsets.all(24),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'APP_VERSION: ${capabilities.version}',
                  style: Theme.of(context).textTheme.titleMedium,
                ),
                const SizedBox(height: 12),
                Text('Features: $features'),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
'''

FLUTTER_DEBUG_FLAGS_DART = '''import 'package:flutter/foundation.dart';

const bool upperUiShowCommunicationLog = kDebugMode;
'''

FLUTTER_VERSION_CAPABILITIES_DART = '''const upperVersionDefine = String.fromEnvironment('UPPER_VERSION', defaultValue: '1');
const upperFeaturesDefine = String.fromEnvironment('UPPER_FEATURES', defaultValue: 'common');

class VersionCapabilities {
  const VersionCapabilities({required this.version, required this.features});

  final int version;
  final List<String> features;

  bool has(String feature) => features.contains(feature);
}

VersionCapabilities resolveUpperFeatures({
  String versionDefine = upperVersionDefine,
  String featureDefine = upperFeaturesDefine,
}) {
  final version = int.tryParse(versionDefine) ?? 1;
  final features = featureDefine
      .split(',')
      .map((item) => item.trim())
      .where((item) => item.isNotEmpty)
      .toList(growable: false);
  return VersionCapabilities(
    version: version,
    features: features.isEmpty ? const ['common'] : features,
  );
}
'''

FLUTTER_VERSION_FEATURES_JSON = '''{
  "_comment": "Flutter upper UI features by firmware version. Add feature names and versions as the product grows.",
  "defaultFeatures": ["common"],
  "versions": {
    "default": {
      "features": ["common"]
    }
  }
}
'''

FLUTTER_WIDGET_TEST = '''import 'package:flutter_test/flutter_test.dart';

import 'package:__PACKAGE_NAME__/app.dart';

void main() {
  testWidgets('__NAME__ app renders', (tester) async {
    await tester.pumpWidget(const __CLASS_NAME__App());

    expect(find.text('__APP_TITLE__'), findsWidgets);
    expect(find.textContaining('APP_VERSION'), findsOneWidget);
  });
}
'''


def create_flutter_upper_ui(
    project_dir: Path,
    name: str,
    app_package_name: str,
    app_title: str,
    version_count: int,
    log: Optional[Callable[[str], None]] = None,
) -> None:
    flutter_dir = project_dir / f"{name}_upper_ui_flutter"
    if flutter_dir.exists():
        return

    package_name = dart_package_name(name)
    class_name = dart_class_name(name)
    run_cmd(
        command(
            "flutter",
            "create",
            "--project-name",
            package_name,
            "--org",
            derive_flutter_org(app_package_name),
            "--platforms=android,web",
            flutter_dir.name,
        ),
        cwd=project_dir,
        log=log,
    )

    (flutter_dir / "pubspec.yaml").write_text(
        render_template(FLUTTER_PUBSPEC, name=name, package_name=package_name, app_title=app_title),
        encoding="utf-8",
    )
    (flutter_dir / "analysis_options.yaml").write_text(FLUTTER_ANALYSIS_OPTIONS, encoding="utf-8")

    lib_dir = flutter_dir / "lib"
    config_dir = lib_dir / "core" / "config"
    version_dir = lib_dir / "core" / "version"
    config_dir.mkdir(parents=True, exist_ok=True)
    version_dir.mkdir(parents=True, exist_ok=True)
    (lib_dir / "main.dart").write_text(
        render_template(FLUTTER_MAIN_DART, class_name=class_name),
        encoding="utf-8",
    )
    (lib_dir / "app.dart").write_text(
        render_template(FLUTTER_APP_DART, name=name, class_name=class_name, app_title=app_title),
        encoding="utf-8",
    )
    (config_dir / "debug_flags.dart").write_text(
        FLUTTER_DEBUG_FLAGS_DART,
        encoding="utf-8",
    )
    (version_dir / "version_capabilities.dart").write_text(
        FLUTTER_VERSION_CAPABILITIES_DART,
        encoding="utf-8",
    )
    (flutter_dir / "version_features.json").write_text(
        build_upper_version_features_json(version_count),
        encoding="utf-8",
    )

    test_dir = flutter_dir / "test"
    test_dir.mkdir(exist_ok=True)
    (test_dir / "widget_test.dart").write_text(
        render_template(
            FLUTTER_WIDGET_TEST,
            name=name,
            package_name=package_name,
            class_name=class_name,
            app_title=app_title,
        ),
        encoding="utf-8",
    )
    patch_flutter_android_identity(flutter_dir, app_package_name, app_title)
    run_cmd(command("flutter", "pub", "get"), cwd=flutter_dir, log=log)

CMAKELISTS = '''cmake_minimum_required(VERSION 3.20)

include(${{CMAKE_CURRENT_LIST_DIR}}/../../cmake/stm32-gcc-toolchain.cmake)

project({name} C ASM)

include(${{CMAKE_CURRENT_LIST_DIR}}/../../cmake/project_template.cmake)

set(MVP_PROJECT_APP_VERSION 1)
set(MVP_APP_VERSION "" CACHE STRING "{name} application version override (empty uses project default 1)")
set_property(CACHE MVP_APP_VERSION PROPERTY STRINGS "" __VERSION_STRINGS__)
mvp_resolve_app_version(DEFAULT ${{MVP_PROJECT_APP_VERSION}} MIN 1 MAX __VERSION_COUNT__)

set(DRIVER_CATALOG_{name}
    # TODO: add driver source list or reference a shared catalog entry.
)

mvp_add_stm32f1_project(${{PROJECT_NAME}}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_{name}
    ENABLE_APP_FRAMEWORK
)
'''

def create_uniapp_upper_ui(
    root: Path,
    project_dir: Path,
    name: str,
    app_package_name: str,
    app_title: str,
    version_count: int,
    log: Optional[Callable[[str], None]] = None,
) -> None:
    upper_dir = project_dir / f"{name}_upper_ui"
    if upper_dir.exists():
        return

    npm_env = os.environ.copy()
    npm_env.setdefault("npm_config_cache", str(root.parent / "npm-cache"))
    run_cmd(
        command("npx", "--yes", "degit", "dcloudio/uni-preset-vue#vite", upper_dir.name),
        cwd=project_dir,
        env=npm_env,
        log=log,
    )
    run_cmd(command("npm", "install"), cwd=upper_dir, env=npm_env, log=log)
    run_cmd(
        command("npm", "install", "@capacitor/core", "@capacitor/cli", "@capacitor/android"),
        cwd=upper_dir,
        env=npm_env,
        log=log,
    )
    scaffold_upper_ui_features(upper_dir, version_count)
    patch_uniapp_identity(upper_dir, app_package_name, app_title)
    run_cmd(command("npm", "run", "build:h5"), cwd=upper_dir, env=npm_env, log=log)
    run_cmd(command("npm", "run", "build:mp-weixin"), cwd=upper_dir, env=npm_env, log=log)

    run_cmd(command("npx", "--yes", "cap", "init", app_title, app_package_name), cwd=upper_dir, env=npm_env, log=log)
    config_path = upper_dir / "capacitor.config.json"
    if config_path.exists():
        data = config_path.read_text(encoding="utf-8")
        data = data.replace('"webDir": "dist"', '"webDir": "dist/build/h5"')
        config_path.write_text(data, encoding="utf-8")
    patch_uniapp_identity(upper_dir, app_package_name, app_title)
    run_cmd(command("npx", "--yes", "cap", "add", "android"), cwd=upper_dir, env=npm_env, log=log)


def normalize_project_options(options: ProjectCreateOptions) -> ProjectCreateOptions:
    version_count = max(1, int(options.version_count))
    upper_ui_types = list(options.upper_ui_types)
    return ProjectCreateOptions(
        name=options.name,
        version_count=version_count,
        upper_ui_types=upper_ui_types,
        app_package_name=options.app_package_name or default_app_package_name(options.name),
        app_title=options.app_title or default_app_title(options.name),
        log=options.log,
    )


def create_project_with_options(root: Path, options: ProjectCreateOptions) -> None:
    options = normalize_project_options(options)
    name = options.name
    logger = options.log or print
    project_dir = root / "projects" / name
    app_dir = project_dir / "app"
    board_dir = project_dir / "board"
    board_template = (root / "bsp" / "board" / "board_config_template.h").read_text(encoding="utf-8")
    app_dir.mkdir(parents=True, exist_ok=False)
    board_dir.mkdir(parents=True, exist_ok=False)
    logger(f"[gen_project] Creating firmware project: {project_dir}")
    (app_dir / "app_main.c").write_text(APP_MAIN, encoding="utf-8")
    (app_dir / "version_config.h").write_text(VERSION_CONFIG.format(name=name), encoding="utf-8")
    (app_dir / "app_logic.h").write_text(APP_LOGIC_H, encoding="utf-8")
    (app_dir / "app_logic.c").write_text(APP_LOGIC_C.replace("__PROJECT_NAME__", name), encoding="utf-8")
    (app_dir / "app_callbacks.c").write_text(CALLBACKS, encoding="utf-8")
    (app_dir / "stm32f10x_conf.h").write_text(STM32F10X_CONF, encoding="utf-8")
    (board_dir / "board_config.h").write_text(
        board_template.replace("BOARD_CONFIG_TEMPLATE_H", "BOARD_CONFIG_H"),
        encoding="utf-8",
    )
    (board_dir / "board_devices.c").write_text(BOARD_DEVICES_C, encoding="utf-8")
    (project_dir / "readme.txt").write_text(README.format(name=name), encoding="utf-8")
    version_strings = " ".join(str(item) for item in range(1, options.version_count + 1))
    (project_dir / "CMakeLists.txt").write_text(
        render_template(
            CMAKELISTS.format(name=name),
            version_count=str(options.version_count),
            version_strings=version_strings,
        ),
        encoding="utf-8",
    )

    if "flutter" in options.upper_ui_types:
        create_flutter_upper_ui(
            project_dir,
            name,
            options.app_package_name or default_app_package_name(name),
            options.app_title or default_app_title(name),
            options.version_count,
            log=options.log,
        )
    if "mini_program" in options.upper_ui_types:
        create_uniapp_upper_ui(
            root,
            project_dir,
            name,
            options.app_package_name or default_app_package_name(name),
            options.app_title or default_app_title(name),
            options.version_count,
            log=options.log,
        )


def create_project(root: Path, name: str, with_mini_program: bool = False) -> None:
    upper_ui_types = ["flutter"]
    if with_mini_program:
        upper_ui_types.append("mini_program")
    create_project_with_options(
        root,
        ProjectCreateOptions(
            name=name,
            upper_ui_types=upper_ui_types,
            app_package_name=default_app_package_name(name),
            app_title=default_app_title(name),
        ),
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a project skeleton under project_template/projects")
    parser.add_argument("name", help="project version name, e.g. RTJK_001")
    parser.add_argument(
        "--with-mini-program",
        action="store_true",
        help="also generate a uni-app stack for WeChat mini-program builds; Flutter is always generated",
    )
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    args = parser.parse_args()
    create_project(args.root, args.name, args.with_mini_program)


if __name__ == "__main__":
    main()
