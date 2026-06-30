#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "gpio_hal.h"

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
/* GL5506 光敏 (HAL_ADC_ID_1 PA0) — 数值越大表示环境越亮                                 */
/* -------------------------------------------------------------------------- */
#define BOARD_GL5506_INVERT 0
#define BOARD_ADC_INSTANCE HAL_ADC_ID_1
#define BOARD_GL5506_ADC_CHANNEL HAL_ADC_CH_0
static const hal_pin_t board_gl5506_adc_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };

/* -------------------------------------------------------------------------- */
/* E18-D80NK ×3（低电平=检测到人体，可按实板改 BOARD_E18_PIR_ACTIVE_LOW）         */
/* -------------------------------------------------------------------------- */
#define BOARD_E18_PIR_COUNT 3U
#define BOARD_E18_PIR_ACTIVE_LOW 1
static const hal_pin_t board_e18_pir1_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_e18_pir2_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_e18_pir3_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_IN_PULLUP };

/* -------------------------------------------------------------------------- */
/* 照明输出 ×3 (PB3/PB4/PB5)                                                    */
/* -------------------------------------------------------------------------- */
#define BOARD_LIGHT_CHANNEL_COUNT 3U
static const hal_pin_t board_light1_pin = { HAL_PORT_B, HAL_PIN_3, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_light2_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_light3_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_OUT_PP };

/* -------------------------------------------------------------------------- */
/* 4 Keys (PB8/PB9/PB10/PB11)                                                   */
/* -------------------------------------------------------------------------- */
static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_PULLUP };

#define BOARD_KEY_COUNT 4U
#define BOARD_HAS_E18_PRESENCE 1

#endif
