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
static const hal_pin_t board_debug_uart_tx = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
#endif

#define BOARD_USART1_BAUDRATE 9600U
#define BOARD_USART2_BAUDRATE 9600U
#define BOARD_USART3_BAUDRATE 115200U

/* -------------------------------------------------------------------------- */
/* Shared sensor I2C (OLED + MAX30102) PB6/PB7                                  */
/* -------------------------------------------------------------------------- */
#if HAL_I2C_USE_SOFT
static const hal_pin_t board_oled_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_sensor_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_sensor_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_OUT_OD };
#else
static const hal_pin_t board_oled_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_oled_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_sensor_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_sensor_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_AF_OD };
#endif

#define BOARD_OLED_I2C I2C1
#define BOARD_OLED_I2C_SPEED 400000U
#define BOARD_OLED_I2C_ADDR 0x78U
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE

#define BOARD_SENSOR_I2C I2C1
#define BOARD_SENSOR_I2C_SPEED 400000U
#define BOARD_SENSOR_I2C_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_MAX30102_I2C_ADDR 0xAEU

/* -------------------------------------------------------------------------- */
/* Keys K1-K4 (PB4/PB5/PB12/PB13) + unused K5                                   */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_key1_pin = { GPIOB, GPIO_Pin_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { GPIOB, GPIO_Pin_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key5_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_IN_PULLUP };

/* -------------------------------------------------------------------------- */
/* Buzzer PB8 / LED PC13                                                        */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_buzzer_pin = { GPIOB, GPIO_Pin_8, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* DS18B20 PA1 (V4+)                                                            */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_ds18b20_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_OUT_OD };

/* -------------------------------------------------------------------------- */
/* MSP20 ADC PA0 (V8+)                                                          */
/* -------------------------------------------------------------------------- */
#if HAL_ADC_ENABLE
static const hal_pin_t board_msp20_adc_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_ANALOG };
#define BOARD_ADC_INSTANCE ADC1
#define BOARD_MSP20_ADC_CHANNEL ADC_Channel_0
#define BOARD_MSP20_VOLT_TO_SYS_K 60.0f
#define BOARD_MSP20_VOLT_TO_SYS_OFFSET 40.0f
#define BOARD_MSP20_VOLT_TO_DIA_K 40.0f
#define BOARD_MSP20_VOLT_TO_DIA_OFFSET 20.0f
#define BOARD_MSP20_DIA_RATIO 0.67f
#endif

/* -------------------------------------------------------------------------- */
/* ESP8266 USART3 (V3/V6/V10)                                                   */
/* -------------------------------------------------------------------------- */
#define BOARD_ESP8266_USART USART3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ

static const hal_pin_t board_esp8266_tx = { GPIOB, GPIO_Pin_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { GPIOB, GPIO_Pin_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "47.94.89.46"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "yskj_rtjk_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "RTJK_001/cmd"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "RTJK_001"

/* -------------------------------------------------------------------------- */
/* JDY31 BLE USART2 (V2/V5)                                                     */
/* -------------------------------------------------------------------------- */
#define BOARD_JDY31_USART USART2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ

static const hal_pin_t board_jdy31_tx = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_IN_FLOATING };

/* -------------------------------------------------------------------------- */
/* SU03T Voice USART1 (V7)                                                      */
/* -------------------------------------------------------------------------- */
#define BOARD_SU03T_USART USART1
#define BOARD_SU03T_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_SU03T_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_SU03T_USART_TX_MODE USART_HAL_TX_MODE_IRQ

static const hal_pin_t board_su03t_tx = { GPIOA, GPIO_Pin_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_su03t_rx = { GPIOA, GPIO_Pin_10, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_VOICE_CMD_HR 1
#define BOARD_VOICE_CMD_SPO2 2
#define BOARD_VOICE_CMD_TEMP 3

#if VERSION_FEATURE_WIFI
#define BOARD_COMM_DEVICE "esp8266"
#elif VERSION_FEATURE_BLE
#define BOARD_COMM_DEVICE "jdy31"
#else
#define BOARD_COMM_DEVICE "none"
#endif

#endif
