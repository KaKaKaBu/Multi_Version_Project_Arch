WXCS_001 尾气检测项目

## 项目说明

WXCS_001 是 STM32F103C8T6 尾气/气体检测项目，固件通过 `APP_VERSION` 选择 1-3 版本功能：

- v1：MQ135 空气质量、MQ-7 CO、DS18B20 温度、OLED、按键、风扇继电器、蜂鸣器/LED 报警。
- v2：v1 + ESP8266 WiFi/MQTT 远程遥测与控制。
- v3：v1 + JDY-31 蓝牙远程遥测与控制。

驱动清单在 `cmake/driver_catalog.cmake` 的 `DRIVER_CATALOG_WXCS_001` 中维护，版本能力宏在 `app/version_config.h` 中维护。

## 目录

- `app/`：固件应用逻辑、板级引脚、版本能力配置。
- `WXCS_001_upper_ui_flutter/`：Flutter 上位机，按 `version_features.json` 做版本能力裁剪。
- `PRODUCT_SPEC.md`：版本矩阵、协议和交互说明。

## 构建

确保 `arm-none-eabi-gcc` 在 PATH 中，或设置 `STM32_TOOLCHAIN_BIN` 指向工具链 bin 目录。

```bash
cmake -S projects/WXCS_001 -B build/WXCS_001_v3 -G Ninja -DAPP_VERSION=3
cmake --build build/WXCS_001_v3
```

Flutter 上位机：

```bash
cd projects/WXCS_001/WXCS_001_upper_ui_flutter
flutter test
flutter run -d chrome --dart-define=UPPER_VERSION=3 --dart-define=UPPER_FEATURES=common,ble,control
```
