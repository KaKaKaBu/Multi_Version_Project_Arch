/**
 * @file board_config_template.h
 * @brief Default board pin map, peripheral instances, and HAL feature overrides.
 *
 * Copy to projects/<name>/board/board_config.h and customize per product.
 * HAL toggles may also be set in CMake via hal_options.cmake
 * (HAL_I2C_USE_SOFT, HAL_SPI_*, HAL_USART_ENABLE_DMA,
 * HAL_ADC_*, HAL_DEBUG_UART_ENABLE). When HAL_I2C_USE_SOFT=1, set SCL/SDA to OUT_OD;
 * otherwise use AF_OD for hardware I2C.
 */

#ifndef BOARD_CONFIG_TEMPLATE_H
#define BOARD_CONFIG_TEMPLATE_H

#include "gpio_hal.h"
#include "spi_hal.h"
#include "usart_hal.h"

/** @brief HAL_USART_ID_1 baud rate (ESP8266 default). */
#define BOARD_USART1_BAUDRATE 115200U
/** @brief HAL_USART_ID_2 baud rate (JDY-31 / CO2 default). */
#define BOARD_USART2_BAUDRATE 9600U

#if HAL_DEBUG_UART_ENABLE
#define BOARD_DEBUG_UART_BAUDRATE 9600U
#define BOARD_DEBUG_UART_PASSTHROUGH_USART3 0
static const hal_pin_t board_debug_uart_tx = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

/** @brief Generic HAL_USART_ID_1 TX pin (PA9, AF push-pull). */
static const hal_pin_t board_usart1_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
/** @brief Generic HAL_USART_ID_1 RX pin (PA10, floating input). */
static const hal_pin_t board_usart1_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
/** @brief Generic HAL_USART_ID_2 TX pin (PA2, AF push-pull). */
static const hal_pin_t board_usart2_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
/** @brief Generic HAL_USART_ID_2 RX pin (PA3, floating input). */
static const hal_pin_t board_usart2_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };

/** @name ESP8266 Wi-Fi / MQTT (HAL_USART_ID_1). */
#define BOARD_ESP8266_USART HAL_USART_ID_1
#define BOARD_ESP8266_USART_ID 1U
#define BOARD_ESP8266_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA HAL_DMA_CH_4
#endif
/** @brief ESP8266 USART TX pin (PA9, AF push-pull). */
static const hal_pin_t board_esp8266_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
/** @brief ESP8266 USART RX pin (PA10, floating input). */
static const hal_pin_t board_esp8266_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
/** @brief ESP8266 CH_PD (power-down) control pin (PB0). */
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
/** @brief ESP8266 reset pin (PB1). */
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_ESP8266_WIFI_SSID "demo"
#define BOARD_ESP8266_WIFI_PASS "12345678"
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "ZNCZ_001_client_01"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZNCZ_001"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZNCZ_001/web"

/** @name JDY-31 Bluetooth (HAL_USART_ID_2). */
#define BOARD_JDY31_USART HAL_USART_ID_2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA HAL_DMA_CH_7
#endif
/** @brief JDY-31 USART TX pin (PA2, AF push-pull). */
static const hal_pin_t board_jdy31_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
/** @brief JDY-31 USART RX pin (PA3, floating input). */
static const hal_pin_t board_jdy31_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };

/** @brief Default comm backend name: esp8266, jdy31, or nrf24. */
#define BOARD_COMM_DEVICE "esp8266"

/** @name OLED display I2C bus (HAL_I2C_ID_1, PB6/PB7). */
#if HAL_I2C_USE_SOFT
/** @brief OLED I2C SCL/SDA (bit-bang, open-drain). */
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_OUT_OD };
#else
/** @brief OLED I2C SCL/SDA (hardware I2C, AF open-drain). */
static const hal_pin_t board_oled_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_oled_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_AF_OD };
#endif

/** @brief OLED I2C peripheral instance (HAL_I2C_ID_1). */
#define BOARD_OLED_I2C HAL_I2C_ID_1
/** @brief OLED I2C bus clock in Hz (400 kHz fast mode). */
#define BOARD_OLED_I2C_SPEED 400000U
/** @brief OLED SSD1306 7-bit I2C address (0x3C << 1). */
#define BOARD_OLED_I2C_ADDR 0x78U
/** @brief AFIO remap for OLED I2C pins (none on default PB6/PB7). */
#define BOARD_OLED_I2C_REMAP GPIO_HAL_REMAP_NONE

/** @name GPIO sensors and actuators. */
/** @brief DHT11 one-wire data pin (PB12, open-drain). */
static const hal_pin_t board_dht11_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_OD };
/** @brief DS18B20 one-wire data pin (PB13, open-drain). */
static const hal_pin_t board_ds18b20_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_OUT_OD };
/** @brief Relay drive pin (PB0, push-pull). */
static const hal_pin_t board_relay_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
/** @brief Status LED pin (PC13, push-pull). */
static const hal_pin_t board_led_pin = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
/** @brief Buzzer drive pin (PB8, push-pull). */
static const hal_pin_t board_buzzer_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };

/** @name SG90 servo PWM (HAL_TIMER_ID_2 CH1). */
/** @brief SG90 PWM output pin (PA0, AF push-pull). */
static const hal_pin_t board_sg90_pwm_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_AF_PP };

#define BOARD_SG90_TIM HAL_TIMER_ID_2
#define BOARD_SG90_TIM_CHANNEL 1U
#define BOARD_SG90_TIM_PRESCALER (72U - 1U)
#define BOARD_SG90_TIM_PERIOD (20000U - 1U)
#define BOARD_SG90_TIM_REMAP GPIO_HAL_REMAP_NONE

#if HAL_SPI_USE_SOFT
static const hal_pin_t board_spi_sck = { HAL_PORT_A, HAL_PIN_5, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_spi_mosi = { HAL_PORT_A, HAL_PIN_7, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_spi_miso = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_IN_FLOATING };
#endif

#if HAL_SPI_USE_HW
static const hal_pin_t board_hw_spi1_sck = { HAL_PORT_A, HAL_PIN_5, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_hw_spi1_mosi = { HAL_PORT_A, HAL_PIN_7, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_hw_spi1_miso = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_HW_SPI1_INSTANCE HAL_SPI_ID_1
#define BOARD_HW_SPI1_PRESCALER SPI_HAL_PRESCALER_8
#define BOARD_HW_SPI1_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_HW_SPI1_CPOL 0U
#define BOARD_HW_SPI1_CPHA 0U
#if HAL_SPI_ENABLE_DMA
#define BOARD_HW_SPI1_TX_DMA HAL_DMA_CH_3
#define BOARD_HW_SPI1_RX_DMA HAL_DMA_CH_2
#endif
#endif

#if HAL_ADC_ENABLE
/** @brief PM2.5 sensor ADC input (PA0, ADC channel 0). */
static const hal_pin_t board_pm25_adc_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };
/** @brief PM2.5 infrared LED control pin (PA12, active low). */
static const hal_pin_t board_pm25_led_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
/** @brief MQ-2 smoke sensor ADC input (PA1, ADC channel 1). */
static const hal_pin_t board_mq2_adc_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };
/** @brief MQ-7 CO sensor ADC input (PA2, ADC channel 2). */
static const hal_pin_t board_mq7_adc_pin = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_ANALOG };
/** @brief pH sensor ADC input (PA4, ADC channel 4). */
static const hal_pin_t board_ph_adc_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_ANALOG };
/** @brief Generic ADC0 input (PA0). */
static const hal_pin_t board_adc0_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };
/** @brief Generic HAL_ADC_ID_1 input (PA1). */
static const hal_pin_t board_adc1_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };

#define BOARD_ADC_INSTANCE HAL_ADC_ID_1
#define BOARD_PM25_ADC_CHANNEL HAL_ADC_CH_0
#define BOARD_PM25_LED_ACTIVE_LEVEL 0U
#define BOARD_PM25_ADC_VREF_MV 3300.0f
#define BOARD_PM25_VOUT_SCALE 1.0f
#define BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V 170.0f
#define BOARD_PM25_DENSITY_OFFSET_UG_M3 (-100.0f)
#define BOARD_PM25_DENSITY_MAX_UG_M3 500.0f
#define BOARD_MQ2_ADC_CHANNEL HAL_ADC_CH_1
#define BOARD_MQ7_ADC_CHANNEL HAL_ADC_CH_2
#define BOARD_ADC0_CHANNEL HAL_ADC_CH_0
#define BOARD_ADC1_CHANNEL HAL_ADC_CH_1
#define BOARD_PH_ADC_CHANNEL HAL_ADC_CH_4
#define BOARD_PH_VOLTAGE_COEFF (-5.6342f)
#define BOARD_PH_VOLTAGE_OFFSET 16.413f
#if HAL_ADC_ENABLE_DMA
#define BOARD_ADC_DMA HAL_DMA_CH_1
#define BOARD_ADC_DMA_CHANNEL_COUNT 2U
static const uint8_t board_adc_dma_channels[BOARD_ADC_DMA_CHANNEL_COUNT] = {
    HAL_ADC_CH_0,
    HAL_ADC_CH_1
};
#endif
#endif

/** @name Shared sensor I2C bus (OLED, BMP180, MPU6050, MAX30102). */
#if HAL_I2C_USE_SOFT
static const hal_pin_t board_sensor_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_sensor_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_OUT_OD };
#else
static const hal_pin_t board_sensor_i2c_scl = { HAL_PORT_B, HAL_PIN_6, GPIO_HAL_MODE_AF_OD };
static const hal_pin_t board_sensor_i2c_sda = { HAL_PORT_B, HAL_PIN_7, GPIO_HAL_MODE_AF_OD };
#endif
#define BOARD_SENSOR_I2C HAL_I2C_ID_1
#define BOARD_SENSOR_I2C_SPEED 400000U
#define BOARD_SENSOR_I2C_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_BMP180_I2C_ADDR 0xEEU
#define BOARD_MPU6050_I2C_ADDR 0xD0U
#define BOARD_MAX30102_I2C_ADDR 0xAEU
#define BOARD_BMP180_PRESSURE_REVISE 526

/** @name HC-SR04 ultrasonic ranger. */
/** @brief HC-SR04 trigger output pin (PA3). */
static const hal_pin_t board_hcsr04_trig_pin = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_OUT_PP };
/** @brief HC-SR04 echo input pin (PA4). */
static const hal_pin_t board_hcsr04_echo_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_IN_FLOATING };

/** @name HX711 load-cell ADC. */
/** @brief HX711 serial clock pin (PA11). */
static const hal_pin_t board_hx711_sck_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
/** @brief HX711 data input pin (PA12). */
static const hal_pin_t board_hx711_dt_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_HX711_SCALE 420.0f
#define BOARD_HX711_OFFSET 8388608UL

/** @name DS1302 RTC (bit-bang). */
/** @brief DS1302 chip-enable pin (PB1). */
static const hal_pin_t board_ds1302_ce_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };
/** @brief DS1302 bidirectional data pin (PB2, open-drain). */
static const hal_pin_t board_ds1302_data_pin = { HAL_PORT_B, HAL_PIN_2, GPIO_HAL_MODE_OUT_OD };
/** @brief DS1302 serial clock pin (PB3). */
static const hal_pin_t board_ds1302_sclk_pin = { HAL_PORT_B, HAL_PIN_3, GPIO_HAL_MODE_OUT_PP };

/** @name Front-panel keys (active low, internal pull-up). */
/** @brief Key 1 input pin (PB4). */
static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 2 input pin (PB5). */
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 3 input pin (PB9). */
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 4 input pin (PB13). */
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
/** @brief Key 5 input pin (PB14). */
static const hal_pin_t board_key5_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_IN_PULLUP };

/** @name Four-phase stepper motor outputs. */
/** @brief Stepper phase A (PB12). */
static const hal_pin_t board_stepmotor_a_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
/** @brief Stepper phase B (PB13). */
static const hal_pin_t board_stepmotor_b_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
/** @brief Stepper phase C (PB14). */
static const hal_pin_t board_stepmotor_c_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_OUT_PP };
/** @brief Stepper phase D (PB15). */
static const hal_pin_t board_stepmotor_d_pin = { HAL_PORT_B, HAL_PIN_15, GPIO_HAL_MODE_OUT_PP };

/** @name 74HC595 shift-register chain #1. */
/** @brief HC595 latch/load pin (PA0). */
static const hal_pin_t board_hc595_1_load_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
/** @brief HC595 serial clock pin (PA1). */
static const hal_pin_t board_hc595_1_sclk_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };
/** @brief HC595 serial data input pin (PA2). */
static const hal_pin_t board_hc595_1_sdi_pin = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_OUT_PP };

/** @name CO2 sensor (HAL_USART_ID_2). */
#define BOARD_CO2_USART HAL_USART_ID_2
#define BOARD_CO2_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_CO2_USART_REMAP GPIO_HAL_REMAP_NONE
/** @brief CO2 sensor USART TX pin (PA2). */
static const hal_pin_t board_co2_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
/** @brief CO2 sensor USART RX pin (PA3). */
static const hal_pin_t board_co2_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };

/** @name SU03T voice module (HAL_USART_ID_3). */
#define BOARD_SU03T_USART HAL_USART_ID_3
#define BOARD_SU03T_BAUDRATE 9600U
#define BOARD_SU03T_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_SU03T_USART_TX_MODE USART_HAL_TX_MODE_IRQ
/** @brief SU03T USART TX pin (PB10). */
static const hal_pin_t board_su03t_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
/** @brief SU03T USART RX pin (PB11). */
static const hal_pin_t board_su03t_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };

/** @name L76K GNSS module (HAL_USART_ID_3). */
#define BOARD_L76K_USART HAL_USART_ID_3
#define BOARD_L76K_BAUDRATE 9600U
#define BOARD_L76K_USART_REMAP GPIO_HAL_REMAP_NONE
/** @brief L76K USART TX pin (PB10). */
static const hal_pin_t board_l76k_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
/** @brief L76K USART RX pin (PB11). */
static const hal_pin_t board_l76k_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };

/** @name Second SG90 servo PWM (HAL_TIMER_ID_2 CH2). */
/** @brief SG90 #2 PWM output pin (PA1). */
static const hal_pin_t board_sg90_2_pwm_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_AF_PP };
#define BOARD_SG90_2_TIM HAL_TIMER_ID_2
#define BOARD_SG90_2_TIM_CHANNEL 2U

#if HAL_SPI_USE_HW
#define BOARD_RC522_SPI_BUS_ID 0U
static const hal_pin_t board_rc522_cs_pin = { HAL_PORT_A, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_rc522_rst_pin = { HAL_PORT_A, HAL_PIN_7, GPIO_HAL_MODE_OUT_PP };

#define BOARD_NRF24_SPI_BUS_ID 1U
static const hal_pin_t board_nrf24_ce_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_nrf24_csn_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
#endif

#endif
