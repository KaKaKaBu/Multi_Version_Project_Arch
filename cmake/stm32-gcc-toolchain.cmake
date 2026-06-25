# STM32 ARM GCC cross-compilation toolchain
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake ...
#   or include() before project() in CMakeLists.txt
#
# Tool discovery (in order):
#   1. If STM32_TOOLCHAIN_BIN is set (cache or env), search that directory first
#   2. Fall back to PATH search
#   3. All tools are REQUIRED — configure fails early with a clear message

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# --- toolchain bin directory hint (optional) ---
set(STM32_TOOLCHAIN_BIN "" CACHE PATH "STM32 GNU toolchain bin directory (optional; also read from env)")

set(_stm32_search_hints "")
if(NOT "${STM32_TOOLCHAIN_BIN}" STREQUAL "")
    list(APPEND _stm32_search_hints "${STM32_TOOLCHAIN_BIN}")
endif()
if(DEFINED ENV{STM32_TOOLCHAIN_BIN} AND NOT "$ENV{STM32_TOOLCHAIN_BIN}" STREQUAL "")
    list(APPEND _stm32_search_hints "$ENV{STM32_TOOLCHAIN_BIN}")
endif()

# --- find tools ---
find_program(CMAKE_C_COMPILER arm-none-eabi-gcc
    PATHS ${_stm32_search_hints}
    REQUIRED
    DOC "ARM GCC C compiler"
)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc
    PATHS ${_stm32_search_hints}
    REQUIRED
    DOC "ARM GCC ASM compiler"
)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy
    PATHS ${_stm32_search_hints}
    REQUIRED
    DOC "ARM objcopy"
)
find_program(CMAKE_SIZE arm-none-eabi-size
    PATHS ${_stm32_search_hints}
    REQUIRED
    DOC "ARM size"
)

# --- cross-compilation root path ---
get_filename_component(_stm32_toolchain_root "${CMAKE_C_COMPILER}" DIRECTORY)
get_filename_component(_stm32_toolchain_root "${_stm32_toolchain_root}" DIRECTORY)
set(CMAKE_FIND_ROOT_PATH "${_stm32_toolchain_root}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

unset(_stm32_search_hints)
unset(_stm32_toolchain_root)
