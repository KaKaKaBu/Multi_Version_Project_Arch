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

static const i2c_device_config_t board_max30102_config = {
    BOARD_SENSOR_I2C,
    BOARD_SENSOR_I2C_SPEED,
    board_sensor_i2c_scl,
    board_sensor_i2c_sda,
    BOARD_SENSOR_I2C_REMAP,
    BOARD_MAX30102_I2C_ADDR
};
REGISTER_BOARD_DEVICE(PULSE_OXIMETER, "max30102", &board_max30102_config);

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
    (uint8_t)(sizeof(board_key_pins) / sizeof(board_key_pins[0])),
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, 1U };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, 0U };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

#if VERSION_FEATURE_TEMP
static const one_wire_sensor_config_t board_ds18b20_config = { board_ds18b20_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "ds18b20", &board_ds18b20_config);
#endif

#if VERSION_FEATURE_BLOOD_PRESSURE && HAL_ADC_ENABLE
static const msp20_driver_config_t board_msp20_config = {
    {
        BOARD_ADC_INSTANCE,
        BOARD_MSP20_ADC_CHANNEL,
        board_msp20_adc_pin,
        0U
    },
    BOARD_MSP20_VOLT_TO_SYS_K,
    BOARD_MSP20_VOLT_TO_SYS_OFFSET,
    BOARD_MSP20_VOLT_TO_DIA_K,
    BOARD_MSP20_VOLT_TO_DIA_OFFSET,
    BOARD_MSP20_DIA_RATIO
};
REGISTER_BOARD_DEVICE(BLOOD_PRESSURE, "msp20", &board_msp20_config);
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
        0U,
        BOARD_ESP8266_USART_ID
    },
    board_esp8266_ch_pd_pin,
    board_esp8266_rst_pin,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "esp8266", &board_esp8266_config);
#endif

#if VERSION_FEATURE_BLE
static const usart_device_config_t board_jdy31_config = {
    BOARD_JDY31_USART,
    BOARD_JDY31_BAUDRATE,
    board_jdy31_tx,
    board_jdy31_rx,
    BOARD_JDY31_USART_REMAP,
    BOARD_JDY31_USART_TX_MODE,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "jdy31", &board_jdy31_config);
#endif

#if VERSION_FEATURE_VOICE
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
#endif
