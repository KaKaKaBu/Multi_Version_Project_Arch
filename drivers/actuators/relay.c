#include "actuator_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void relay_init(void)
{
    gpio_hal_config_pin(&board_relay_pin);
    gpio_hal_write(board_relay_pin.port, board_relay_pin.pin, 0U);
}

static void relay_set_state(unsigned char on)
{
    gpio_hal_write(board_relay_pin.port, board_relay_pin.pin, on);
}

static const relay_driver_t relay_drv = {
    "relay",
    relay_init,
    relay_set_state
};

REGISTER_DRIVER(RELAY, relay_drv);
