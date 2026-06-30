/**
 * @file board_config.h
 * @brief ZHJG_001 智能井盖板级资源配置。
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "version_config.h"
#include "gpio_hal.h"
#include "usart_hal.h"

#define BOARD_USART1_BAUDRATE 115200U
#define BOARD_USART2_BAUDRATE 9600U
#define BOARD_USART3_BAUDRATE 115200U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

/** @name OLED / MPU6050 shared HAL_I2C_ID_1 bus (PB6/PB7). */
#if HAL_I2C_USE_SOFT
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_sensor_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_sensor_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_OUT_OD };
#else
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_sensor_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_sensor_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_AF_OD };
#endif
#define BOARD_OLED_I2C HAL_I2C_ID_1
#define BOARD_OLED_I2C_SPEED 400000U
#define BOARD_OLED_I2C_ADDR 0x78U
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_SENSOR_I2C HAL_I2C_ID_1
#define BOARD_SENSOR_I2C_SPEED 400000U
#define BOARD_SENSOR_I2C_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_MPU6050_I2C_ADDR 0xD0U

/** @name MQ-4 methane and analog water-level sensors. */
#if HAL_ADC_ENABLE
#define BOARD_ADC_INSTANCE HAL_ADC_ID_1
#define BOARD_MQ4_ADC_CHANNEL HAL_ADC_CH_0
#define BOARD_WATER_LEVEL_ADC_CHANNEL HAL_ADC_CH_1
#define BOARD_WATER_LEVEL_INVERT 0
static const hal_pin_t board_mq4_adc_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_water_level_adc_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };
#endif

/** @name Alarm outputs. */
static const hal_pin_t board_buzzer_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_OUT_PP };

/** @name Front-panel keys: mode, threshold select, increment, decrement. */
static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t *const board_zhjg_key_pins[] = {
    &board_key1_pin,
    &board_key2_pin,
    &board_key3_pin,
    &board_key4_pin
};
#define KEY_DRIVER_PIN_TABLE board_zhjg_key_pins
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(board_zhjg_key_pins) / sizeof(board_zhjg_key_pins[0])))

#if VERSION_FEATURE_GPS
/** @name L76K GNSS receiver (HAL_USART_ID_2 PA2/PA3). */
#define BOARD_L76K_USART HAL_USART_ID_2
#define BOARD_L76K_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_L76K_USART_REMAP GPIO_HAL_REMAP_NONE
static const hal_pin_t board_l76k_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_l76k_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if VERSION_FEATURE_SMS
/** @name A7670C SMS modem (HAL_USART_ID_1 PA9/PA10). */
#define BOARD_A7670C_USART HAL_USART_ID_1
#define BOARD_A7670C_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_A7670C_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_A7670C_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_A7670C_USART_TX_DMA HAL_DMA_CH_4
#endif
static const hal_pin_t board_a7670c_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_a7670c_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_A7670C_ALERT_PHONE "15617830769"
#endif

#if VERSION_FEATURE_WIFI
/** @name ESP-01S / ESP8266 Wi-Fi MQTT (HAL_USART_ID_3 PB10/PB11). */
#define BOARD_ESP8266_USART HAL_USART_ID_3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA HAL_DMA_CH_2
#endif
static const hal_pin_t board_esp8266_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_A, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "ZHJG_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZHJG_001"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZHJG_001/web"
#define BOARD_COMM_DEVICE "esp8266"
#endif

#define BOARD_HAS_BUZZER 1
#define BOARD_HAS_LED 1

#endif
