#include "input_if.h"
#include "gpio_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

typedef void (*key_callback_t)(unsigned char key);

static key_callback_t key_registered_callback;
static const gpio_input_driver_config_t *key_config;

static const hal_pin_t *key_get_pin(uint8_t index)
{
    if ((key_config == 0) || (index >= key_config->count)) {
        return 0;
    }
    if (key_config->pin_refs != 0) {
        return key_config->pin_refs[index];
    }
    if (key_config->pins != 0) {
        return &key_config->pins[index];
    }
    return 0;
}

static void key_init(const void *config)
{
    uint8_t i;
    const hal_pin_t *pin;

    key_config = (const gpio_input_driver_config_t *)config;
    if (key_config == 0) {
        return;
    }

    for (i = 0U; i < key_config->count; ++i) {
        pin = key_get_pin(i);
        if (pin != 0) {
            gpio_hal_config_pin(pin);
        }
    }
}

static unsigned char key_read_pin(const hal_pin_t *pin)
{
    uint8_t level = gpio_hal_read(pin->port, pin->pin);
    if ((key_config != 0) && (key_config->active_low != 0U)) {
        return level == 0U ? 1U : 0U;
    }
    return level != 0U ? 1U : 0U;
}

static unsigned char key_read_key(void)
{
    uint8_t i;
    const hal_pin_t *pin;

    if (key_config == 0) {
        return 0U;
    }

    for (i = 0U; i < key_config->count; ++i) {
        pin = key_get_pin(i);
        if ((pin != 0) && (key_read_pin(pin) != 0U)) {
            return (unsigned char)(i + 1U);
        }
    }
    return 0U;
}

void key_register_callback(void (*callback)(unsigned char key))
{
    key_registered_callback = callback;
}

void key_poll_dispatch(void)
{
    unsigned char key = key_read_key();

    if ((key != 0U) && (key_registered_callback != 0)) {
        key_registered_callback(key);
    }
}

const input_driver_t key_drv = {
    "key",
    key_init,
    key_read_key
};

REGISTER_DRIVER(INPUT, key_drv);
