cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(KQZL3 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

mvp_resolve_app_version(DEFAULT 14 MIN 1 MAX 14)

set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC" FORCE)

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_KQZL3
    ENABLE_APP_FRAMEWORK
)
