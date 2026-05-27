#ifndef BOARD_CONFIG_TEMPLATE_H
#define BOARD_CONFIG_TEMPLATE_H

#include "gpio_hal.h"
#include "usart_hal.h"
#include "stm32f10x.h"

/*
 * HAL 功能开关（也可在 CMake 中通过 -DHAL_*=ON 或 option() 设置）：
 *   HAL_I2C_USE_SOFT=1       软件 I2C，SCL/SDA 引脚 mode 应设为 GPIO_HAL_MODE_OUT_OD
 *   HAL_SPI_USE_SOFT=1       软件 SPI
 *   HAL_SPI_USE_HW=1         硬件 SPI（与 HAL_SPI_USE_SOFT 互斥）
 *   HAL_SPI_ENABLE_DMA=1     硬件 SPI DMA（需 HAL_SPI_USE_HW）
 *   HAL_USART_ENABLE_DMA=1   USART DMA TX
 *   HAL_ADC_ENABLE=1         ADC 轮询采集
 *   HAL_ADC_ENABLE_DMA=1     ADC DMA 连续采集（需 HAL_ADC_ENABLE）
 *   HAL_DEBUG_UART_ENABLE=1  PC13 等 GPIO 软串口 TX，printf 输出调试信息
 */

#define BOARD_USART1_BAUDRATE 115200U
#define BOARD_USART2_BAUDRATE 9600U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
static const hal_pin_t board_debug_uart_tx = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
#endif

static const hal_pin_t board_usart1_tx = { GPIOA, GPIO_Pin_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_usart1_rx = { GPIOA, GPIO_Pin_10, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_usart2_tx = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_usart2_rx = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_ESP8266_USART USART1
#define BOARD_ESP8266_USART_ID 1U
#define BOARD_ESP8266_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA DMA1_Channel4
#endif
static const hal_pin_t board_esp8266_ch_pd_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "WIFI_SSID"
#define BOARD_ESP8266_WIFI_PASS "WIFI_PASSWORD"
#define BOARD_ESP8266_MQTT_BROKER "broker.emqx.io"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "ZNCZ_001"
#define BOARD_ESP8266_MQTT_USER ""
#define BOARD_ESP8266_MQTT_PASS ""
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZNCZ_001/cmd"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZNCZ_001/telemetry"

#define BOARD_JDY31_USART USART2
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA DMA1_Channel7
#endif

/* 应用层默认通讯后端：esp8266 / jdy31 / nrf24 */
#define BOARD_COMM_DEVICE "esp8266"

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

static const hal_pin_t board_dht11_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_ds18b20_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_relay_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_buzzer_pin = { GPIOB, GPIO_Pin_8, GPIO_HAL_MODE_OUT_PP };

static const hal_pin_t board_sg90_pwm_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_AF_PP };

#define BOARD_SG90_TIM TIM2
#define BOARD_SG90_TIM_CHANNEL 1U
#define BOARD_SG90_TIM_PRESCALER (72U - 1U)
#define BOARD_SG90_TIM_PERIOD (20000U - 1U)
#define BOARD_SG90_TIM_REMAP GPIO_HAL_REMAP_NONE

#if HAL_SPI_USE_SOFT
static const hal_pin_t board_spi_sck = { GPIOA, GPIO_Pin_5, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_spi_mosi = { GPIOA, GPIO_Pin_7, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_spi_miso = { GPIOA, GPIO_Pin_6, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if HAL_SPI_USE_HW
static const hal_pin_t board_hw_spi1_sck = { GPIOA, GPIO_Pin_5, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_hw_spi1_mosi = { GPIOA, GPIO_Pin_7, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_hw_spi1_miso = { GPIOA, GPIO_Pin_6, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_HW_SPI1_INSTANCE SPI1
#define BOARD_HW_SPI1_PRESCALER SPI_BaudRatePrescaler_8
#define BOARD_HW_SPI1_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_HW_SPI1_CPOL 0U
#define BOARD_HW_SPI1_CPHA 0U
#if HAL_SPI_ENABLE_DMA
#define BOARD_HW_SPI1_TX_DMA DMA1_Channel3
#define BOARD_HW_SPI1_RX_DMA DMA1_Channel2
#endif
#endif

#if HAL_ADC_ENABLE
static const hal_pin_t board_adc0_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_adc1_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_pm25_adc_pin = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_ph_adc_pin = { GPIOA, GPIO_Pin_4, GPIO_HAL_MODE_ANALOG };

#define BOARD_ADC_INSTANCE ADC1
#define BOARD_ADC0_CHANNEL ADC_Channel_0
#define BOARD_ADC1_CHANNEL ADC_Channel_1
#define BOARD_PM25_ADC_CHANNEL ADC_Channel_2
#define BOARD_PH_ADC_CHANNEL ADC_Channel_4
#define BOARD_PH_VOLTAGE_COEFF (-5.6342f)
#define BOARD_PH_VOLTAGE_OFFSET 16.413f
#if HAL_ADC_ENABLE_DMA
#define BOARD_ADC_DMA DMA1_Channel1
#define BOARD_ADC_DMA_CHANNEL_COUNT 2U
static const uint8_t board_adc_dma_channels[BOARD_ADC_DMA_CHANNEL_COUNT] = {
    ADC_Channel_0,
    ADC_Channel_1
};
#endif
#endif

/* Shared sensor I2C bus (OLED / BMP180 / MPU6050 / MAX30102) */
#if HAL_I2C_USE_SOFT
static const hal_pin_t board_sensor_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_sensor_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_OUT_OD };
#else
static const hal_pin_t board_sensor_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_sensor_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_AF_OD };
#endif
#define BOARD_SENSOR_I2C I2C1
#define BOARD_SENSOR_I2C_SPEED 400000U
#define BOARD_SENSOR_I2C_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_BMP180_I2C_ADDR 0xEEU
#define BOARD_MPU6050_I2C_ADDR 0xD0U
#define BOARD_MAX30102_I2C_ADDR 0xAEU
#define BOARD_BMP180_PRESSURE_REVISE 526

static const hal_pin_t board_hcsr04_trig_pin = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hcsr04_echo_pin = { GPIOA, GPIO_Pin_4, GPIO_HAL_MODE_IN_FLOATING };

static const hal_pin_t board_hx711_sck_pin = { GPIOA, GPIO_Pin_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hx711_dt_pin = { GPIOA, GPIO_Pin_12, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_HX711_SCALE 420.0f
#define BOARD_HX711_OFFSET 8388608UL

static const hal_pin_t board_ds1302_ce_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_data_pin = { GPIOB, GPIO_Pin_2, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_ds1302_sclk_pin = { GPIOB, GPIO_Pin_3, GPIO_HAL_MODE_OUT_PP };

static const hal_pin_t board_key1_pin = { GPIOB, GPIO_Pin_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { GPIOB, GPIO_Pin_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { GPIOB, GPIO_Pin_9, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key5_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_IN_PULLUP };

static const hal_pin_t board_stepmotor_a_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_b_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_c_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_d_pin = { GPIOB, GPIO_Pin_15, GPIO_HAL_MODE_OUT_PP };

static const hal_pin_t board_hc595_1_load_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hc595_1_sclk_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hc595_1_sdi_pin = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_OUT_PP };

#define BOARD_CO2_USART USART2
#define BOARD_CO2_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_CO2_USART_REMAP GPIO_HAL_REMAP_NONE

static const hal_pin_t board_usart3_tx = { GPIOB, GPIO_Pin_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_usart3_rx = { GPIOB, GPIO_Pin_11, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_SU03T_USART USART3
#define BOARD_SU03T_BAUDRATE 9600U
#define BOARD_SU03T_USART_REMAP GPIO_HAL_REMAP_NONE

#define BOARD_L76K_USART USART3
#define BOARD_L76K_BAUDRATE 9600U
#define BOARD_L76K_USART_REMAP GPIO_HAL_REMAP_NONE

static const hal_pin_t board_sg90_2_pwm_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_AF_PP };
#define BOARD_SG90_2_TIM TIM2
#define BOARD_SG90_2_TIM_CHANNEL 2U

#if HAL_SPI_USE_HW
#define BOARD_RC522_SPI_BUS_ID 0U
static const hal_pin_t board_rc522_cs_pin = { GPIOA, GPIO_Pin_8, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_rc522_rst_pin = { GPIOA, GPIO_Pin_7, GPIO_HAL_MODE_OUT_PP };

#define BOARD_NRF24_SPI_BUS_ID 1U
static const hal_pin_t board_nrf24_ce_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_nrf24_csn_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
#endif

#endif
