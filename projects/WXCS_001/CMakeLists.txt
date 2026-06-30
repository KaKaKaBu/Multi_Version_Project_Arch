cmake_minimum_required(VERSION 3.20)

set(TARGET_PLATFORM "stm32f1" CACHE STRING "Target platform: stm32f1 or mcs51_sdcc")
set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS stm32f1 mcs51_sdcc)

if(TARGET_PLATFORM STREQUAL "mcs51_sdcc")
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/platforms/mcs51-sdcc-toolchain.cmake)
    project(WXCS_001 C)
elseif(TARGET_PLATFORM STREQUAL "stm32f1")
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)
    project(WXCS_001 C ASM)
else()
    message(FATAL_ERROR "TARGET_PLATFORM must be stm32f1 or mcs51_sdcc, got '${TARGET_PLATFORM}'")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

if(TARGET_PLATFORM STREQUAL "mcs51_sdcc")
    mvp_resolve_app_version(DEFAULT 1 MIN 1 MAX 1 FORCE_DEFAULT)
    mvp_add_mcs51_polling_project(${PROJECT_NAME}
        DRIVER_CATALOG_VAR DRIVER_CATALOG_WXCS_001_MCS51
    )
else()
    mvp_resolve_app_version(DEFAULT 3 MIN 1 MAX 3)
    set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC for MQ135 and MQ-7" FORCE)
    mvp_add_stm32f1_project(${PROJECT_NAME}
        DRIVER_CATALOG_VAR DRIVER_CATALOG_WXCS_001
    )
endif()
