ZHJS_001 — 智慧照明（光照 + 红外×3 + 灯×3 + 自动/手动）

构建（在 CLion 或命令行，需 STM32 GNU 工具链）：
  cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../../cmake/stm32-gcc-toolchain.cmake
  cmake --build build

产物：build/ZHJS_001.elf、ZHJS_001.bin

规格说明：PRODUCT_SPEC.md
引脚与极性：board/board_config.h
