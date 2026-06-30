/**
 * @file led.c
 * @brief Board status LED driver registered as MISC via misc_if.h.
 */

#include "gpio_hal.h"
#include "misc_if.h"
#include "driver_configs.h"
#include "debug_uart.h"
#include "driver_core.h"
#if !defined(PLATFORM_MCS51)
#endif

static const gpio_output_driver_config_t *led_config;

/** @brief Initializes LED GPIO unless the pin is shared with debug UART. */
static void led_init(const void *config)
{
    led_config = (const gpio_output_driver_config_t *)config;
    if (led_config == 0) {
        return;
    }

    if (debug_uart_uses_pin(&led_config->pin) != 0) {
        return;
    }

    gpio_hal_config_pin(&led_config->pin);
    gpio_hal_write(led_config->pin.port, led_config->pin.pin, led_config->active_high ? 0U : 1U);
}

/**
 * @brief Sets LED on/off with active-low wiring.
 * @param on Non-zero turns the LED on; zero turns it off.
 */
static void led_set_state(unsigned char on)
{
    uint8_t level;

    if (led_config == 0) {
        return;
    }

    if (debug_uart_uses_pin(&led_config->pin) != 0) {
        return;
    }

    level = (on != 0U) ? led_config->active_high : (uint8_t)!led_config->active_high;
    gpio_hal_write(led_config->pin.port, led_config->pin.pin, level);
}

/** @brief misc_if.h LED driver instance registered as MISC. */
const misc_driver_t led_drv = {
    "led",
    led_init,
    led_set_state
};

REGISTER_DRIVER(MISC, led_drv);
