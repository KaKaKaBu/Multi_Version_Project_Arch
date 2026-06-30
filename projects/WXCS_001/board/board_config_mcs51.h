#ifndef BOARD_CONFIG_MCS51_H
#define BOARD_CONFIG_MCS51_H

#include "version_config.h"
#include "gpio_hal.h"
#include "i2c_hal.h"
#include "usart_hal.h"

#define BOARD_COMM_DEVICE ""

#define BOARD_OLED_I2C 0U
#define BOARD_OLED_I2C_SPEED 100000UL
#define BOARD_OLED_I2C_ADDR 0x78U
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_P1, 0U, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_P1, 1U, GPIO_HAL_MODE_OUT_OD };

static const hal_pin_t board_adc0832_cs_pin = { HAL_PORT_P1, 2U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_adc0832_clk_pin = { HAL_PORT_P1, 3U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_adc0832_dio_pin = { HAL_PORT_P1, 4U, GPIO_HAL_MODE_OUT_OD };
#define BOARD_MQ135_ADC0832_CHANNEL 0U
#define BOARD_MQ7_ADC0832_CHANNEL 1U
#define BOARD_MQ135_ADC0832_MAX_PPM 10000U
#define BOARD_MQ7_ADC0832_MAX_PPM 1000U

static const hal_pin_t board_ds18b20_pin = { HAL_PORT_P1, 5U, GPIO_HAL_MODE_OUT_OD };

static const hal_pin_t board_key1_pin = { HAL_PORT_P2, 0U, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_P2, 1U, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_P2, 2U, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_P2, 3U, GPIO_HAL_MODE_IN_PULLUP };
#define BOARD_KEY_COUNT 4U

static const hal_pin_t board_relay_pin = { HAL_PORT_P2, 4U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_buzzer_pin = { HAL_PORT_P2, 5U, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { HAL_PORT_P2, 6U, GPIO_HAL_MODE_OUT_PP };

#define BOARD_HAS_DS18B20 1
#define BOARD_HAS_RELAY 1
#define BOARD_HAS_BUZZER 1
#define BOARD_HAS_LED 1

#endif
