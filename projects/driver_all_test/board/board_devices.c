#include "board_config.h"
#include "driver_core.h"
#include "driver_configs.h"

static const esp8266_driver_config_t board_esp8266_config = {
    {
        BOARD_ESP8266_USART,
        BOARD_ESP8266_BAUDRATE,
        board_esp8266_tx,
        board_esp8266_rx,
        BOARD_ESP8266_USART_REMAP,
        BOARD_ESP8266_USART_TX_MODE,
#if HAL_USART_ENABLE_DMA && defined(BOARD_ESP8266_USART_TX_DMA)
        BOARD_ESP8266_USART_TX_DMA,
#else
        0U,
#endif
        0U
    },
    board_esp8266_ch_pd_pin,
    board_esp8266_rst_pin,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "esp8266", &board_esp8266_config);

static const usart_device_config_t board_jdy31_config = {
    BOARD_JDY31_USART,
    BOARD_JDY31_BAUDRATE,
    board_jdy31_tx,
    board_jdy31_rx,
    BOARD_JDY31_USART_REMAP,
    BOARD_JDY31_USART_TX_MODE,
#if HAL_USART_ENABLE_DMA && defined(BOARD_JDY31_USART_TX_DMA)
    BOARD_JDY31_USART_TX_DMA,
#else
    0U,
#endif
    0U
};
REGISTER_BOARD_DEVICE(COMM, "jdy31", &board_jdy31_config);

static const i2c_device_config_t board_oled_config = {
    BOARD_OLED_I2C,
    BOARD_OLED_I2C_SPEED,
    board_oled_i2c_scl,
    board_oled_i2c_sda,
    BOARD_OLED_I2C_REMAP,
    BOARD_OLED_I2C_ADDR
};
REGISTER_BOARD_DEVICE(DISPLAY, "oled", &board_oled_config);

#if HAL_SPI_USE_HW
static const spi_lcd_driver_config_t board_ili9341_config = {
    {
        BOARD_ILI9341_SPI_BUS_ID,
        BOARD_HW_SPI1_INSTANCE,
        board_hw_spi1_sck,
        board_hw_spi1_mosi,
        board_hw_spi1_miso,
        BOARD_HW_SPI1_REMAP,
        BOARD_HW_SPI1_PRESCALER,
        BOARD_HW_SPI1_CPOL,
        BOARD_HW_SPI1_CPHA,
#if HAL_SPI_ENABLE_DMA && defined(BOARD_HW_SPI1_TX_DMA)
        BOARD_HW_SPI1_TX_DMA,
        BOARD_HW_SPI1_RX_DMA,
#else
        0U,
        0U,
#endif
        board_ili9341_cs_pin,
        board_lcd_rst_pin
    },
    board_lcd_dc_pin,
    board_lcd_rst_pin,
    240U,
    320U,
    0U,
    0U,
    0U,
    0U,
    0x0000U,
    0xFFFFU,
    0
};
REGISTER_BOARD_DEVICE(DISPLAY, "ili9341", &board_ili9341_config);

static const spi_lcd_driver_config_t board_st7789_config = {
    {
        BOARD_ST7789_SPI_BUS_ID,
        BOARD_HW_SPI1_INSTANCE,
        board_hw_spi1_sck,
        board_hw_spi1_mosi,
        board_hw_spi1_miso,
        BOARD_HW_SPI1_REMAP,
        BOARD_HW_SPI1_PRESCALER,
        BOARD_HW_SPI1_CPOL,
        BOARD_HW_SPI1_CPHA,
#if HAL_SPI_ENABLE_DMA && defined(BOARD_HW_SPI1_TX_DMA)
        BOARD_HW_SPI1_TX_DMA,
        BOARD_HW_SPI1_RX_DMA,
#else
        0U,
        0U,
#endif
        board_st7789_cs_pin,
        board_lcd_rst_pin
    },
    board_lcd_dc_pin,
    board_lcd_rst_pin,
    240U,
    240U,
    0U,
    80U,
    0U,
    1U,
    0x0000U,
    0xFFFFU,
    0
};
REGISTER_BOARD_DEVICE(DISPLAY, "st7789", &board_st7789_config);
#endif

static const one_wire_sensor_config_t board_dht11_config = { board_dht11_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "dht11", &board_dht11_config);

static const one_wire_sensor_config_t board_ds18b20_config = { board_ds18b20_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "ds18b20", &board_ds18b20_config);

static const gpio_output_driver_config_t board_relay_config = { board_relay_pin, 1U };
REGISTER_BOARD_DEVICE(RELAY, "relay", &board_relay_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, 0U };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, 1U };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

static const servo_driver_config_t board_sg90_config = {
    BOARD_SG90_TIM,
    BOARD_SG90_TIM_CHANNEL,
    BOARD_SG90_TIM_PRESCALER,
    BOARD_SG90_TIM_PERIOD,
    board_sg90_pwm_pin,
    BOARD_SG90_TIM_REMAP
};
REGISTER_BOARD_DEVICE(SERVO, "sg90", &board_sg90_config);

#if HAL_ADC_ENABLE
static const pm25_driver_config_t board_pm25_config = {
    {
        BOARD_ADC_INSTANCE,
        BOARD_PM25_ADC_CHANNEL,
        board_pm25_adc_pin,
        0U
    },
    board_pm25_led_pin,
    BOARD_PM25_LED_ACTIVE_LEVEL,
    BOARD_PM25_ADC_VREF_MV,
    BOARD_PM25_VOUT_SCALE,
    BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V,
    BOARD_PM25_DENSITY_OFFSET_UG_M3,
    BOARD_PM25_DENSITY_MAX_UG_M3
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "pm25", &board_pm25_config);

static const adc_channel_driver_config_t board_mq2_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ2_ADC_CHANNEL,
    board_mq2_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq2_smoke", &board_mq2_config);

static const adc_channel_driver_config_t board_mq4_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ4_ADC_CHANNEL,
    board_mq4_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq4_methane", &board_mq4_config);

static const adc_channel_driver_config_t board_mq7_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ7_ADC_CHANNEL,
    board_mq7_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq7_co", &board_mq7_config);

static const adc_channel_driver_config_t board_mq135_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ135_ADC_CHANNEL,
    board_mq135_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq135", &board_mq135_config);

static const adc_channel_driver_config_t board_gl5506_config = {
    BOARD_ADC_INSTANCE,
    BOARD_GL5506_ADC_CHANNEL,
    board_gl5506_adc_pin,
    BOARD_GL5506_INVERT
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "gl5506", &board_gl5506_config);

static const adc_channel_driver_config_t board_water_level_config = {
    BOARD_ADC_INSTANCE,
    BOARD_WATER_LEVEL_ADC_CHANNEL,
    board_water_level_adc_pin,
    BOARD_WATER_LEVEL_INVERT
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "water_level", &board_water_level_config);

static const ph_driver_config_t board_ph_config = {
    {
        BOARD_ADC_INSTANCE,
        BOARD_PH_ADC_CHANNEL,
        board_ph_adc_pin,
        0U
    },
    BOARD_PH_VOLTAGE_COEFF,
    BOARD_PH_VOLTAGE_OFFSET
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "ph_sensor", &board_ph_config);
#endif

static const i2c_device_config_t board_bmp180_config_i2c = {
    BOARD_SENSOR_I2C,
    BOARD_SENSOR_I2C_SPEED,
    board_sensor_i2c_scl,
    board_sensor_i2c_sda,
    BOARD_SENSOR_I2C_REMAP,
    BOARD_BMP180_I2C_ADDR
};
static const bmp180_driver_config_t board_bmp180_config = {
    board_bmp180_config_i2c,
    BOARD_BMP180_PRESSURE_REVISE
};
REGISTER_BOARD_DEVICE(PRESSURE_SENSOR, "bmp180", &board_bmp180_config);

static const mpu6050_driver_config_t board_mpu6050_config = {
    {
        BOARD_SENSOR_I2C,
        BOARD_SENSOR_I2C_SPEED,
        board_sensor_i2c_scl,
        board_sensor_i2c_sda,
        BOARD_SENSOR_I2C_REMAP,
        BOARD_MPU6050_I2C_ADDR
    },
    0,
    0U,
    0x18U,
    0x18U
};
REGISTER_BOARD_DEVICE(IMU_SENSOR, "mpu6050", &board_mpu6050_config);

static const i2c_device_config_t board_max30102_config = {
    BOARD_SENSOR_I2C,
    BOARD_SENSOR_I2C_SPEED,
    board_sensor_i2c_scl,
    board_sensor_i2c_sda,
    BOARD_SENSOR_I2C_REMAP,
    BOARD_MAX30102_I2C_ADDR
};
REGISTER_BOARD_DEVICE(PULSE_OXIMETER, "max30102", &board_max30102_config);

static const hcsr04_driver_config_t board_hcsr04_config = {
    board_hcsr04_trig_pin,
    board_hcsr04_echo_pin
};
REGISTER_BOARD_DEVICE(DISTANCE_SENSOR, "hcsr04", &board_hcsr04_config);

static const hx711_driver_config_t board_hx711_config = {
    board_hx711_sck_pin,
    board_hx711_dt_pin,
    BOARD_HX711_SCALE,
    BOARD_HX711_OFFSET
};
REGISTER_BOARD_DEVICE(WEIGHT_SENSOR, "hx711", &board_hx711_config);

static const ds1302_driver_config_t board_ds1302_config = {
    board_ds1302_ce_pin,
    board_ds1302_data_pin,
    board_ds1302_sclk_pin
};
REGISTER_BOARD_DEVICE(RTC, "ds1302", &board_ds1302_config);

static const hal_pin_t board_key_pins[] = {
    board_key1_pin,
    board_key2_pin,
    board_key3_pin,
    board_key4_pin,
    board_key5_pin
};
static const gpio_input_driver_config_t board_key_config = {
    board_key_pins,
    0,
    BOARD_KEY_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);

static const stepmotor_driver_config_t board_stepmotor_config = {
    board_stepmotor_a_pin,
    board_stepmotor_b_pin,
    board_stepmotor_c_pin,
    board_stepmotor_d_pin
};
REGISTER_BOARD_DEVICE(STEPPER, "stepmotor", &board_stepmotor_config);

static const hc595_driver_config_t board_hc595_1_config = {
    board_hc595_1_load_pin,
    board_hc595_1_sclk_pin,
    board_hc595_1_sdi_pin
};
REGISTER_BOARD_DEVICE(SEGMENT_DISPLAY, "hc595_1", &board_hc595_1_config);

static const usart_device_config_t board_co2_config = {
    BOARD_CO2_USART,
    BOARD_CO2_BAUDRATE,
    board_co2_tx,
    board_co2_rx,
    BOARD_CO2_USART_REMAP,
    USART_HAL_TX_MODE_IRQ,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "co2", &board_co2_config);

static const usart_device_config_t board_su03t_config = {
    BOARD_SU03T_USART,
    BOARD_SU03T_BAUDRATE,
    board_su03t_tx,
    board_su03t_rx,
    BOARD_SU03T_USART_REMAP,
    BOARD_SU03T_USART_TX_MODE,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "su03t", &board_su03t_config);

static const usart_device_config_t board_l76k_config = {
    BOARD_L76K_USART,
    BOARD_L76K_BAUDRATE,
    board_l76k_tx,
    board_l76k_rx,
    BOARD_L76K_USART_REMAP,
    USART_HAL_TX_MODE_IRQ,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(GNSS, "l76k", &board_l76k_config);

static const usart_device_config_t board_a7670c_config = {
    BOARD_A7670C_USART,
    BOARD_A7670C_BAUDRATE,
    board_a7670c_tx,
    board_a7670c_rx,
    BOARD_A7670C_USART_REMAP,
    BOARD_A7670C_USART_TX_MODE,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "a7670c", &board_a7670c_config);

static const servo_driver_config_t board_sg90_2_config = {
    BOARD_SG90_2_TIM,
    BOARD_SG90_2_TIM_CHANNEL,
    BOARD_SG90_TIM_PRESCALER,
    BOARD_SG90_TIM_PERIOD,
    board_sg90_2_pwm_pin,
    BOARD_SG90_TIM_REMAP
};
REGISTER_BOARD_DEVICE(SERVO, "sg90_2", &board_sg90_2_config);

#if HAL_SPI_USE_HW
static const spi_device_config_t board_rc522_config = {
    BOARD_RC522_SPI_BUS_ID,
    BOARD_HW_SPI1_INSTANCE,
    board_hw_spi1_sck,
    board_hw_spi1_mosi,
    board_hw_spi1_miso,
    BOARD_HW_SPI1_REMAP,
    BOARD_HW_SPI1_PRESCALER,
    BOARD_HW_SPI1_CPOL,
    BOARD_HW_SPI1_CPHA,
#if HAL_SPI_ENABLE_DMA && defined(BOARD_HW_SPI1_TX_DMA)
    BOARD_HW_SPI1_TX_DMA,
    BOARD_HW_SPI1_RX_DMA,
#else
    0U,
    0U,
#endif
    board_rc522_cs_pin,
    board_rc522_rst_pin
};
REGISTER_BOARD_DEVICE(RFID, "rc522", &board_rc522_config);

static const spi_device_config_t board_nrf24_config = {
    BOARD_NRF24_SPI_BUS_ID,
    BOARD_HW_SPI1_INSTANCE,
    board_hw_spi1_sck,
    board_hw_spi1_mosi,
    board_hw_spi1_miso,
    BOARD_HW_SPI1_REMAP,
    BOARD_HW_SPI1_PRESCALER,
    BOARD_HW_SPI1_CPOL,
    BOARD_HW_SPI1_CPHA,
#if HAL_SPI_ENABLE_DMA && defined(BOARD_HW_SPI1_TX_DMA)
    BOARD_HW_SPI1_TX_DMA,
    BOARD_HW_SPI1_RX_DMA,
#else
    0U,
    0U,
#endif
    board_nrf24_csn_pin,
    board_nrf24_ce_pin
};
REGISTER_BOARD_DEVICE(COMM, "nrf24", &board_nrf24_config);
#endif
