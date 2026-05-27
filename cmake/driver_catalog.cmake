# Catalog of all migrated device drivers under drivers/
# Include this file after setting TEMPLATE_ROOT, then use DRIVER_CATALOG_ALL or pick subsets.

set(DRIVER_CATALOG_SENSORS
    ${TEMPLATE_ROOT}/drivers/sensors/dht11.c
    ${TEMPLATE_ROOT}/drivers/sensors/ds18b20.c
    ${TEMPLATE_ROOT}/drivers/sensors/bmp180.c
    ${TEMPLATE_ROOT}/drivers/sensors/mpu6050.c
    ${TEMPLATE_ROOT}/drivers/sensors/max30102.c
    ${TEMPLATE_ROOT}/drivers/sensors/pm25.c
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
