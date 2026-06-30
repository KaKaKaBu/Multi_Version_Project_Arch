cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(RTJK_001 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

mvp_resolve_app_version(DEFAULT 10 MIN 1 MAX 10)

set(HAL_DEBUG_UART_ENABLE ON CACHE BOOL "Enable GPIO bit-bang debug UART TX on PC13")
if(APP_VERSION GREATER_EQUAL 8)
    set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC for MSP20 blood pressure" FORCE)
endif()

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_RTJK_001
)
