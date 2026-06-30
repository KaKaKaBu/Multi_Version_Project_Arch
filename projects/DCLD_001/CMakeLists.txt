cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(DCLD_001 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

mvp_resolve_app_version(DEFAULT 7 MIN 1 MAX 7)

set(HAL_DEBUG_UART_ENABLE ON CACHE BOOL "Enable GPIO bit-bang debug UART TX on PC13")

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_DCLD_001
)
