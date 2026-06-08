/**
 * @file e18_presence.c
 * @brief E18-D80NK 人体红外（最多 3 路 GPIO），analog_probe 返回 0/1 表示是否有人。
 */

#include "analog_probe_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void e18_init(void)
{
#if BOARD_E18_PIR_COUNT >= 1U
    gpio_hal_config_pin(&board_e18_pir1_pin);
#endif
#if BOARD_E18_PIR_COUNT >= 2U
    gpio_hal_config_pin(&board_e18_pir2_pin);
#endif
#if BOARD_E18_PIR_COUNT >= 3U
    gpio_hal_config_pin(&board_e18_pir3_pin);
#endif
}

static unsigned char e18_pin_active(const hal_pin_t *pin)
{
    unsigned char level;

    if (pin == 0) {
        return 0U;
    }

    level = gpio_hal_read(pin->port, pin->pin);
#if BOARD_E18_PIR_ACTIVE_LOW
    return (level == 0U) ? 1U : 0U;
#else
    return (level != 0U) ? 1U : 0U;
#endif
}

static float e18_read_value(void)
{
#if BOARD_E18_PIR_COUNT >= 1U
    if (e18_pin_active(&board_e18_pir1_pin) != 0U) {
        return 1.0f;
    }
#endif
#if BOARD_E18_PIR_COUNT >= 2U
    if (e18_pin_active(&board_e18_pir2_pin) != 0U) {
        return 1.0f;
    }
#endif
#if BOARD_E18_PIR_COUNT >= 3U
    if (e18_pin_active(&board_e18_pir3_pin) != 0U) {
        return 1.0f;
    }
#endif

    return 0.0f;
}

static const analog_probe_t e18_presence_drv = {
    "presence",
    e18_init,
    e18_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, e18_presence_drv);
