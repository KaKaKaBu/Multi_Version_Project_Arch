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

#if !VERSION_FEATURE_IR_REMOTE
static const ds1302_driver_config_t board_ds1302_config = {
    board_ds1302_ce_pin,
    board_ds1302_data_pin,
    board_ds1302_sclk_pin
};
REGISTER_BOARD_DEVICE(RTC, "ds1302", &board_ds1302_config);

static const hal_pin_t board_presence_pins[] = {
    board_presence_pin
};
static const gpio_input_driver_config_t board_presence_config = {
    board_presence_pins,
    0,
    1U,
    BOARD_PRESENCE_ACTIVE_LOW
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "presence", &board_presence_config);

#endif

static const gpio_output_driver_config_t board_led_config = { board_led_pin, BOARD_LED_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, BOARD_AQMJ_BUZZER_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

#if VERSION_FEATURE_LIGHT
static const adc_channel_driver_config_t board_light_config = {
    HAL_ADC_ID_1,
    HAL_ADC_CH_0,
    board_light_adc_pin,
    0U
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "gl5506", &board_light_config);

static const gpio_output_driver_config_t board_lamp_config = { board_lamp_pin, BOARD_LAMP_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(RELAY, "relay", &board_lamp_config);
#endif

#if VERSION_FEATURE_MESSAGE
static const isd1820_driver_config_t board_isd1820_config = {
    board_isd1820_rec_pin,
    board_isd1820_play_pin,
    BOARD_ISD1820_TRIGGER_LEVEL
};
REGISTER_BOARD_DEVICE(AUDIO_RECORDER, "isd1820", &board_isd1820_config);
#endif

static const gpio_input_driver_config_t board_key_config = {
    0,
    KEY_DRIVER_PIN_TABLE,
    KEY_DRIVER_BUTTON_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);

#if VERSION_FEATURE_IR_REMOTE
static const ir_remote_driver_config_t board_ir_remote_config = {
    board_ir_remote_pin,
    BOARD_IR_REMOTE_TIMER,
    BOARD_IR_REMOTE_ACTIVE_LOW,
    4U
};
REGISTER_BOARD_DEVICE(INPUT, "ir_remote", &board_ir_remote_config);
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

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
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

#if VERSION_FEATURE_TTS
static const usart_device_config_t board_tts_uart_config = {
    BOARD_TTS_UART_USART,
    BOARD_TTS_UART_BAUDRATE,
    board_tts_uart_tx,
    board_tts_uart_rx,
    BOARD_TTS_UART_USART_REMAP,
    BOARD_TTS_UART_USART_TX_MODE,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "tts_uart", &board_tts_uart_config);
#endif
