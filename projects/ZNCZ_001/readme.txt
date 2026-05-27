ZNCZ-001 智能插座（版本一）

产品：继电器 + 定时 + 手动/自动模式 + WiFi（ESP-01S）+ DS1302 + OLED + 5 键
MCU：STM32F103C8T6

功能规格与 APP 协议见：PRODUCT_SPEC.md

当前工程已实现：
- 手动/定时双模式 + 5 键 UI + WiFi 信息界面
- DS1302 实时时钟 + 定时开关继电器
- ESP8266 MQTT JSON 远程校时/控继电器/设定时

驱动：oled / relay / ds1302 / key / esp8266_wifi / esp8266_mqtt / led

构建方式：
1. 确认已安装 CMake 与 STM32CubeCLT 工具链。
2. 在本目录执行：
   cmake -S . -B build -G "Ninja"
     -DCMAKE_TOOLCHAIN_FILE=../../cmake/stm32-gcc-toolchain.cmake
     -DCMAKE_MAKE_PROGRAM=<Ninja 路径>/ninja.exe
3. cmake --build build

WiFi/MQTT 配置：修改 app/board_config.h 中 BOARD_ESP8266_* 宏。
