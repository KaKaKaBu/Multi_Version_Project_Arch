/**
 * @file board_config_template.h
 * @brief Default board pin map, peripheral instances, and HAL feature overrides.
 *
 * Copy to app/board_config.h and customize per product. HAL toggles may also be
 * set in CMake via hal_options.cmake (HAL_I2C_USE_SOFT, HAL_SPI_*, HAL_USART_ENABLE_DMA,
 * HAL_ADC_*, HAL_DEBUG_UART_ENABLE). When HAL_I2C_USE_SOFT=1, set SCL/SDA to OUT_OD;
 * otherwise use AF_OD for hardware I2C.
 */

#ifndef BOARD_CONFIG_TEMPLATE_H
#define BOARD_CONFIG_TEMPLATE_H

#include "gpio_hal.h"
#include "usart_hal.h"
#include "stm32f10x.h"

/** @brief USART1 baud rate (ESP8266 default). */
#define BOARD_USART1_BAUDRATE 115200U
/** @brief USART2 baud rate (JDY-31 / CO2 default). */
#define BOARD_USART2_BAUDRATE 9600U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
static const hal_pin_t board_debug_uart_tx = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
#endif

/** @brief Generic USART1 TX pin (PA9, AF push-pull). */
static const hal_pin_t board_usart1_tx = { GPIOA, GPIO_Pin_9, GPIO_HAL_MODE_AF_PP };
/** @brief Generic USART1 RX pin (PA10, floating input). */
static const hal_pin_t board_usart1_rx = { GPIOA, GPIO_Pin_10, GPIO_HAL_MODE_IN_FLOATING };
/** @brief Generic USART2 TX pin (PA2, AF push-pull). */
static const hal_pin_t board_usart2_tx = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_AF_PP };
/** @brief Generic USART2 RX pin (PA3, floating input). */
static const hal_pin_t board_usart2_rx = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_IN_FLOATING };

/** @name ESP8266 Wi-Fi / MQTT (USART1). */
#define BOARD_ESP8266_USART USART1
#define BOARD_ESP8266_USART_ID 1U
#define BOARD_ESP8266_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA DMA1_Channel4
#endif
/** @brief ESP8266 USART TX pin (PA9, AF push-pull). */
static const hal_pin_t board_esp8266_tx = { GPIOA, GPIO_Pin_9, GPIO_HAL_MODE_AF_PP };
/** @brief ESP8266 USART RX pin (PA10, floating input). */
static const hal_pin_t board_esp8266_rx = { GPIOA, GPIO_Pin_10, GPIO_HAL_MODE_IN_FLOATING };
/** @brief ESP8266 CH_PD (power-down) control pin (PB0). */
static const hal_pin_t board_esp8266_ch_pd_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
/** @brief ESP8266 reset pin (PB1). */
static const hal_pin_t board_esp8266_rst_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "ZNCZ_001_client_01"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZNCZ_001"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZNCZ_001/web"

/** @name JDY-31 Bluetooth (USART2). */
#define BOARD_JDY31_USART USART2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA DMA1_Channel7
#endif
/** @brief JDY-31 USART TX pin (PA2, AF push-pull). */
static const hal_pin_t board_jdy31_tx = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_AF_PP };
/** @brief JDY-31 USART RX pin (PA3, floating input). */
static const hal_pin_t board_jdy31_rx = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_IN_FLOATING };

/** @brief Default comm backend name: esp8266, jdy31, or nrf24. */
#define BOARD_COMM_DEVICE "esp8266"

/** @name OLED display I2C bus (I2C1, PB6/PB7). */
#if HAL_I2C_USE_SOFT
/** @brief OLED I2C SCL/SDA (bit-bang, open-drain). */
static const hal_pin_t board_oled_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_OUT_OD };
#else
/** @brief OLED I2C SCL/SDA (hardware I2C, AF open-drain). */
static const hal_pin_t board_oled_i2c_scl = { GPIOB, GPIO_Pin_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_oled_i2c_sda = { GPIOB, GPIO_Pin_7, GPIO_HAL_MODE_AF_OD };
#endif

/** @brief OLED I2C peripheral instance (I2C1). */
#define BOARD_OLED_I2C I2C1
/** @brief OLED I2C bus clock in Hz (400 kHz fast mode). */
#define BOARD_OLED_I2C_SPEED 400000U
/** @brief OLED SSD1306 7-bit I2C address (0x3C << 1). */
#define BOARD_OLED_I2C_ADDR 0x78U
/** @brief AFIO remap for OLED I2C pins (none on default PB6/PB7). */
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE

/** @name GPIO sensors and actuators. */
/** @brief DHT11 one-wire data pin (PB12, open-drain). */
static const hal_pin_t board_dht11_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_OUT_OD };
/** @brief DS18B20 one-wire data pin (PB13, open-drain). */
static const hal_pin_t board_ds18b20_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_OUT_OD };
/** @brief Relay drive pin (PB0, push-pull). */
static const hal_pin_t board_relay_pin = { GPIOB, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
/** @brief Status LED pin (PC13, push-pull). */
static const hal_pin_t board_led_pin = { GPIOC, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
/** @brief Buzzer drive pin (PB8, push-pull). */
static const hal_pin_t board_buzzer_pin = { GPIOB, GPIO_Pin_8, GPIO_HAL_MODE_OUT_PP };

/** @name SG90 servo PWM (TIM2 CH1). */
/** @brief SG90 PWM output pin (PA0, AF push-pull). */
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
/** @brief PM2.5 sensor ADC input (PA0, ADC channel 0). */
static const hal_pin_t board_pm25_adc_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_ANALOG };
/** @brief PM2.5 infrared LED control pin (PA12, active low). */
static const hal_pin_t board_pm25_led_pin = { GPIOA, GPIO_Pin_12, GPIO_HAL_MODE_OUT_PP };
/** @brief MQ-2 smoke sensor ADC input (PA1, ADC channel 1). */
static const hal_pin_t board_mq2_adc_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_ANALOG };
/** @brief MQ-7 CO sensor ADC input (PA2, ADC channel 2). */
static const hal_pin_t board_mq7_adc_pin = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_ANALOG };
/** @brief pH sensor ADC input (PA4, ADC channel 4). */
static const hal_pin_t board_ph_adc_pin = { GPIOA, GPIO_Pin_4, GPIO_HAL_MODE_ANALOG };
/** @brief Generic ADC0 input (PA0). */
static const hal_pin_t board_adc0_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_ANALOG };
/** @brief Generic ADC1 input (PA1). */
static const hal_pin_t board_adc1_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_ANALOG };

#define BOARD_ADC_INSTANCE ADC1
#define BOARD_PM25_ADC_CHANNEL ADC_Channel_0
#define BOARD_PM25_LED_ACTIVE_LEVEL 0U
#define BOARD_PM25_ADC_VREF_MV 3300.0f
#define BOARD_PM25_VOUT_SCALE 1.0f
#define BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V 170.0f
#define BOARD_PM25_DENSITY_OFFSET_UG_M3 (-100.0f)
#define BOARD_PM25_DENSITY_MAX_UG_M3 500.0f
#define BOARD_MQ2_ADC_CHANNEL ADC_Channel_1
#define BOARD_MQ7_ADC_CHANNEL ADC_Channel_2
#define BOARD_ADC0_CHANNEL ADC_Channel_0
#define BOARD_ADC1_CHANNEL ADC_Channel_1
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

/** @name Shared sensor I2C bus (OLED, BMP180, MPU6050, MAX30102). */
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

/** @name HC-SR04 ultrasonic ranger. */
/** @brief HC-SR04 trigger output pin (PA3). */
static const hal_pin_t board_hcsr04_trig_pin = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_OUT_PP };
/** @brief HC-SR04 echo input pin (PA4). */
static const hal_pin_t board_hcsr04_echo_pin = { GPIOA, GPIO_Pin_4, GPIO_HAL_MODE_IN_FLOATING };

/** @name HX711 load-cell ADC. */
/** @brief HX711 serial clock pin (PA11). */
static const hal_pin_t board_hx711_sck_pin = { GPIOA, GPIO_Pin_11, GPIO_HAL_MODE_OUT_PP };
/** @brief HX711 data input pin (PA12). */
static const hal_pin_t board_hx711_dt_pin = { GPIOA, GPIO_Pin_12, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_HX711_SCALE 420.0f
#define BOARD_HX711_OFFSET 8388608UL

/** @name DS1302 RTC (bit-bang). */
/** @brief DS1302 chip-enable pin (PB1). */
static const hal_pin_t board_ds1302_ce_pin = { GPIOB, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };
/** @brief DS1302 bidirectional data pin (PB2, open-drain). */
static const hal_pin_t board_ds1302_data_pin = { GPIOB, GPIO_Pin_2, GPIO_HAL_MODE_OUT_OD };
/** @brief DS1302 serial clock pin (PB3). */
static const hal_pin_t board_ds1302_sclk_pin = { GPIOB, GPIO_Pin_3, GPIO_HAL_MODE_OUT_PP };

/** @name Front-panel keys (active low, internal pull-up). */
/** @brief Key 1 input pin (PB4). */
static const hal_pin_t board_key1_pin = { GPIOB, GPIO_Pin_4, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 2 input pin (PB5). */
static const hal_pin_t board_key2_pin = { GPIOB, GPIO_Pin_5, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 3 input pin (PB9). */
static const hal_pin_t board_key3_pin = { GPIOB, GPIO_Pin_9, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 4 input pin (PB13). */
static const hal_pin_t board_key4_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 5 input pin (PB14). */
static const hal_pin_t board_key5_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_IN_PULLUP };

/** @name Four-phase stepper motor outputs. */
/** @brief Stepper phase A (PB12). */
static const hal_pin_t board_stepmotor_a_pin = { GPIOB, GPIO_Pin_12, GPIO_HAL_MODE_OUT_PP };
/** @brief Stepper phase B (PB13). */
static const hal_pin_t board_stepmotor_b_pin = { GPIOB, GPIO_Pin_13, GPIO_HAL_MODE_OUT_PP };
/** @brief Stepper phase C (PB14). */
static const hal_pin_t board_stepmotor_c_pin = { GPIOB, GPIO_Pin_14, GPIO_HAL_MODE_OUT_PP };
/** @brief Stepper phase D (PB15). */
static const hal_pin_t board_stepmotor_d_pin = { GPIOB, GPIO_Pin_15, GPIO_HAL_MODE_OUT_PP };

/** @name 74HC595 shift-register chain #1. */
/** @brief HC595 latch/load pin (PA0). */
static const hal_pin_t board_hc595_1_load_pin = { GPIOA, GPIO_Pin_0, GPIO_HAL_MODE_OUT_PP };
/** @brief HC595 serial clock pin (PA1). */
static const hal_pin_t board_hc595_1_sclk_pin = { GPIOA, GPIO_Pin_1, GPIO_HAL_MODE_OUT_PP };
/** @brief HC595 serial data input pin (PA2). */
static const hal_pin_t board_hc595_1_sdi_pin = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_OUT_PP };

/** @name CO2 sensor (USART2). */
#define BOARD_CO2_USART USART2
#define BOARD_CO2_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_CO2_USART_REMAP GPIO_HAL_REMAP_NONE
/** @brief CO2 sensor USART TX pin (PA2). */
static const hal_pin_t board_co2_tx = { GPIOA, GPIO_Pin_2, GPIO_HAL_MODE_AF_PP };
/** @brief CO2 sensor USART RX pin (PA3). */
static const hal_pin_t board_co2_rx = { GPIOA, GPIO_Pin_3, GPIO_HAL_MODE_IN_FLOATING };

/** @name SU03T voice module (USART3). */
#define BOARD_SU03T_USART USART3
#define BOARD_SU03T_BAUDRATE 9600U
#define BOARD_SU03T_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_SU03T_USART_TX_MODE USART_HAL_TX_MODE_IRQ
/** @brief SU03T USART TX pin (PB10). */
static const hal_pin_t board_su03t_tx = { GPIOB, GPIO_Pin_10, GPIO_HAL_MODE_AF_PP };
/** @brief SU03T USART RX pin (PB11). */
static const hal_pin_t board_su03t_rx = { GPIOB, GPIO_Pin_11, GPIO_HAL_MODE_IN_FLOATING };

/** @name L76K GNSS module (USART3). */
#define BOARD_L76K_USART USART3
#define BOARD_L76K_BAUDRATE 9600U
#define BOARD_L76K_USART_REMAP GPIO_HAL_REMAP_NONE
/** @brief L76K USART TX pin (PB10). */
static const hal_pin_t board_l76k_tx = { GPIOB, GPIO_Pin_10, GPIO_HAL_MODE_AF_PP };
/** @brief L76K USART RX pin (PB11). */
static const hal_pin_t board_l76k_rx = { GPIOB, GPIO_Pin_11, GPIO_HAL_MODE_IN_FLOATING };

/** @name Second SG90 servo PWM (TIM2 CH2). */
/** @brief SG90 #2 PWM output pin (PA1). */
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
