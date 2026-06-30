/**
 * @file relay.c
 * @brief GPIO relay actuator driver registered as RELAY via actuator_if.h.
 */

#include "actuator_if.h"
#include "gpio_hal.h"
#include "driver_configs.h"
#include "driver_core.h"
#if !defined(PLATFORM_MCS51)
#endif

static const gpio_output_driver_config_t *relay_config;

/** @brief Configures relay GPIO and drives the coil off at startup. */
static void relay_init(const void *config)
{
    relay_config = (const gpio_output_driver_config_t *)config;
    if (relay_config == 0) {
        return;
    }
    gpio_hal_config_pin(&relay_config->pin);
    gpio_hal_write(relay_config->pin.port, relay_config->pin.pin, relay_config->active_high ? 0U : 1U);
}

/**
 * @brief Sets relay coil energized state.
 * @param on Non-zero turns the relay on; zero turns it off.
 */
static void relay_set_state(unsigned char on)
{
    uint8_t level;

    if (relay_config == 0) {
        return;
    }

    level = (on != 0U) ? relay_config->active_high : (uint8_t)!relay_config->active_high;
    gpio_hal_write(relay_config->pin.port, relay_config->pin.pin, level);
}

/** @brief actuator_if.h relay driver instance registered as RELAY. */
const relay_driver_t relay_drv = {
    "relay",
    relay_init,
    relay_set_state
};

REGISTER_DRIVER(RELAY, relay_drv);
