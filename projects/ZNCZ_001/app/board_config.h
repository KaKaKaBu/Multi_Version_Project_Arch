#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "gpio_hal.h"
#include "usart_hal.h"
#include "stm32f10x.h"

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
static const hal_pin_t board_debug_uart_tx = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
#endif


/* -------------------------------------------------------------------------- */
/* ESP8266 WiFi / MQTT (USART3)                                                 */
/* -------------------------------------------------------------------------- */
#define BOARD_USART3_BAUDRATE 115200U

#define BOARD_ESP8266_USART USART3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA DMA1_Channel3
#endif

static const hal_pin_t board_esp8266_tx = { GPIOB, GPIO_Pin_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { GPIOB, GPIO_Pin_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };

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
/* OLED SSD1306 (I2C1 PB6/PB7)                                                  */
/* -------------------------------------------------------------------------- */
#if HAL_I2C_USE_SOFT
static const hal_pin_t board_oled_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_OUT_OD };
#else
static const hal_pin_t board_oled_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_oled_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_AF_OD };
#endif

#define BOARD_OLED_I2C I2C1
#define BOARD_OLED_I2C_SPEED 400000U
#define BOARD_OLED_I2C_ADDR 0x78U
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE

/* -------------------------------------------------------------------------- */
/* Relay (PA11)                                                                 */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_relay_pin = { GPIOA, GPIO_Pin_11, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* LED (PC13，与 Debug UART 共用时会自动跳过初始化)                              */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_led_pin = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* DS1302 RTC (RST PB8 / DAT PB9 / CLK PB15)                                    */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_ds1302_ce_pin = { GPIOB, GPIO_Pin_15, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_data_pin = { GPIOB, GPIO_Pin_9, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_sclk_pin = { GPIOB, GPIO_Pin_8, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 5 Keys (PB4/PB5/PB12/PB13/PB14)                                              */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_key1_pin = { GPIOB, GPIO_Pin_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { GPIOB, GPIO_Pin_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key5_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_IN_PULLUP };

#endif
