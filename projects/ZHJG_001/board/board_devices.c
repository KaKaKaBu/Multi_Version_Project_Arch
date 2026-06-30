#include "board_config.h"
#include "driver_core.h"
#include "driver_configs.h"

static const i2c_device_config_t board_oled_config = {
    BOARD_OLED_I2C,
    BOARD_OLED_I2C_SPEED,
    board_oled_i2c_scl,
    board_oled_i2c_sda,
    BOARD_OLED_I2C_REMAP,
    BOARD_OLED_I2C_ADDR
};
REGISTER_BOARD_DEVICE(DISPLAY, "oled", &board_oled_config);

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

#if HAL_ADC_ENABLE
static const adc_channel_driver_config_t board_mq4_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ4_ADC_CHANNEL,
    board_mq4_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq4_methane", &board_mq4_config);

static const adc_channel_driver_config_t board_water_level_config = {
    BOARD_ADC_INSTANCE,
    BOARD_WATER_LEVEL_ADC_CHANNEL,
    board_water_level_adc_pin,
    BOARD_WATER_LEVEL_INVERT
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "water_level", &board_water_level_config);
#endif

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, 1U };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, 0U };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

static const gpio_input_driver_config_t board_key_config = {
    0,
    KEY_DRIVER_PIN_TABLE,
    KEY_DRIVER_BUTTON_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);

#if VERSION_FEATURE_GPS
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
#endif

#if VERSION_FEATURE_SMS
static const usart_device_config_t board_a7670c_config = {
    BOARD_A7670C_USART,
    BOARD_A7670C_BAUDRATE,
    board_a7670c_tx,
    board_a7670c_rx,
    BOARD_A7670C_USART_REMAP,
    BOARD_A7670C_USART_TX_MODE,
#if HAL_USART_ENABLE_DMA && defined(BOARD_A7670C_USART_TX_DMA)
    BOARD_A7670C_USART_TX_DMA,
#else
    0U,
#endif
    0U
};
REGISTER_BOARD_DEVICE(COMM, "a7670c", &board_a7670c_config);
#endif

#if VERSION_FEATURE_WIFI
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
        BOARD_ESP8266_USART_ID
    },
    board_esp8266_ch_pd_pin,
    board_esp8266_rst_pin,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "esp8266", &board_esp8266_config);
#endif
