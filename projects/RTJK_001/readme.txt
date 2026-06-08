RTJK_001 — 人体健康监测仪

默认版本：V10（心率+血压+体温+WiFi+声光报警）

切换版本：
  1. CMake 变量 RTJK_VERSION=1..10（默认 10）
  2. CLion：Settings → CMake → CMake options 添加 -DRTJK_VERSION=10
     注意：RTJK_VERSION 是整数 CACHE 变量，不要用 ON/OFF
  3. 或修改 app/version_config.h 中的 RTJK_VERSION（需与 CMake 一致）

版本导出（独立目录，驱动列表自动解析 driver_catalog.cmake）：
  python ../../tools/export_project.py --project RTJK_001 --version 10 -o ../../exports
  python ../../tools/export_project.py --project RTJK_001 --batch --versions 1-10 -o ../../exports --clean

构建（需 CMake + STM32CubeCLT 工具链）：
  cmake -S . -B build -G "Ninja"
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/stm32-gcc-toolchain.cmake
    -DCMAKE_MAKE_PROGRAM=<Ninja路径>/ninja.exe
    -DRTJK_VERSION=10
  cmake --build build

WiFi/MQTT 配置：修改 app/board_config.h 中 BOARD_ESP8266_* 宏。
功能规格与 JSON 协议：见 PRODUCT_SPEC.md
