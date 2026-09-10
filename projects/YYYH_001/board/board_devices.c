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

static const ds1302_driver_config_t board_ds1302_config = {
    board_ds1302_ce_pin,
    board_ds1302_data_pin,
    board_ds1302_sclk_pin
};
REGISTER_BOARD_DEVICE(RTC, "ds1302", &board_ds1302_config);

static const hal_pin_t board_take_sensor_pins[] = {
    board_take_sensor_pin
};
static const gpio_input_driver_config_t board_take_sensor_config = {
    board_take_sensor_pins,
    0,
    1U,
    BOARD_TAKE_SENSOR_ACTIVE_LOW
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "presence", &board_take_sensor_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, BOARD_LED_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, BOARD_YYYH_BUZZER_TRIGGER_LEVEL };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

#if VERSION_FEATURE_SERVO
static const servo_driver_config_t board_sg90_config = {
    BOARD_SG90_TIM,
    BOARD_SG90_TIM_CHANNEL,
    BOARD_SG90_TIM_PRESCALER,
    BOARD_SG90_TIM_PERIOD,
    board_sg90_pwm_pin,
    BOARD_SG90_TIM_REMAP
};
REGISTER_BOARD_DEVICE(SERVO, "sg90", &board_sg90_config);
#endif

#if VERSION_FEATURE_WEIGHT
static const hx711_driver_config_t board_hx711_config = {
    board_hx711_sck_pin,
    board_hx711_dt_pin,
    BOARD_HX711_SCALE,
    BOARD_HX711_OFFSET
};
REGISTER_BOARD_DEVICE(WEIGHT_SENSOR, "hx711", &board_hx711_config);
#endif

#if VERSION_FEATURE_DHT11
static const one_wire_sensor_config_t board_dht11_config = { board_dht11_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "dht11", &board_dht11_config);
#endif

static const gpio_input_driver_config_t board_key_config = {
    0,
    KEY_DRIVER_PIN_TABLE,
    KEY_DRIVER_BUTTON_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);

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
    2U
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

#if VERSION_FEATURE_VOICE
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
