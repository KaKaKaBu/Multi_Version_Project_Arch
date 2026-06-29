SMZL_001 睡眠监测项目

## 项目说明

SMZL_001 是 STM32F103C8T6 老年人睡眠监测项目，固件通过 `APP_VERSION` 选择 1-3 版本功能：

- v1：MPU6050 翻身检测、MAX30102 心率/血氧、DS18B20 体温、OLED、按键、蜂鸣器/LED 报警。
- v2：v1 + JDY-31 蓝牙远程监测。
- v3：v1 + ESP8266 WiFi/MQTT 远程监测。

驱动清单在 `cmake/driver_catalog.cmake` 的 `DRIVER_CATALOG_SMZL_001` 中维护，版本能力宏在 `app/version_config.h` 中维护。

## 目录

- `app/`：固件应用逻辑、板级引脚、版本能力配置。
- `SMZL_001_upper_ui_flutter/`：Flutter 上位机，按 `version_features.json` 做版本能力裁剪。
- `PRODUCT_SPEC.md`：版本矩阵、协议和交互说明。

## 构建

确保 `arm-none-eabi-gcc` 在 PATH 中，或设置 `STM32_TOOLCHAIN_BIN` 指向工具链 bin 目录。

```bash
cmake -S projects/SMZL_001 -B build/SMZL_001_v3 -G Ninja -DAPP_VERSION=3
cmake --build build/SMZL_001_v3
```

Flutter 上位机：

```bash
cd projects/SMZL_001/SMZL_001_upper_ui_flutter
flutter test
flutter run -d chrome --dart-define=UPPER_VERSION=3 --dart-define=UPPER_FEATURES=common,wifi,monitor
```
