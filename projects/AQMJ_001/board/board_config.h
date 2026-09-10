#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "version_config.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "usart_hal.h"

#define BOARD_USART1_BAUDRATE 9600U
#define BOARD_USART2_BAUDRATE 9600U
#define BOARD_USART3_BAUDRATE 115200U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 38400U
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

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

static const hal_pin_t board_ds1302_ce_pin = { HAL_PORT_B, HAL_PIN_15, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_data_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_ds1302_sclk_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };

static const hal_pin_t board_presence_pin = { HAL_PORT_A, HAL_PIN_7, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_PRESENCE_ACTIVE_LOW 0U

static const hal_pin_t board_doorbell_pin = { HAL_PORT_A, HAL_PIN_8, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_DOORBELL_ACTIVE_LOW 1U

static const hal_pin_t board_led_pin = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_buzzer_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
#define BOARD_LED_TRIGGER_LEVEL GPIO_OUTPUT_TRIGGER_HIGH
#define BOARD_AQMJ_BUZZER_TRIGGER_LEVEL GPIO_OUTPUT_TRIGGER_LOW

#if VERSION_FEATURE_LIGHT
static const hal_pin_t board_light_adc_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_lamp_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_OUT_PP };
#define BOARD_LIGHT_DARK_THRESHOLD 35U
#define BOARD_LAMP_TRIGGER_LEVEL GPIO_OUTPUT_TRIGGER_HIGH
#endif

#if VERSION_FEATURE_MESSAGE
static const hal_pin_t board_isd1820_rec_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_isd1820_play_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
#define BOARD_ISD1820_TRIGGER_LEVEL GPIO_OUTPUT_TRIGGER_HIGH
#define BOARD_ISD1820_RECORD_MS 10000U
#define BOARD_ISD1820_PLAY_MS 10000U
#endif

#if VERSION_FEATURE_IR_REMOTE
static const hal_pin_t board_ir_remote_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_IR_REMOTE_TIMER HAL_TIMER_ID_2
#define BOARD_IR_REMOTE_ACTIVE_LOW 1U
#endif

static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_2, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_3, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t *const board_aqmj_key_pins[] = {
    &board_key1_pin,
    &board_key2_pin,
    &board_key3_pin,
    &board_key4_pin,
    &board_doorbell_pin
};
#define KEY_DRIVER_PIN_TABLE board_aqmj_key_pins
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(board_aqmj_key_pins) / sizeof(board_aqmj_key_pins[0])))
#define BOARD_KEY_SWJ_REMAP GPIO_HAL_REMAP_SWJ_JTAG_DISABLE

#if VERSION_FEATURE_BLE
#define BOARD_JDY31_USART HAL_USART_ID_2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_jdy31_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
#define BOARD_ESP8266_USART HAL_USART_ID_3
#define BOARD_ESP8266_USART_ID 3U
#define BOARD_ESP8266_BAUDRATE BOARD_USART3_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_esp8266_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"

#if VERSION_FEATURE_WIFI
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "AQMJ_001"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "AQMJ_001"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "AQMJ_001/web"
#endif

#if VERSION_FEATURE_CLOUD
#define BOARD_ESP8266_HUAWEI_BROKER "7e87c47089.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define BOARD_ESP8266_HUAWEI_PORT 1883U
#define BOARD_ESP8266_HUAWEI_DEVICE_ID "6a476514e094d615924ed7ef_AQMJ_stm32"
#define BOARD_ESP8266_HUAWEI_DEVICE_SECRET "AQMJ_stm32"
#define BOARD_ESP8266_HUAWEI_SERVICE_ID "AQMJ"
#define BOARD_ESP8266_HUAWEI_CLIENT_ID BOARD_ESP8266_HUAWEI_DEVICE_ID
#define BOARD_ESP8266_HUAWEI_USER BOARD_ESP8266_HUAWEI_DEVICE_ID
#define BOARD_ESP8266_HUAWEI_PASS ""
#define BOARD_ESP8266_HUAWEI_PROPERTY_REPORT_TOPIC "$oc/devices/" BOARD_ESP8266_HUAWEI_DEVICE_ID "/sys/properties/report"
#define BOARD_ESP8266_HUAWEI_PROPERTY_SET_TOPIC "$oc/devices/" BOARD_ESP8266_HUAWEI_DEVICE_ID "/sys/properties/set/#"
#define BOARD_ESP8266_HUAWEI_CUSTOM_PUB_TOPIC "/jiabailie/M2M/AQMJ_stm32/up"
#define BOARD_ESP8266_HUAWEI_CUSTOM_SUB_TOPIC "/jiabailie/M2M/AQMJ_stm32/down"
#endif
#endif

#if VERSION_FEATURE_TTS
#define BOARD_TTS_UART_USART HAL_USART_ID_1
#define BOARD_TTS_UART_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_TTS_UART_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_TTS_UART_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_tts_uart_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_tts_uart_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if VERSION_FEATURE_BLE
#define BOARD_COMM_DEVICE "jdy31"
#elif VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
#define BOARD_COMM_DEVICE "esp8266"
#else
#define BOARD_COMM_DEVICE ""
#endif

#endif
