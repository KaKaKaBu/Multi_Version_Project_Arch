/**
 * @file key_4ch.c
 * @brief 四键 GPIO 输入，注册名为 "key"。
 */

#include "input_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void key_4ch_init(void)
{
    gpio_hal_config_pin(&board_key1_pin);
    gpio_hal_config_pin(&board_key2_pin);
    gpio_hal_config_pin(&board_key3_pin);
    gpio_hal_config_pin(&board_key4_pin);
}

static unsigned char key_4ch_read_key(void)
{
    if (gpio_hal_read(board_key1_pin.port, board_key1_pin.pin) == 0U) {
        return 1U;
    }
    if (gpio_hal_read(board_key2_pin.port, board_key2_pin.pin) == 0U) {
        return 2U;
    }
    if (gpio_hal_read(board_key3_pin.port, board_key3_pin.pin) == 0U) {
        return 3U;
    }
    if (gpio_hal_read(board_key4_pin.port, board_key4_pin.pin) == 0U) {
        return 4U;
    }

    return 0U;
}

static const input_driver_t key_4ch_drv = {
    "key",
    key_4ch_init,
    key_4ch_read_key
};

REGISTER_DRIVER(INPUT, key_4ch_drv);
