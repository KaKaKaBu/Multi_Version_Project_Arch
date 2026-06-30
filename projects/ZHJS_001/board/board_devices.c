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
static const adc_channel_driver_config_t board_gl5506_config = {
    BOARD_ADC_INSTANCE,
    BOARD_GL5506_ADC_CHANNEL,
    board_gl5506_adc_pin,
    BOARD_GL5506_INVERT
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "gl5506", &board_gl5506_config);
#endif

static const hal_pin_t board_e18_pins[] = {
    board_e18_pir1_pin,
    board_e18_pir2_pin,
    board_e18_pir3_pin
};
static const gpio_input_driver_config_t board_e18_config = {
    board_e18_pins,
    0,
    BOARD_E18_PIR_COUNT,
    BOARD_E18_PIR_ACTIVE_LOW
};
REGISTER_BOARD_DEVICE(ANALOG_PROBE, "presence", &board_e18_config);

static const gpio_output_driver_config_t board_light1_config = { board_light1_pin, 1U };
REGISTER_BOARD_DEVICE(RELAY, "light1", &board_light1_config);

static const gpio_output_driver_config_t board_light2_config = { board_light2_pin, 1U };
REGISTER_BOARD_DEVICE(RELAY, "light2", &board_light2_config);

static const gpio_output_driver_config_t board_light3_config = { board_light3_pin, 1U };
REGISTER_BOARD_DEVICE(RELAY, "light3", &board_light3_config);

static const hal_pin_t board_key_pins[] = {
    board_key1_pin,
    board_key2_pin,
    board_key3_pin,
    board_key4_pin
};
static const gpio_input_driver_config_t board_key_config = {
    board_key_pins,
    0,
    BOARD_KEY_COUNT,
    1U
};
REGISTER_BOARD_DEVICE(INPUT, "key", &board_key_config);
