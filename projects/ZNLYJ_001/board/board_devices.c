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

static const one_wire_sensor_config_t board_dht11_config = { board_dht11_pin };
REGISTER_BOARD_DEVICE(TEMP_HUM_SENSOR, "dht11", &board_dht11_config);

#if VERSION_FEATURE_LIGHT && HAL_ADC_ENABLE
static const adc_channel_driver_config_t board_gl5506_config = {
    BOARD_ADC_INSTANCE,
    BOARD_GL5506_ADC_CHANNEL,
    board_gl5506_adc_pin,
    BOARD_GL5506_INVERT
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "gl5506", &board_gl5506_config);
#endif

static const hal_pin_t board_e18_pins[] = {
    board_e18_pir1_pin
};
static const gpio_input_driver_config_t board_e18_config = {
    board_e18_pins,
    0,
    BOARD_E18_PIR_COUNT,
    BOARD_E18_PIR_ACTIVE_LOW
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "presence", &board_e18_config);

static const gpio_probe_driver_config_t board_ir_clothes_config = {
    board_ir_clothes_pin,
    BOARD_IR_CLOTHES_ACTIVE_LOW
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "ir_clothes", &board_ir_clothes_config);

static const stepmotor_driver_config_t board_stepmotor_config = {
    board_stepmotor_a_pin,
    board_stepmotor_b_pin,
    board_stepmotor_c_pin,
    board_stepmotor_d_pin
};
REGISTER_BOARD_DEVICE(STEPPER, "stepmotor", &board_stepmotor_config);

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, 1U };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

#if VERSION_FEATURE_WEIGHT
static const hx711_driver_config_t board_hx711_config = {
    board_hx711_sck_pin,
    board_hx711_dt_pin,
    BOARD_HX711_SCALE,
    BOARD_HX711_OFFSET
};
REGISTER_BOARD_DEVICE(WEIGHT_SENSOR, "hx711", &board_hx711_config);
#endif

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
