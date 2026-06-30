/**
 * @file key_4ch.c
 * @brief 四键 GPIO 输入，注册名为 "key"。
 */

#include "input_if.h"
#include "gpio_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static const gpio_input_driver_config_t *key_4ch_config;

static const hal_pin_t *key_4ch_get_pin(uint8_t index)
{
    if ((key_4ch_config == 0) || (index >= key_4ch_config->count)) {
        return 0;
    }
    if (key_4ch_config->pin_refs != 0) {
        return key_4ch_config->pin_refs[index];
    }
    if (key_4ch_config->pins != 0) {
        return &key_4ch_config->pins[index];
    }
    return 0;
}

static void key_4ch_init(const void *config)
{
    uint8_t i;
    const hal_pin_t *pin;

    key_4ch_config = (const gpio_input_driver_config_t *)config;
    if (key_4ch_config == 0) {
        return;
    }

    for (i = 0U; i < key_4ch_config->count; ++i) {
        pin = key_4ch_get_pin(i);
        if (pin != 0) {
            gpio_hal_config_pin(pin);
        }
    }
}

static unsigned char key_4ch_read_key(void)
{
    uint8_t i;
    uint8_t active;
    const hal_pin_t *pin;

    if (key_4ch_config == 0) {
        return 0U;
    }

    for (i = 0U; i < key_4ch_config->count; ++i) {
        pin = key_4ch_get_pin(i);
        if (pin != 0) {
            active = gpio_hal_read(pin->port, pin->pin);
            if (key_4ch_config->active_low != 0U) {
                active = (uint8_t)(active == 0U);
            }
            if (active != 0U) {
                return (unsigned char)(i + 1U);
            }
        }
    }

    return 0U;
}

static const input_driver_t key_4ch_drv = {
    "key",
    key_4ch_init,
    key_4ch_read_key
};

REGISTER_DRIVER(INPUT, key_4ch_drv);
