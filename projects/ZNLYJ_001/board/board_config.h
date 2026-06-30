/**
 * @file board_config.h
 * @brief ZNLYJ_001 智能晾衣架板级引脚和通讯参数。
 */

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
#define BOARD_ESP8266_DEBUG_TRACE_ENABLE 0
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

/* -------------------------------------------------------------------------- */
/* OLED SSD1306：HAL_I2C_ID_1 PB6/PB7，软件/硬件 I2C 由 HAL_I2C_USE_SOFT 决定。          */
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
/* DHT11 温湿度：PA0 单总线。                                                    */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_dht11_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_OUT_OD };

/* -------------------------------------------------------------------------- */
/* GL5506 光照传感器：v2+ 使用 PA1 ADC。                                        */
/* -------------------------------------------------------------------------- */
#if VERSION_FEATURE_LIGHT
#if HAL_ADC_ENABLE
#define BOARD_ADC_INSTANCE HAL_ADC_ID_1
#define BOARD_GL5506_INVERT 1
#define BOARD_GL5506_ADC_CHANNEL HAL_ADC_CH_1
static const hal_pin_t board_gl5506_adc_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };
#endif
#endif

/* -------------------------------------------------------------------------- */
/* E18-D80NK 人体检测：PB2，低电平有效。                                         */
/* -------------------------------------------------------------------------- */
#define BOARD_E18_PIR_COUNT 1U
#define BOARD_E18_PIR_ACTIVE_LOW 1
static const hal_pin_t board_e18_pir1_pin = { HAL_PORT_B, HAL_PIN_2, GPIO_HAL_MODE_IN_PULLUP };

/* -------------------------------------------------------------------------- */
/* 红外衣物检测：PB3。                                                           */
/* -------------------------------------------------------------------------- */
#define BOARD_IR_CLOTHES_ACTIVE_LOW 0
static const hal_pin_t board_ir_clothes_pin = { HAL_PORT_B, HAL_PIN_3, GPIO_HAL_MODE_IN_PULLUP };

/* -------------------------------------------------------------------------- */
/* 步进电机：PA4/PA5/PA6/PA7 控制晾衣架开合。                                   */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_stepmotor_a_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_b_pin = { HAL_PORT_A, HAL_PIN_5, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_c_pin = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_d_pin = { HAL_PORT_A, HAL_PIN_7, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 蜂鸣器：PB8。                                                                 */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_buzzer_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* HX711 重量传感器：v2+ 使用 PA11 SCK / PA12 DT。                              */
/* -------------------------------------------------------------------------- */
#if VERSION_FEATURE_WEIGHT
static const hal_pin_t board_hx711_sck_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hx711_dt_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_HX711_SCALE 420.0f
#define BOARD_HX711_OFFSET 8388608UL
#endif

/* -------------------------------------------------------------------------- */
/* 4 个按键：模式、开合/选择、加、减。                                           */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t *const board_znlyj_key_pins[] = {
    &board_key1_pin,
    &board_key2_pin,
    &board_key3_pin,
    &board_key4_pin
};
#define KEY_DRIVER_PIN_TABLE board_znlyj_key_pins
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(board_znlyj_key_pins) / sizeof(board_znlyj_key_pins[0])))

/* -------------------------------------------------------------------------- */
/* ESP8266 WiFi：v4 使用 HAL_USART_ID_3 PB10/PB11，CH_PD/RST 用 PB0/PB1。              */
/* -------------------------------------------------------------------------- */
#if VERSION_FEATURE_WIFI
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
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "ZNLYJ_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZNLYJ_001/sub"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZNLYJ_001/pub"
#endif

/* -------------------------------------------------------------------------- */
/* JDY-31 蓝牙：v3 使用 HAL_USART_ID_2 PA2/PA3。                                        */
/* -------------------------------------------------------------------------- */
#if VERSION_FEATURE_BLE
#define BOARD_JDY31_USART HAL_USART_ID_2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA HAL_DMA_CH_7
#endif
static const hal_pin_t board_jdy31_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if VERSION_FEATURE_WIFI
#define BOARD_COMM_DEVICE "esp8266"
#elif VERSION_FEATURE_BLE
#define BOARD_COMM_DEVICE "jdy31"
#else
#define BOARD_COMM_DEVICE ""
#endif

#define BOARD_HAS_DHT11 1
#define BOARD_HAS_E18_PRESENCE 1
#define BOARD_HAS_IR_CLOTHES 1
#define BOARD_HAS_STEPMOTOR 1
#define BOARD_HAS_BUZZER 1
#if VERSION_FEATURE_WEIGHT
#define BOARD_HAS_HX711 1
#endif

#endif
