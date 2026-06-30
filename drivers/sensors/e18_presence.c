/**
 * @file e18_presence.c
 * @brief E18-D80NK 人体红外（最多 3 路 GPIO），analog_probe 返回 0/1 表示是否有人。
 */

#include "analog_probe_if.h"
#include "gpio_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static const gpio_input_driver_config_t *e18_config;

static void e18_init(const void *config)
{
    uint8_t i;

    e18_config = (const gpio_input_driver_config_t *)config;
    if (e18_config == 0) {
        return;
    }

    for (i = 0U; i < e18_config->count; ++i) {
        gpio_hal_config_pin(&e18_config->pins[i]);
    }
}

static unsigned char e18_pin_active(const hal_pin_t *pin)
{
    unsigned char level;

    if (pin == 0) {
        return 0U;
    }

    level = gpio_hal_read(pin->port, pin->pin);
    if ((e18_config != 0) && (e18_config->active_low != 0U)) {
        return (level == 0U) ? 1U : 0U;
    }
    return (level != 0U) ? 1U : 0U;
}

static float e18_read_value(void)
{
    uint8_t i;

    if (e18_config == 0) {
        return 0.0f;
    }

    for (i = 0U; i < e18_config->count; ++i) {
        if (e18_pin_active(&e18_config->pins[i]) != 0U) {
            return 1.0f;
        }
    }

    return 0.0f;
}

static const analog_probe_t e18_presence_drv = {
    "presence",
    e18_init,
    e18_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, e18_presence_drv);
