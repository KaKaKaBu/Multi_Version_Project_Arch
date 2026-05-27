#include "gpio_hal.h"
#include "misc_if.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void buzzer_init(void)
{
    gpio_hal_config_pin(&board_buzzer_pin);
    gpio_hal_write(board_buzzer_pin.port, board_buzzer_pin.pin, 0U);
}

static void buzzer_set_state(unsigned char on)
{
    gpio_hal_write(board_buzzer_pin.port, board_buzzer_pin.pin, on);
}

static const misc_driver_t buzzer_drv = {
    "buzzer",
    buzzer_init,
    buzzer_set_state
};

REGISTER_DRIVER(MISC, buzzer_drv);
