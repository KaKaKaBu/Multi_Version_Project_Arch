/**
 * @file board_config.h
 * @brief KQZL3 board pin map, peripheral instances, and HAL feature overrides.
 */

/* KQZL3 板级配置：集中定义传感器、执行器、通讯端口和 MQTT 参数。 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "version_config.h"
#include "gpio_hal.h"
#include "usart_hal.h"

#define BOARD_USART2_BAUDRATE 9600U
#define BOARD_USART3_BAUDRATE 115200U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

/** @name ESP8266 Wi-Fi / MQTT (HAL_USART_ID_3 PB10/PB11). */
#define BOARD_ESP8266_USART HAL_USART_ID_3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA HAL_DMA_CH_3
#endif
static const hal_pin_t board_esp8266_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "KQZL3"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "KQZL3"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "KQZL3/web"

/** @name JDY-31 Bluetooth (HAL_USART_ID_2 PA2/PA3). */
#define BOARD_JDY31_USART HAL_USART_ID_2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA HAL_DMA_CH_7
#endif
static const hal_pin_t board_jdy31_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };

#if VERSION_FEATURE_BLE
#define BOARD_COMM_DEVICE "jdy31"
#else
#define BOARD_COMM_DEVICE "esp8266"
#endif

/** @name OLED display I2C bus (HAL_I2C_ID_1 PB6/PB7). */
#if HAL_I2C_USE_SOFT
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_OUT_OD };
#else
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_AF_OD };
#endif
#define BOARD_OLED_I2C HAL_I2C_ID_1
#define BOARD_OLED_I2C_SPEED 400000U
#define BOARD_OLED_I2C_ADDR 0x78U
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE

/** @name GPIO sensors and actuators. */
static const hal_pin_t board_dht11_pin = { HAL_PORT_A, HAL_PIN_5, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_relay_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_buzzer_pin = { HAL_PORT_A, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };

#if HAL_ADC_ENABLE
static const hal_pin_t board_pm25_adc_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_pm25_led_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_mq2_adc_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_mq7_adc_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_ANALOG };

#define BOARD_ADC_INSTANCE HAL_ADC_ID_1
#define BOARD_PM25_ADC_CHANNEL HAL_ADC_CH_0
#define BOARD_PM25_LED_ACTIVE_LEVEL 0U
#define BOARD_PM25_ADC_VREF_MV 3300.0f
#define BOARD_PM25_VOUT_SCALE 1.0f
#define BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V 170.0f
#define BOARD_PM25_DENSITY_OFFSET_UG_M3 (-100.0f)
#define BOARD_PM25_DENSITY_MAX_UG_M3 500.0f
#define BOARD_MQ2_ADC_CHANNEL HAL_ADC_CH_1
#define BOARD_MQ7_ADC_CHANNEL HAL_ADC_CH_4
#endif

/** @name Front-panel keys (active low, internal pull-up). */
static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t *const board_kqzl3_key_pins[] = {
    &board_key1_pin,
    &board_key2_pin,
    &board_key3_pin,
    &board_key4_pin
};
#define KEY_DRIVER_PIN_TABLE board_kqzl3_key_pins
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(board_kqzl3_key_pins) / sizeof(board_kqzl3_key_pins[0])))

#define BOARD_HAS_DHT11 1
#define BOARD_HAS_RELAY 1
#define BOARD_HAS_LED 1
#define BOARD_HAS_BUZZER 1

#endif
