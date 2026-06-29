/**
 * @file board_config.h
 * @brief WXCS_001 车辆尾气检测板级引脚和通讯参数。
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#if defined(PLATFORM_MCS51)
#include "board_config_mcs51.h"
#else

#include "version_config.h"
#include "gpio_hal.h"
#include "usart_hal.h"
#include "stm32f10x.h"

/** @brief USART2 baud rate (JDY-31 BLE). */
#define BOARD_USART2_BAUDRATE 9600U
/** @brief USART3 baud rate (ESP8266). */
#define BOARD_USART3_BAUDRATE 115200U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
static const hal_pin_t board_debug_uart_tx = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
#endif

/* -------------------------------------------------------------------------- */
/* OLED SSD1306：I2C1 PB6/PB7，软件/硬件 I2C 由 HAL_I2C_USE_SOFT 决定。          */
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
/* MQ135 空气质量传感器：ADC1 PA0。                                             */
/* -------------------------------------------------------------------------- */
#if HAL_ADC_ENABLE
#define BOARD_ADC_INSTANCE ADC1
#define BOARD_MQ135_ADC_CHANNEL ADC_Channel_0
static const hal_pin_t board_mq135_adc_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_ANALOG };

/* MQ-7 CO 传感器：ADC1 PA1。 */
#define BOARD_MQ7_ADC_CHANNEL ADC_Channel_1
static const hal_pin_t board_mq7_adc_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_ANALOG };
#endif

/* -------------------------------------------------------------------------- */
/* DS18B20 温度传感器：PB12 单总线。                                             */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_ds18b20_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_OUT_OD };

/* -------------------------------------------------------------------------- */
/* 排风扇继电器：PA4。                                                          */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_relay_pin = { GPIOA, GPIO_Pin_4, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 蜂鸣器报警：PB8。                                                            */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_buzzer_pin = { GPIOB, GPIO_Pin_8, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 状态 LED：PC13。                                                             */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_led_pin = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 4 个按键：模式、选择、加、减。                                                */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_key1_pin = { GPIOB, GPIO_Pin_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { GPIOB, GPIO_Pin_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t *const board_wxcs_key_pins[] = {
    &board_key1_pin,
    &board_key2_pin,
    &board_key3_pin,
    &board_key4_pin
};
#define KEY_DRIVER_PIN_TABLE board_wxcs_key_pins
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(board_wxcs_key_pins) / sizeof(board_wxcs_key_pins[0])))

/* -------------------------------------------------------------------------- */
/* ESP8266 WiFi：v2 使用 USART3 PB10/PB11，CH_PD/RST 用 PB0/PB1。              */
/* -------------------------------------------------------------------------- */
#if VERSION_FEATURE_WIFI
#define BOARD_ESP8266_USART USART3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA DMA1_Channel2
#endif
static const hal_pin_t board_esp8266_tx = { GPIOB, GPIO_Pin_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { GPIOB, GPIO_Pin_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "WXCS_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "WXCS_001/sub"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "WXCS_001/pub"
#endif

/* -------------------------------------------------------------------------- */
/* JDY-31 蓝牙：v3 使用 USART2 PA2/PA3。                                        */
/* -------------------------------------------------------------------------- */
#if VERSION_FEATURE_BLE
#define BOARD_JDY31_USART USART2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA DMA1_Channel7
#endif
static const hal_pin_t board_jdy31_tx = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if VERSION_FEATURE_WIFI
#define BOARD_COMM_DEVICE "esp8266"
#elif VERSION_FEATURE_BLE
#define BOARD_COMM_DEVICE "jdy31"
#else
#define BOARD_COMM_DEVICE ""
#endif

#endif

#endif
