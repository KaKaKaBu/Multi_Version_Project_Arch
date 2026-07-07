cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(KQZL2 C ASM)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

set(MVP_PROJECT_APP_VERSION 14)
set(MVP_APP_VERSION "" CACHE STRING "KQZL2 application version override (empty uses project default 14)")
set_property(CACHE MVP_APP_VERSION PROPERTY STRINGS "" 1 2 3 4 5 6 7 8 9 10 11 12 13 14)
mvp_resolve_app_version(DEFAULT ${MVP_PROJECT_APP_VERSION} MIN 1 MAX 14)

set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC" FORCE)

if(MVP_RESOLVED_APP_VERSION EQUAL 8 OR MVP_RESOLVED_APP_VERSION EQUAL 14)
    list(APPEND MVP_EXTRA_DEFINES ESP8266_MQTT_ENABLE_HUAWEI=1)
else()
    list(APPEND MVP_EXTRA_DEFINES ESP8266_MQTT_ENABLE_HUAWEI=0)
endif()

mvp_add_stm32f1_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_KQZL2
    ENABLE_APP_FRAMEWORK
)

