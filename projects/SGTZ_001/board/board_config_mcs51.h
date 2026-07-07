#ifndef BOARD_CONFIG_MCS51_H
#define BOARD_CONFIG_MCS51_H

#include "version_config.h"
#include "gpio_hal.h"
#include "usart_hal.h"

#if VERSION_FEATURE_WIFI
#define BOARD_COMM_DEVICE "esp8266"
#else
#define BOARD_COMM_DEVICE ""
#endif

#define BOARD_OLED_I2C 0U
#define BOARD_OLED_I2C_SPEED 100000UL
#define BOARD_OLED_I2C_ADDR 0x78U
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_P1, 0U, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_P1, 1U, GPIO_HAL_MODE_OUT_OD };

static const hal_pin_t board_hx711_sck_pin = { HAL_PORT_P1, 2U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hx711_data_pin = { HAL_PORT_P1, 3U, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_HX711_SCALE 420.0f
#define BOARD_HX711_OFFSET 0UL
#define BOARD_WEIGHT_ZERO_DEADBAND_G 5UL
#define BOARD_PRESSURE_TRIGGER_G 500UL
#define BOARD_WEIGHT_ALARM_DEFAULT_G 3000UL
#define BOARD_WEIGHT_ALARM_STEP_G 100UL
#define BOARD_WEIGHT_ALARM_FAST_STEP_G 1000UL

static const hal_pin_t board_hcsr04_trig_pin = { HAL_PORT_P1, 4U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hcsr04_echo_pin = { HAL_PORT_P1, 5U, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_HEIGHT_SENSOR_MOUNT_CM 200U
#define BOARD_HUMAN_MIN_HEIGHT_CM 50U
#define BOARD_HUMAN_MAX_HEIGHT_CM 220U

static const hal_pin_t board_key1_pin = { HAL_PORT_P2, 0U, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_P2, 1U, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_P2, 2U, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_P2, 3U, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_KEY_COUNT 4U

static const hal_pin_t board_fan_relay_pin = { HAL_PORT_P2, 4U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_buzzer_pin = { HAL_PORT_P2, 5U, GPIO_HAL_MODE_OUT_PP };
#define BOARD_BUZZER_TRIGGER_LEVEL GPIO_OUTPUT_TRIGGER_HIGH
static const hal_pin_t board_mode_switch_pin = { HAL_PORT_P3, 2U, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_MODE_SWITCH_MANUAL_LEVEL 0U

static const hal_pin_t board_voice_tx_pin = { HAL_PORT_P3, 1U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_voice_rx_pin = { HAL_PORT_P3, 0U, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_VOICE_USART 0U
#define BOARD_VOICE_BAUDRATE 9600UL
#define BOARD_VOICE_TX_TIMEOUT_US 50000UL

static const hal_pin_t board_esp8266_tx = { HAL_PORT_P3, 1U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rx = { HAL_PORT_P3, 0U, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_P2, 6U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_P2, 7U, GPIO_HAL_MODE_OUT_PP };
#define BOARD_ESP8266_USART 0U
#define BOARD_ESP8266_USART_ID 1U
#define BOARD_ESP8266_BAUDRATE 9600UL
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#define BOARD_ESP8266_DEBUG_TRACE_ENABLE 0U

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"

#ifndef BOARD_ESP8266_MQTT_BACKEND
#define BOARD_ESP8266_MQTT_BACKEND 0U /* 0: normal MQTT, 1: Huawei Cloud IoTDA */
#endif
#define BOARD_ESP8266_MQTT_SCHEME 1U

#if BOARD_ESP8266_MQTT_BACKEND == 1U
#define BOARD_ESP8266_MQTT_BROKER "7e87c47089.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "6a3cc62518855b39c5286e1e_g212stm32_0_0_2026062506"
#define BOARD_ESP8266_MQTT_USER "6a3cc62518855b39c5286e1e_g212stm32"
#define BOARD_ESP8266_MQTT_PASS "fd5d31d99faf2bcc47854d874131926ff44424167eabba5a1d284c5ddaa26aeb"
#define BOARD_ESP8266_HUAWEI_DEVICE_ID "6a3cc62518855b39c5286e1e_g212stm32"
#define BOARD_ESP8266_HUAWEI_SERVICE_ID "g212stm32"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "$oc/devices/" BOARD_ESP8266_HUAWEI_DEVICE_ID "/sys/properties/set/#"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "$oc/devices/" BOARD_ESP8266_HUAWEI_DEVICE_ID "/sys/properties/report"
#define BOARD_ESP8266_HUAWEI_CUSTOM_SUB_TOPIC "/jiabailie/M2M/g212stm32/down"
#else
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "SGTZ_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "SGTZ_001/sub"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "SGTZ_001/pub"
#define BOARD_ESP8266_HUAWEI_DEVICE_ID ""
#define BOARD_ESP8266_HUAWEI_SERVICE_ID ""
#define BOARD_ESP8266_HUAWEI_CUSTOM_SUB_TOPIC ""
#endif

#define BOARD_BMI_LIGHT_MAX_X10 184U
#define BOARD_BMI_NORMAL_MAX_X10 239U

#define BOARD_HAS_OLED 1
#define BOARD_HAS_HX711 1
#define BOARD_HAS_HCSR04 1
#define BOARD_HAS_RELAY 1
#define BOARD_HAS_BUZZER 1
#define BOARD_HAS_KEYS 1

#endif
