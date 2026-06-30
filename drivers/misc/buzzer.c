#include "gpio_hal.h"
#include "misc_if.h"
#include "driver_configs.h"
#include "driver_core.h"
#if !defined(PLATFORM_MCS51)
#endif

static const gpio_output_driver_config_t *buzzer_config;

static void buzzer_init(const void *config)
{
    buzzer_config = (const gpio_output_driver_config_t *)config;
    if (buzzer_config == 0) {
        return;
    }
    gpio_hal_config_pin(&buzzer_config->pin);
    gpio_hal_write(buzzer_config->pin.port, buzzer_config->pin.pin, buzzer_config->active_high ? 0U : 1U);
}

static void buzzer_set_state(unsigned char on)
{
    uint8_t level;

    if (buzzer_config == 0) {
        return;
    }

    level = (on != 0U) ? buzzer_config->active_high : (uint8_t)!buzzer_config->active_high;
    gpio_hal_write(buzzer_config->pin.port, buzzer_config->pin.pin, level);
}

const misc_driver_t buzzer_drv = {
    "buzzer",
    buzzer_init,
    buzzer_set_state
};

REGISTER_DRIVER(MISC, buzzer_drv);
