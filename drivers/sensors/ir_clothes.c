#include "analog_probe_if.h"
#include "gpio_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static const gpio_probe_driver_config_t *ir_clothes_config;

static void ir_clothes_init(const void *config)
{
    ir_clothes_config = (const gpio_probe_driver_config_t *)config;
    if (ir_clothes_config == 0) {
        return;
    }
    gpio_hal_config_pin(&ir_clothes_config->pin);
}

static float ir_clothes_read_value(void)
{
    unsigned char level;

    if (ir_clothes_config == 0) {
        return 0.0f;
    }

    level = gpio_hal_read(ir_clothes_config->pin.port, ir_clothes_config->pin.pin);
    if (ir_clothes_config->active_low != 0U) {
        return (level == 0U) ? 1.0f : 0.0f;
    }
    return (level != 0U) ? 1.0f : 0.0f;
}

static const analog_probe_t ir_clothes_drv = {
    "ir_clothes",
    ir_clothes_init,
    ir_clothes_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, ir_clothes_drv);
