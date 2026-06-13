# Catalog of all migrated device drivers under drivers/
# Include this file after setting TEMPLATE_ROOT, then use DRIVER_CATALOG_ALL or pick subsets.

function(driver_catalog_resolve_app_version out_var legacy_var default_version)
    set(_resolved_version "")
    if(DEFINED APP_VERSION AND NOT "${APP_VERSION}" STREQUAL "" AND NOT "${APP_VERSION}" STREQUAL "OFF" AND NOT "${APP_VERSION}" STREQUAL "ON")
        set(_resolved_version "${APP_VERSION}")
    elseif(DEFINED ${legacy_var})
        set(_legacy_version "${${legacy_var}}")
        if(NOT "${_legacy_version}" STREQUAL "" AND NOT "${_legacy_version}" STREQUAL "OFF" AND NOT "${_legacy_version}" STREQUAL "ON")
            set(_resolved_version "${_legacy_version}")
        endif()
    endif()
    if("${_resolved_version}" STREQUAL "")
        set(_resolved_version "${default_version}")
    endif()
    set(${out_var} "${_resolved_version}" PARENT_SCOPE)
endfunction()

set(DRIVER_CATALOG_SENSORS
    ${TEMPLATE_ROOT}/drivers/sensors/dht11.c
    ${TEMPLATE_ROOT}/drivers/sensors/ds18b20.c
    ${TEMPLATE_ROOT}/drivers/sensors/bmp180.c
    ${TEMPLATE_ROOT}/drivers/sensors/mpu6050.c
    ${TEMPLATE_ROOT}/drivers/sensors/max30102.c
    ${TEMPLATE_ROOT}/drivers/sensors/pm25.c
    ${TEMPLATE_ROOT}/drivers/sensors/mq2_smoke.c
    ${TEMPLATE_ROOT}/drivers/sensors/mq7_co.c
    ${TEMPLATE_ROOT}/drivers/sensors/ph_sensor.c
    ${TEMPLATE_ROOT}/drivers/sensors/co2_uart.c
    ${TEMPLATE_ROOT}/drivers/sensors/hcsr04.c
    ${TEMPLATE_ROOT}/drivers/sensors/hx711.c
    ${TEMPLATE_ROOT}/drivers/sensors/ds1302_rtc.c
    ${TEMPLATE_ROOT}/drivers/sensors/rfid_rc522.c
)

set(DRIVER_CATALOG_ACTUATORS
    ${TEMPLATE_ROOT}/drivers/actuators/sg90.c
    ${TEMPLATE_ROOT}/drivers/actuators/stepmotor.c
    ${TEMPLATE_ROOT}/drivers/actuators/relay.c
)

set(DRIVER_CATALOG_COMM
    ${TEMPLATE_ROOT}/drivers/comm/esp8266_wifi.c
    ${TEMPLATE_ROOT}/drivers/comm/esp8266_mqtt.c
    ${TEMPLATE_ROOT}/drivers/comm/jdy31_ble.c
    ${TEMPLATE_ROOT}/drivers/comm/su03t_voice.c
    ${TEMPLATE_ROOT}/drivers/comm/l76k_gnss.c
    ${TEMPLATE_ROOT}/drivers/comm/nrf24l01.c
)

set(DRIVER_CATALOG_DISPLAYS
    ${TEMPLATE_ROOT}/drivers/displays/oled_font.c
    ${TEMPLATE_ROOT}/drivers/displays/oled_ssd1306.c
    ${TEMPLATE_ROOT}/drivers/displays/hc595_seg.c
)

set(DRIVER_CATALOG_MISC
    ${TEMPLATE_ROOT}/drivers/misc/led.c
    ${TEMPLATE_ROOT}/drivers/misc/buzzer.c
    ${TEMPLATE_ROOT}/drivers/misc/key.c
)

set(DRIVER_CATALOG_ALL
    ${DRIVER_CATALOG_SENSORS}
    ${DRIVER_CATALOG_ACTUATORS}
    ${DRIVER_CATALOG_COMM}
    ${DRIVER_CATALOG_DISPLAYS}
    ${DRIVER_CATALOG_MISC}
)

# ZHJS_001 — 智慧照明：GL5506 + E18×3 + 照明×3 + OLED + 4键
set(DRIVER_CATALOG_ZHJS_001
    ${TEMPLATE_ROOT}/drivers/displays/oled_font.c
    ${TEMPLATE_ROOT}/drivers/displays/oled_ssd1306.c
    ${TEMPLATE_ROOT}/drivers/sensors/gl5506_light.c
    ${TEMPLATE_ROOT}/drivers/sensors/e18_presence.c
    ${TEMPLATE_ROOT}/drivers/actuators/light_channel.c
    ${TEMPLATE_ROOT}/drivers/misc/key_4ch.c
)

# ZNCZ_001 demo subset — 智能插座：OLED + 继电器 + DS1302 + 5键 + ESP8266 MQTT
set(DRIVER_CATALOG_ZNCZ_001
    ${TEMPLATE_ROOT}/drivers/displays/oled_font.c
    ${TEMPLATE_ROOT}/drivers/displays/oled_ssd1306.c
    ${TEMPLATE_ROOT}/drivers/actuators/relay.c
    ${TEMPLATE_ROOT}/drivers/sensors/ds1302_rtc.c
    ${TEMPLATE_ROOT}/drivers/misc/key.c
    ${TEMPLATE_ROOT}/drivers/comm/esp8266_wifi.c
    ${TEMPLATE_ROOT}/drivers/comm/esp8266_mqtt.c
    ${TEMPLATE_ROOT}/drivers/misc/led.c
)

# RTJK_001 — 人体健康监测仪（APP_VERSION 1-10 条件编译）
driver_catalog_resolve_app_version(DRIVER_CATALOG_RTJK_001_VERSION RTJK_VERSION 10)

set(DRIVER_CATALOG_RTJK_001
    ${TEMPLATE_ROOT}/drivers/displays/oled_font.c
    ${TEMPLATE_ROOT}/drivers/displays/oled_ssd1306.c
    ${TEMPLATE_ROOT}/drivers/sensors/max30102.c
    ${TEMPLATE_ROOT}/drivers/misc/key.c
    ${TEMPLATE_ROOT}/drivers/misc/buzzer.c
    ${TEMPLATE_ROOT}/drivers/misc/led.c
)

if(DRIVER_CATALOG_RTJK_001_VERSION GREATER_EQUAL 4)
    if(DRIVER_CATALOG_RTJK_001_VERSION LESS_EQUAL 7 OR DRIVER_CATALOG_RTJK_001_VERSION GREATER_EQUAL 9)
        list(APPEND DRIVER_CATALOG_RTJK_001 ${TEMPLATE_ROOT}/drivers/sensors/ds18b20.c)
    endif()
endif()

if(DRIVER_CATALOG_RTJK_001_VERSION EQUAL 2 OR DRIVER_CATALOG_RTJK_001_VERSION EQUAL 5)
    list(APPEND DRIVER_CATALOG_RTJK_001 ${TEMPLATE_ROOT}/drivers/comm/jdy31_ble.c)
endif()

if(DRIVER_CATALOG_RTJK_001_VERSION EQUAL 3 OR DRIVER_CATALOG_RTJK_001_VERSION EQUAL 6 OR DRIVER_CATALOG_RTJK_001_VERSION GREATER_EQUAL 10)
    list(APPEND DRIVER_CATALOG_RTJK_001
        ${TEMPLATE_ROOT}/drivers/comm/esp8266_wifi.c
        ${TEMPLATE_ROOT}/drivers/comm/esp8266_mqtt.c
    )
endif()

if(DRIVER_CATALOG_RTJK_001_VERSION EQUAL 7)
    list(APPEND DRIVER_CATALOG_RTJK_001 ${TEMPLATE_ROOT}/drivers/comm/su03t_voice.c)
endif()

if(DRIVER_CATALOG_RTJK_001_VERSION GREATER_EQUAL 8)
    list(APPEND DRIVER_CATALOG_RTJK_001 ${TEMPLATE_ROOT}/drivers/sensors/msp20_bp.c)
endif()

# KQZL3 — 空气质量检测系统（APP_VERSION 1-14 条件编译）
driver_catalog_resolve_app_version(DRIVER_CATALOG_KQZL3_VERSION KQZL3_VERSION 14)

# Base drivers for all KQZL3 versions: OLED + relay + buzzer + led + key + pm25
set(DRIVER_CATALOG_KQZL3
    ${TEMPLATE_ROOT}/drivers/displays/oled_font.c
    ${TEMPLATE_ROOT}/drivers/displays/oled_ssd1306.c
    ${TEMPLATE_ROOT}/drivers/actuators/relay.c
    ${TEMPLATE_ROOT}/drivers/misc/buzzer.c
    ${TEMPLATE_ROOT}/drivers/misc/led.c
    ${TEMPLATE_ROOT}/drivers/misc/key.c
    ${TEMPLATE_ROOT}/drivers/sensors/pm25.c
)

# Version 2+: DHT11 temperature/humidity
if(DRIVER_CATALOG_KQZL3_VERSION GREATER_EQUAL 2)
    list(APPEND DRIVER_CATALOG_KQZL3 ${TEMPLATE_ROOT}/drivers/sensors/dht11.c)
endif()

# Versions 3,6,8,9,10,12,14: ESP8266 WiFi + MQTT
if(DRIVER_CATALOG_KQZL3_VERSION EQUAL 3 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 6 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 8 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 9 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 10 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 12 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 14)
    list(APPEND DRIVER_CATALOG_KQZL3
        ${TEMPLATE_ROOT}/drivers/comm/esp8266_wifi.c
        ${TEMPLATE_ROOT}/drivers/comm/esp8266_mqtt.c
    )
endif()

# Versions 4,7,13: JDY-31 Bluetooth
if(DRIVER_CATALOG_KQZL3_VERSION EQUAL 4 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 7 OR DRIVER_CATALOG_KQZL3_VERSION EQUAL 13)
    list(APPEND DRIVER_CATALOG_KQZL3 ${TEMPLATE_ROOT}/drivers/comm/jdy31_ble.c)
endif()

# Version 5+: MQ-2 smoke sensor
if(DRIVER_CATALOG_KQZL3_VERSION GREATER_EQUAL 5)
    list(APPEND DRIVER_CATALOG_KQZL3 ${TEMPLATE_ROOT}/drivers/sensors/mq2_smoke.c)
endif()

# Version 11+: MQ-7 CO sensor
if(DRIVER_CATALOG_KQZL3_VERSION GREATER_EQUAL 11)
    list(APPEND DRIVER_CATALOG_KQZL3 ${TEMPLATE_ROOT}/drivers/sensors/mq7_co.c)
endif()

# DCLD_001 — 倒车雷达（APP_VERSION 1-7 条件编译）
driver_catalog_resolve_app_version(DRIVER_CATALOG_DCLD_001_VERSION DCLD_VERSION 7)

set(DRIVER_CATALOG_DCLD_001
    ${TEMPLATE_ROOT}/drivers/displays/oled_font.c
    ${TEMPLATE_ROOT}/drivers/displays/oled_ssd1306.c
    ${TEMPLATE_ROOT}/drivers/sensors/hcsr04.c
    ${TEMPLATE_ROOT}/drivers/misc/key.c
    ${TEMPLATE_ROOT}/drivers/misc/buzzer.c
    ${TEMPLATE_ROOT}/drivers/misc/led.c
)

if(DRIVER_CATALOG_DCLD_001_VERSION GREATER_EQUAL 2)
    list(APPEND DRIVER_CATALOG_DCLD_001 ${TEMPLATE_ROOT}/drivers/sensors/ds18b20.c)
endif()

if(DRIVER_CATALOG_DCLD_001_VERSION EQUAL 3 OR DRIVER_CATALOG_DCLD_001_VERSION EQUAL 6 OR DRIVER_CATALOG_DCLD_001_VERSION EQUAL 7)
    list(APPEND DRIVER_CATALOG_DCLD_001 ${TEMPLATE_ROOT}/drivers/comm/su03t_voice.c)
endif()

if(DRIVER_CATALOG_DCLD_001_VERSION EQUAL 4 OR DRIVER_CATALOG_DCLD_001_VERSION EQUAL 6 OR DRIVER_CATALOG_DCLD_001_VERSION EQUAL 7)
    list(APPEND DRIVER_CATALOG_DCLD_001
        ${TEMPLATE_ROOT}/drivers/comm/esp8266_wifi.c
        ${TEMPLATE_ROOT}/drivers/comm/esp8266_mqtt.c
    )
endif()

if(DRIVER_CATALOG_DCLD_001_VERSION EQUAL 5)
    list(APPEND DRIVER_CATALOG_DCLD_001 ${TEMPLATE_ROOT}/drivers/comm/jdy31_ble.c)
endif()

