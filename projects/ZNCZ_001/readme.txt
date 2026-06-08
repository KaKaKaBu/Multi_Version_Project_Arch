ZNCZ-001 智能插座（版本一）

产品：继电器 + 定时 + 手动/自动模式 + WiFi（ESP-01S）+ DS1302 + OLED + 5 键
MCU：STM32F103C8T6

功能规格、JSON 协议、引脚表：见 PRODUCT_SPEC.md

已实现概要：
- 手动/定时双模式，5 键 UI（app_fsm 驱动 5 个界面状态）
- sched_loop 四任务：logic / comm / key / rtc
- DS1302 实时时钟 + 定时自动控继电器
- ESP8266 MQTT JSON：校时、设开/关时间、遥控继电器、状态上报

驱动（driver_catalog）：oled / relay / ds1302 / key / esp8266_wifi / esp8266_mqtt / led

模板内构建：
  cmake -S . -B build -G Ninja
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/stm32-gcc-toolchain.cmake
    -DCMAKE_MAKE_PROGRAM=<Ninja路径>/ninja.exe
  cmake --build build

导出独立工程（交付/归档）：
  python ../../tools/export_project.py --project ZNCZ_001 -o ../../exports
    --extras readme.txt,PRODUCT_SPEC.md

WiFi/MQTT：修改 app/board_config.h 中 BOARD_ESP8266_* 宏。
调试串口：默认 HAL_DEBUG_UART_ENABLE=ON，PC13 9600 TX。
