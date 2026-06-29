#include "input_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"

typedef void (*key_callback_t)(unsigned char key);

static key_callback_t key_registered_callback;

static void key_init(void)
{
#if BOARD_KEY_COUNT >= 1
    gpio_hal_config_pin(&board_key1_pin);
#endif
#if BOARD_KEY_COUNT >= 2
    gpio_hal_config_pin(&board_key2_pin);
#endif
#if BOARD_KEY_COUNT >= 3
    gpio_hal_config_pin(&board_key3_pin);
#endif
#if BOARD_KEY_COUNT >= 4
    gpio_hal_config_pin(&board_key4_pin);
#endif
}

static unsigned char key_read_pin(const hal_pin_t *pin)
{
    return gpio_hal_read(pin->port, pin->pin) == 0U ? 1U : 0U;
}

static unsigned char key_read_key(void)
{
#if BOARD_KEY_COUNT >= 1
    if (key_read_pin(&board_key1_pin) != 0U) {
        return 1U;
    }
#endif
#if BOARD_KEY_COUNT >= 2
    if (key_read_pin(&board_key2_pin) != 0U) {
        return 2U;
    }
#endif
#if BOARD_KEY_COUNT >= 3
    if (key_read_pin(&board_key3_pin) != 0U) {
        return 3U;
    }
#endif
#if BOARD_KEY_COUNT >= 4
    if (key_read_pin(&board_key4_pin) != 0U) {
        return 4U;
    }
#endif
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
