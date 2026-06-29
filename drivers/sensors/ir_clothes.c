#include "analog_probe_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void ir_clothes_init(void)
{
    gpio_hal_config_pin(&board_ir_clothes_pin);
}

static float ir_clothes_read_value(void)
{
    unsigned char level;

    level = gpio_hal_read(board_ir_clothes_pin.port, board_ir_clothes_pin.pin);
#if BOARD_IR_CLOTHES_ACTIVE_LOW
    return (level == 0U) ? 1.0f : 0.0f;
#else
    return (level != 0U) ? 1.0f : 0.0f;
#endif
}

static const analog_probe_t ir_clothes_drv = {
    "ir_clothes",
    ir_clothes_init,
    ir_clothes_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, ir_clothes_drv);
