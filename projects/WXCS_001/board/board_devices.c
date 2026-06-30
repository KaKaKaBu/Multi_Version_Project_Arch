#include "board_config.h"
#include "driver_core.h"
#include "driver_configs.h"

#if !defined(PLATFORM_MCS51)

static const i2c_device_config_t board_oled_config = {
    BOARD_OLED_I2C,
    BOARD_OLED_I2C_SPEED,
    board_oled_i2c_scl,
    board_oled_i2c_sda,
    BOARD_OLED_I2C_REMAP,
    BOARD_OLED_I2C_ADDR
};
REGISTER_BOARD_DEVICE(DISPLAY, "oled", &board_oled_config);

#if HAL_ADC_ENABLE
static const adc_channel_driver_config_t board_mq135_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ135_ADC_CHANNEL,
    board_mq135_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq135", &board_mq135_config);

static const adc_channel_driver_config_t board_mq7_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ7_ADC_CHANNEL,
    board_mq7_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq7_co", &board_mq7_config);
#endif

static const one_wire_sensor_config_t board_ds18b20_config = { board_ds18b20_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "ds18b20", &board_ds18b20_config);

static const gpio_output_driver_config_t board_relay_config = { board_relay_pin, 1U };
REGISTER_BOARD_DEVICE(RELAY, "relay", &board_relay_config);

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
#if defined(BOARD_ESP8266_DEBUG_TRACE_ENABLE)
    BOARD_ESP8266_DEBUG_TRACE_ENABLE
#else
    0U
#endif
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
#if HAL_USART_ENABLE_DMA && defined(BOARD_JDY31_USART_TX_DMA)
    BOARD_JDY31_USART_TX_DMA,
#else
    0U,
#endif
    0U
};
REGISTER_BOARD_DEVICE(COMM, "jdy31", &board_jdy31_config);
#endif

#endif
