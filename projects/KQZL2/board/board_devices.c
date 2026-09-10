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

static const adc_channel_driver_config_t board_mq7_config = {
    BOARD_ADC_INSTANCE,
    BOARD_MQ7_ADC_CHANNEL,
    board_mq7_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(GAS_SENSOR, "mq7_co", &board_mq7_config);
#endif

static const one_wire_sensor_config_t board_dht11_config = { board_dht11_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "dht11", &board_dht11_config);

static const gpio_output_driver_config_t board_relay_config = { board_relay_pin, GPIO_OUTPUT_TRIGGER_LOW };
REGISTER_BOARD_DEVICE(RELAY, "relay", &board_relay_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, BOARD_LED_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, BOARD_BUZZER_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

static const gpio_input_driver_config_t board_key_config = {
    0,
    KEY_DRIVER_PIN_TABLE,
    KEY_DRIVER_BUTTON_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);

#if defined(BOARD_ESP8266_USART)
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

#if defined(BOARD_JDY31_USART)
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
    2U
};
REGISTER_BOARD_DEVICE(COMM, "jdy31", &board_jdy31_config);
#endif
