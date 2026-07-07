# Multi Version Project Architecture

This repository is a multi-version embedded project workspace for STM32/51 MCU firmware, reusable drivers, export tooling, and Flutter upper-computer applications.

The project is organized around reusable platform layers and per-product version folders. Each project can declare its own hardware configuration, firmware logic, version features, product specification, and upper UI implementation while sharing common drivers, HAL wrappers, scheduler utilities, communication protocols, and Flutter UI infrastructure.

## Repository Layout

```text
.
├── bsp/                 MCU platform support, HAL implementations, startup files, and board templates
├── cmake/               CMake toolchains, project templates, and driver catalog resolution
├── common/              Shared C utilities, driver core, scheduler helpers, and shared Flutter package
├── docs/                Architecture, maintenance, protocol, and upper UI documentation
├── drivers/             Reusable device drivers for sensors, actuators, communication, display, voice, and misc modules
├── hal_wrapper/         Cross-platform HAL-facing headers
├── projects/            Product projects and version-specific firmware/UI assets
└── tools/               Project generation, export, GUI, and automation scripts
```

## Project Structure

Each product under `projects/` generally follows this layout:

```text
projects/<PROJECT_ID>/
├── app/                 Application logic, callbacks, main entry, and version configuration
├── board/               Board pin mapping and device registry
├── <PROJECT>_upper_ui_flutter/
│   ├── lib/             Flutter upper UI pages, services, models, and project config
│   ├── android/         Android host project and permissions
│   └── windows/         Windows host project when enabled
├── PRODUCT_SPEC.md      Product requirements, version matrix, and pin/function notes
└── readme.txt           Legacy project notes
```

Some legacy projects may still contain older upper UI implementations. New or migrated upper UI work should prefer the shared Flutter common layer.

## Shared Flutter Layer

The shared Flutter package lives in:

```text
common/flutter/
```

It provides:

- Common transport interfaces and active transport switching
- MQTT transport
- Huawei IoTDA MQTT transport
- Bluetooth SPP transport and compatible plugin integration
- Common home shell, connection panel, dashboard/control page primitives, adaptive layout, MJPEG view, and shared theme
- Version capability helpers driven by `--dart-define`

Project Flutter apps should keep only project-specific configuration, telemetry models, services, and view composition in their own folder. Communication primitives, connection UI, adaptive layout, and theme should come from `mvp_flutter_common`.

## Firmware Layers

Firmware code is split into:

- `bsp/`: MCU/platform-specific GPIO, USART, timers, I2C, EXTI, startup, and vendor libraries
- `drivers/`: reusable hardware drivers such as ESP8266 MQTT/WiFi, TTS UART, SU03T voice, keys, buzzer, sensors, display, and actuators
- `common/`: scheduler, driver feature flags, memory/util helpers, HMAC/SHA utilities, and shared framework code
- `projects/<PROJECT>/app`: project business logic and version behavior
- `projects/<PROJECT>/board`: concrete pin/device binding for the selected board

Board pin definitions and actual firmware behavior should stay synchronized with each project's `PRODUCT_SPEC.md`.

## Version Features

Firmware and upper UI versions are controlled through project configuration and compile/runtime feature definitions.

Flutter upper UI versions commonly use:

```powershell
flutter run --dart-define=UPPER_VERSION=<version> --dart-define=UPPER_FEATURES=<features>
```

Feature names such as `common`, `wifi`, `ble`, `cloud`, `camera`, `voice`, and project-specific sensor/control flags are used to conditionally show UI pages and enable communication paths.

## Communication

The repository currently supports several communication modes:

- Local MQTT broker communication
- Huawei IoTDA MQTT communication
- Bluetooth SPP transparent serial communication
- ESP8266 AT-based MQTT/WiFi firmware communication
- MJPEG camera stream display, with camera metadata reported by ESP32-CAM where applicable

For Flutter apps, the preferred implementation path is:

```text
CommunicationController
  -> ActiveTransportService
  -> MqttTransport / HuaweiIotdaTransport / BluetoothSppTransport
  -> project service parser and command serializer
```

## Export And Build Tooling

The `tools/` directory contains GUI and CLI helpers for project generation and export. Exported firmware builds depend on the configured compiler toolchain and CMake availability.

Typical requirements:

- CMake available in `PATH`
- ARM GCC toolchain for STM32 builds
- SDCC toolchain for MCS51 builds where applicable
- Flutter SDK for upper UI builds
- Dart/Flutter local package cache configured when using offline or mirrored environments

## Development Notes

- Keep reusable drivers and Flutter primitives in common layers instead of duplicating them inside project folders.
- Keep project folders responsible for project-specific logic, configuration, telemetry mapping, and version views.
- Preserve existing pin mappings unless a task explicitly requests hardware migration.
- When changing firmware protocol fields, update the corresponding Flutter service/model and product specification.
- Before export, run the relevant firmware build and `dart analyze lib` for changed Flutter projects.
- Do not commit generated build folders, local caches, or IDE runtime state.

## Recent Architecture Direction

The current migration direction is:

- Consolidate Flutter communication, connection panels, themes, and adaptive layout into `common/flutter`
- Keep each upper UI project focused on version-specific views and MQTT/cloud/BLE configuration
- Use conditional capabilities to control which UI pages and connection modes are visible per version
- Align Huawei cloud product-model parameters with project requirements before platform configuration
- Keep camera stream reporting separated from STM32 telemetry when ESP32-CAM owns camera data

