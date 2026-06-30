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
        BOARD_ESP8266_USART_ID
    },
    board_esp8266_ch_pd_pin,
    board_esp8266_rst_pin,
    0U
};
REGISTER_BOARD_DEVICE(COMM, "esp8266", &board_esp8266_config);

static const i2c_device_config_t board_oled_config = {
    BOARD_OLED_I2C,
    BOARD_OLED_I2C_SPEED,
    board_oled_i2c_scl,
    board_oled_i2c_sda,
    BOARD_OLED_I2C_REMAP,
    BOARD_OLED_I2C_ADDR
};
REGISTER_BOARD_DEVICE(DISPLAY, "oled", &board_oled_config);

static const gpio_output_driver_config_t board_relay_config = { board_relay_pin, 1U };
REGISTER_BOARD_DEVICE(RELAY, "relay", &board_relay_config);

static const gpio_output_driver_config_t board_led_config = { board_led_pin, 0U };
REGISTER_BOARD_DEVICE(MISC, "led", &board_led_config);

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
