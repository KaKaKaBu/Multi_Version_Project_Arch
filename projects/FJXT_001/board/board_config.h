/**
 * @file board_config.h
 * @brief FJXT_001 车窗防夹系统板级资源配置。
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
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

/** @name OLED SSD1306 display on I2C1 PB6/PB7. */
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

/** @name Four local control keys. */
static const hal_pin_t board_key_open_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key_close_pin = { HAL_PORT_A, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key_open_step_pin = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key_close_step_pin = { HAL_PORT_A, HAL_PIN_7, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t *const board_fjxt_key_pins[] = {
    &board_key_open_pin,
    &board_key_close_pin,
    &board_key_open_step_pin,
    &board_key_close_step_pin
};
#define KEY_DRIVER_PIN_TABLE board_fjxt_key_pins
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(board_fjxt_key_pins) / sizeof(board_fjxt_key_pins[0])))

/** @name Window limit and anti-pinch switches, active-low by default. */
static const hal_pin_t board_open_limit_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_close_limit_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_pinch_sensor_pin = { HAL_PORT_A, HAL_PIN_8, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_OPEN_LIMIT_ACTIVE_LOW 1U
#define BOARD_CLOSE_LIMIT_ACTIVE_LOW 1U
#define BOARD_PINCH_SENSOR_ACTIVE_LOW 1U

/** @name ULN2003 stepper motor phases. */
static const hal_pin_t board_stepmotor_a_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_b_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_c_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_d_pin = { HAL_PORT_B, HAL_PIN_15, GPIO_HAL_MODE_OUT_PP };
#define BOARD_STEPMOTOR_OPEN_DIR 1U
#define BOARD_STEPMOTOR_CLOSE_DIR 0U
#define BOARD_STEPMOTOR_STEP_DEGREE 3U
#define BOARD_STEPMOTOR_NUDGE_DEGREE 45U
#define BOARD_STEPMOTOR_REVERSE_DEGREE 90U
#define BOARD_STEPMOTOR_STEP_DELAY_MS 3U

/** @name Audible and visual reminder outputs. */
static const hal_pin_t board_buzzer_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_OUT_PP };

#if VERSION_FEATURE_BLE
/** @name JDY-31 Bluetooth serial module on USART2 PA2/PA3. */
#define BOARD_JDY31_USART HAL_USART_ID_2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_jdy31_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_COMM_DEVICE "jdy31"
#endif

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
/** @name ESP-01S/ESP8266 WiFi or cloud MQTT module on USART3 PB10/PB11. */
#define BOARD_ESP8266_USART HAL_USART_ID_3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_esp8266_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "FJXT_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "FJXT_001"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "FJXT_001/web"
#define BOARD_COMM_DEVICE "esp8266"
#endif

#if VERSION_FEATURE_VOICE
/** @name JR6001/SU03T style voice module on USART1 PA9/PA10. */
#define BOARD_SU03T_USART HAL_USART_ID_1
#define BOARD_SU03T_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_SU03T_USART_REMAP GPIO_HAL_REMAP_NONE
static const hal_pin_t board_su03t_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_su03t_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_VOICE_CMD_OPEN_DONE 1U
#define BOARD_VOICE_CMD_CLOSE_DONE 2U
#define BOARD_VOICE_CMD_PINCH 3U
#else
#define BOARD_VOICE_CMD_OPEN_DONE 1U
#define BOARD_VOICE_CMD_CLOSE_DONE 2U
#define BOARD_VOICE_CMD_PINCH 3U
#endif

#endif
