#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "gpio_hal.h"
#include "usart_hal.h"

/* -------------------------------------------------------------------------- */
/* Debug UART (PC13 bit-bang)                                                   */
/* -------------------------------------------------------------------------- */
#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 38400U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
#define DEBUG_LOG_ENABLE 0
#define DEBUG_LOG_APP_ENABLE 0
#define DEBUG_LOG_APP_CB_ENABLE 0
#define DEBUG_LOG_ESP8266_ENABLE 0
#define DEBUG_LOG_ESP8266_DRAIN_ENABLE 0
#define DEBUG_LOG_MQTT_ENABLE 0
#define DEBUG_LOG_MQTT_AT_ENABLE 0
#define DEBUG_LOG_MQTT_POLL_ENABLE 0
#define DEBUG_LOG_IRQ_EVENT_ENABLE 0
#define DEBUG_LOG_USART_ENABLE 0
#define DEBUG_LOG_RTC_ENABLE 0
#define DEBUG_LOG_SCHED_ENABLE 0
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif


/* -------------------------------------------------------------------------- */
/* ESP8266 WiFi / MQTT (HAL_USART_ID_3)                                                 */
/* -------------------------------------------------------------------------- */
#define BOARD_USART3_BAUDRATE 115200U

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
#define BOARD_ESP8266_MQTT_BROKER "47.94.89.46"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "yskj_zncz_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZNCZ_001/web"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZNCZ_001"

#define BOARD_COMM_DEVICE "esp8266"

/* -------------------------------------------------------------------------- */
/* OLED SSD1306 (HAL_I2C_ID_1 PB6/PB7)                                                  */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/* Relay (PA11)                                                                 */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_relay_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* LED (PC13，与 Debug UART 共用时会自动跳过初始化)                              */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_led_pin = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* DS1302 RTC (RST PB8 / DAT PB9 / CLK PB15)                                    */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_ds1302_ce_pin = { HAL_PORT_B, HAL_PIN_15, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_data_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_sclk_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 5 Keys (PB4/PB5/PB12/PB13/PB14)                                              */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key5_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_IN_PULLUP };

#define BOARD_KEY_COUNT 5U
#define BOARD_HAS_RELAY 1
#define BOARD_HAS_LED 1
#define BOARD_HAS_DS1302 1

#endif
