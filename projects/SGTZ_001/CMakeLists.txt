cmake_minimum_required(VERSION 3.20)

set(TARGET_PLATFORM "mcs51_sdcc" CACHE STRING "Target platform: mcs51_sdcc")
set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS mcs51_sdcc)

if(TARGET_PLATFORM STREQUAL "mcs51_sdcc")
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/platforms/mcs51-sdcc-toolchain.cmake)
    project(SGTZ_001 C)
else()
    message(FATAL_ERROR "SGTZ_001 is currently a 51 MCU project; TARGET_PLATFORM must be mcs51_sdcc")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_template.cmake)

set(MVP_PROJECT_APP_VERSION 7)
set(MVP_APP_VERSION "" CACHE STRING "SGTZ_001 application version override (empty uses project default 7)")
set_property(CACHE MVP_APP_VERSION PROPERTY STRINGS "" 1 2 3 4 5 6 7)
mvp_resolve_app_version(DEFAULT ${MVP_PROJECT_APP_VERSION} MIN 1 MAX 7)

set(BOARD_ESP8266_MQTT_BACKEND "0" CACHE STRING "ESP8266 MQTT backend: 0=normal MQTT, 1=Huawei Cloud IoTDA")
set_property(CACHE BOARD_ESP8266_MQTT_BACKEND PROPERTY STRINGS 0 1)
if(NOT "${BOARD_ESP8266_MQTT_BACKEND}" MATCHES "^[01]$")
    message(FATAL_ERROR "BOARD_ESP8266_MQTT_BACKEND must be 0 or 1")
endif()
add_compile_definitions(BOARD_ESP8266_MQTT_BACKEND=${BOARD_ESP8266_MQTT_BACKEND})

set(SGTZ_MCS51_USART_OPTION "")
if(APP_VERSION EQUAL 5 OR APP_VERSION EQUAL 6 OR APP_VERSION EQUAL 7)
    set(SGTZ_MCS51_USART_OPTION ENABLE_USART)
endif()

mvp_add_mcs51_polling_project(${PROJECT_NAME}
    DRIVER_CATALOG_VAR DRIVER_CATALOG_SGTZ_001_MCS51
    ${SGTZ_MCS51_USART_OPTION}
)
