ZNLYJ_001 智能晾衣架项目

## 项目说明

ZNLYJ_001 是 STM32F103C8T6 智能晾衣架项目，固件通过 `APP_VERSION` 选择 1-4 版本功能：

- v1：DHT11 温湿度、E18 人体检测、红外衣物检测、步进电机、OLED、按键、蜂鸣器报警。
- v2：v1 + GL5506 光照检测 + HX711 重量检测。
- v3：v2 + JDY-31 蓝牙远程控制。
- v4：v2 + ESP8266 WiFi/MQTT 远程控制。

驱动清单在 `cmake/driver_catalog.cmake` 的 `DRIVER_CATALOG_ZNLYJ_001` 中维护，版本能力宏在 `app/version_config.h` 中维护。

## 目录

- `app/`：固件应用逻辑、板级引脚、版本能力配置。
- `ZNLYJ_001_upper_ui_flutter/`：Flutter 上位机，按 `version_features.json` 做版本能力裁剪。
- `PRODUCT_SPEC.md`：版本矩阵、协议和交互说明。

## 构建

确保 `arm-none-eabi-gcc` 在 PATH 中，或设置 `STM32_TOOLCHAIN_BIN` 指向工具链 bin 目录。

```bash
cmake -S projects/ZNLYJ_001 -B build/ZNLYJ_001_v4 -G Ninja -DAPP_VERSION=4
cmake --build build/ZNLYJ_001_v4
```

Flutter 上位机：

```bash
cd projects/ZNLYJ_001/ZNLYJ_001_upper_ui_flutter
flutter test
flutter run -d chrome --dart-define=UPPER_VERSION=4 --dart-define=UPPER_FEATURES=common,light,weight,wifi,control
```
