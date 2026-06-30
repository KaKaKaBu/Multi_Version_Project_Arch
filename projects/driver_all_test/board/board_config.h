/* driver_all_test 板级配置：为全集成驱动提供默认引脚，便于编译和冒烟测试。 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "gpio_hal.h"
#include "usart_hal.h"

#define BOARD_USART1_BAUDRATE 115200U
#define BOARD_USART2_BAUDRATE 9600U

static const hal_pin_t board_usart1_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_usart1_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_usart2_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_usart2_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_esp8266_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_esp8266_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };
static const hal_pin_t board_jdy31_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_jdy31_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_ESP8266_USART HAL_USART_ID_1
#define BOARD_ESP8266_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_ESP8266_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_ESP8266_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_ESP8266_USART_TX_DMA HAL_DMA_CH_4
#endif
static const hal_pin_t board_esp8266_ch_pd_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_esp8266_rst_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };

#define BOARD_JDY31_USART HAL_USART_ID_2
#define BOARD_JDY31_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_JDY31_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_JDY31_USART_TX_MODE USART_HAL_TX_MODE_IRQ
#if HAL_USART_ENABLE_DMA
#define BOARD_JDY31_USART_TX_DMA HAL_DMA_CH_7
#endif

#define BOARD_COMM_DEVICE "esp8266"

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

static const hal_pin_t board_dht11_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_ds18b20_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_relay_pin = { HAL_PORT_B, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_led_pin = { HAL_PORT_C, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_buzzer_pin = { HAL_PORT_B, HAL_PIN_8, GPIO_HAL_MODE_OUT_PP };

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
static const hal_pin_t board_adc0_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_adc1_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_pm25_adc_pin = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_pm25_led_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ph_adc_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_mq2_adc_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_mq4_adc_pin = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_mq7_adc_pin = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_mq135_adc_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_gl5506_adc_pin = { HAL_PORT_A, HAL_PIN_5, GPIO_HAL_MODE_ANALOG };
static const hal_pin_t board_water_level_adc_pin = { HAL_PORT_A, HAL_PIN_6, GPIO_HAL_MODE_ANALOG };

#define BOARD_ADC_INSTANCE HAL_ADC_ID_1
#define BOARD_ADC0_CHANNEL HAL_ADC_CH_0
#define BOARD_ADC1_CHANNEL HAL_ADC_CH_1
#define BOARD_PM25_ADC_CHANNEL HAL_ADC_CH_2
#define BOARD_MQ2_ADC_CHANNEL HAL_ADC_CH_1
#define BOARD_MQ4_ADC_CHANNEL HAL_ADC_CH_2
#define BOARD_MQ7_ADC_CHANNEL HAL_ADC_CH_3
#define BOARD_MQ135_ADC_CHANNEL HAL_ADC_CH_4
#define BOARD_GL5506_ADC_CHANNEL HAL_ADC_CH_5
#define BOARD_WATER_LEVEL_ADC_CHANNEL HAL_ADC_CH_6
#define BOARD_GL5506_INVERT 0
#define BOARD_WATER_LEVEL_INVERT 0
#define BOARD_PM25_LED_ACTIVE_LEVEL 0U
#define BOARD_PM25_ADC_VREF_MV 3300.0f
#define BOARD_PM25_VOUT_SCALE 1.0f
#define BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V 170.0f
#define BOARD_PM25_DENSITY_OFFSET_UG_M3 (-100.0f)
#define BOARD_PM25_DENSITY_MAX_UG_M3 500.0f
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

static const hal_pin_t board_hcsr04_trig_pin = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hcsr04_echo_pin = { HAL_PORT_A, HAL_PIN_4, GPIO_HAL_MODE_IN_FLOATING };

static const hal_pin_t board_hx711_sck_pin = { HAL_PORT_A, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hx711_dt_pin = { HAL_PORT_A, HAL_PIN_12, GPIO_HAL_MODE_IN_FLOATING };
#define BOARD_HX711_SCALE 420.0f
#define BOARD_HX711_OFFSET 8388608UL

static const hal_pin_t board_ds1302_ce_pin = { HAL_PORT_B, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ds1302_data_pin = { HAL_PORT_B, HAL_PIN_2, GPIO_HAL_MODE_OUT_OD };
static const hal_pin_t board_ds1302_sclk_pin = { HAL_PORT_B, HAL_PIN_3, GPIO_HAL_MODE_OUT_PP };

static const hal_pin_t board_key1_pin = { HAL_PORT_B, HAL_PIN_4, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key2_pin = { HAL_PORT_B, HAL_PIN_5, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key3_pin = { HAL_PORT_B, HAL_PIN_9, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key4_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_IN_PULLUP };
static const hal_pin_t board_key5_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_IN_PULLUP };

static const hal_pin_t board_stepmotor_a_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_b_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_c_pin = { HAL_PORT_B, HAL_PIN_14, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_stepmotor_d_pin = { HAL_PORT_B, HAL_PIN_15, GPIO_HAL_MODE_OUT_PP };

static const hal_pin_t board_hc595_1_load_pin = { HAL_PORT_A, HAL_PIN_0, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hc595_1_sclk_pin = { HAL_PORT_A, HAL_PIN_1, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_hc595_1_sdi_pin = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_OUT_PP };

#define BOARD_CO2_USART HAL_USART_ID_2
#define BOARD_CO2_BAUDRATE BOARD_USART2_BAUDRATE
#define BOARD_CO2_USART_REMAP GPIO_HAL_REMAP_NONE
static const hal_pin_t board_co2_tx = { HAL_PORT_A, HAL_PIN_2, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_co2_rx = { HAL_PORT_A, HAL_PIN_3, GPIO_HAL_MODE_IN_FLOATING };

static const hal_pin_t board_usart3_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_usart3_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_SU03T_USART HAL_USART_ID_3
#define BOARD_SU03T_BAUDRATE 9600U
#define BOARD_SU03T_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_SU03T_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_su03t_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_su03t_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_L76K_USART HAL_USART_ID_3
#define BOARD_L76K_BAUDRATE 9600U
#define BOARD_L76K_USART_REMAP GPIO_HAL_REMAP_NONE
static const hal_pin_t board_l76k_tx = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_l76k_rx = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_A7670C_USART HAL_USART_ID_1
#define BOARD_A7670C_BAUDRATE BOARD_USART1_BAUDRATE
#define BOARD_A7670C_USART_REMAP GPIO_HAL_REMAP_NONE
#define BOARD_A7670C_USART_TX_MODE USART_HAL_TX_MODE_IRQ
static const hal_pin_t board_a7670c_tx = { HAL_PORT_A, HAL_PIN_9, GPIO_HAL_MODE_AF_PP };
static const hal_pin_t board_a7670c_rx = { HAL_PORT_A, HAL_PIN_10, GPIO_HAL_MODE_IN_FLOATING };

#define BOARD_SG90_2
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

#define BOARD_ILI9341_SPI_BUS_ID 0U
#define BOARD_ST7789_SPI_BUS_ID 1U
static const hal_pin_t board_lcd_dc_pin = { HAL_PORT_B, HAL_PIN_10, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_lcd_rst_pin = { HAL_PORT_B, HAL_PIN_11, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_ili9341_cs_pin = { HAL_PORT_B, HAL_PIN_12, GPIO_HAL_MODE_OUT_PP };
static const hal_pin_t board_st7789_cs_pin = { HAL_PORT_B, HAL_PIN_13, GPIO_HAL_MODE_OUT_PP };
#endif

#define BOARD_KEY_COUNT 5U

#define BOARD_HAS_DHT11 1
#define BOARD_HAS_DS18B20 1
#define BOARD_HAS_RELAY 1
#define BOARD_HAS_LED 1
#define BOARD_HAS_BUZZER 1
#define BOARD_HAS_HCSR04 1
#define BOARD_HAS_HX711 1
#define BOARD_HAS_DS1302 1
#define BOARD_HAS_STEPMOTOR 1
#define BOARD_HAS_HC595_1 1
#if HAL_SPI_USE_HW
#define BOARD_HAS_RC522 1
#define BOARD_HAS_NRF24 1
#endif

#endif
