/* FJXT_001 板级设备实例表：只描述本项目硬件实例，驱动实现由 drivers/ 自注册。 */
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

static const gpio_input_driver_config_t board_key_config = {
    0,
    KEY_DRIVER_PIN_TABLE,
    KEY_DRIVER_BUTTON_COUNT,
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

static const gpio_output_driver_config_t board_buzzer_config = { board_buzzer_pin, 1U };
REGISTER_BOARD_DEVICE(MISC, "buzzer", &board_buzzer_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, 1U };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

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
        0U
    },
    board_esp8266_ch_pd_pin,
    board_esp8266_rst_pin,
    1U
};
REGISTER_BOARD_DEVICE(COMM, "esp8266", &board_esp8266_config);
#endif

#if VERSION_FEATURE_VOICE
static const usart_device_config_t board_su03t_config = {
    BOARD_SU03T_USART,
    BOARD_SU03T_BAUDRATE,
    board_su03t_tx,
    board_su03t_rx,
    BOARD_SU03T_USART_REMAP,
    USART_HAL_TX_MODE_IRQ,
    0U,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "su03t", &board_su03t_config);
#endif
