#include "gpio_hal.h"
#include "misc_if.h"
#include "board_config.h"
#include "debug_uart.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void led_init(void)
{
    if (debug_uart_uses_pin(&board_led_pin) != 0) {
        return;
    }

    gpio_hal_config_pin(&board_led_pin);
    gpio_hal_write(board_led_pin.port, board_led_pin.pin, 1U);
}

static void led_set_state(unsigned char on)
{
    if (debug_uart_uses_pin(&board_led_pin) != 0) {
        return;
    }

    gpio_hal_write(board_led_pin.port, board_led_pin.pin, on == 0U ? 1U : 0U);
}

static const misc_driver_t led_drv = {
    "led",
    led_init,
    led_set_state
};

REGISTER_DRIVER(MISC, led_drv);
