cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(ZNLYJ_001 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

mvp_resolve_app_version(DEFAULT 4 MIN 1 MAX 4)

if(APP_VERSION GREATER_EQUAL 2)
    set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC for GL5506" FORCE)
endif()

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_ZNLYJ_001
)
