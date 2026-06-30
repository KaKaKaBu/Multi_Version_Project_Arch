cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(RTJK_001 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

set(MVP_PROJECT_APP_VERSION 10)
set(MVP_APP_VERSION "" CACHE STRING "RTJK_001 application version override (empty uses project default 10)")
set_property(CACHE MVP_APP_VERSION PROPERTY STRINGS "" 1 2 3 4 5 6 7 8 9 10)
mvp_resolve_app_version(DEFAULT ${MVP_PROJECT_APP_VERSION} MIN 1 MAX 10)

set(HAL_DEBUG_UART_ENABLE ON CACHE BOOL "Enable GPIO bit-bang debug UART TX on PC13")
if(APP_VERSION GREATER_EQUAL 8)
    set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC for MSP20 blood pressure" FORCE)
endif()

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_RTJK_001
)
