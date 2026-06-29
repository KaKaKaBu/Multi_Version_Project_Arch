/**
 * @file relay.c
 * @brief GPIO relay actuator driver registered as RELAY via actuator_if.h.
 */

#include "actuator_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#if !defined(PLATFORM_MCS51)
#endif

/** @brief Configures relay GPIO and drives the coil off at startup. */
static void relay_init(void)
{
    gpio_hal_config_pin(&board_relay_pin);
    gpio_hal_write(board_relay_pin.port, board_relay_pin.pin, 0U);
}

/**
 * @brief Sets relay coil energized state.
 * @param on Non-zero turns the relay on; zero turns it off.
 */
static void relay_set_state(unsigned char on)
{
    gpio_hal_write(board_relay_pin.port, board_relay_pin.pin, on);
}

/** @brief actuator_if.h relay driver instance registered as RELAY. */
const relay_driver_t relay_drv = {
    "relay",
    relay_init,
    relay_set_state
};

REGISTER_DRIVER(RELAY, relay_drv);
