cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(ZHJS_001 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

set(MVP_PROJECT_APP_VERSION 1)
set(MVP_APP_VERSION "" CACHE STRING "ZHJS_001 application version override (empty uses project default 1)")
set_property(CACHE MVP_APP_VERSION PROPERTY STRINGS "" 1)
mvp_resolve_app_version(DEFAULT ${MVP_PROJECT_APP_VERSION} MIN 1 MAX 1)
set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC for GL5506" FORCE)

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_ZHJS_001
)
