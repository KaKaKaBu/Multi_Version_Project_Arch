#include "distance_if.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

#define HCSR04_ECHO_TIMEOUT_US 30000U

typedef struct hcsr04_poll_ctx {
    const hal_pin_t *pin;
    uint8_t expect_high;
} hcsr04_poll_ctx_t;

static unsigned short hcsr04_distance_cache;
static const hcsr04_driver_config_t *hcsr04_config;

static uint8_t hcsr04_echo_poll(void *ctx)
{
    hcsr04_poll_ctx_t *poll = (hcsr04_poll_ctx_t *)ctx;
    uint8_t level = gpio_hal_read(poll->pin->port, poll->pin->pin);

    if (poll->expect_high != 0U) {
        return level;
    }
    return (uint8_t)(level == 0U);
}

static void hcsr04_trigger(void)
{
    gpio_hal_write(hcsr04_config->trig.port, hcsr04_config->trig.pin, 1U);
    hal_delay_us(15U);
    gpio_hal_write(hcsr04_config->trig.port, hcsr04_config->trig.pin, 0U);
}

static unsigned short hcsr04_measure_cm(void)
{
    hcsr04_poll_ctx_t ctx;
    uint32_t start_us;
    uint32_t duration_us;

    if (hcsr04_config == 0) {
        return 0U;
    }

    ctx.pin = &hcsr04_config->echo;

    hcsr04_trigger();

    ctx.expect_high = 1U;
    if (hal_wait_flag_us(hcsr04_echo_poll, &ctx, HCSR04_ECHO_TIMEOUT_US) != HAL_OK) {
        return 0U;
    }

    start_us = hal_get_us();
    ctx.expect_high = 0U;
    if (hal_wait_flag_us(hcsr04_echo_poll, &ctx, HCSR04_ECHO_TIMEOUT_US) != HAL_OK) {
        return 0U;
    }

    duration_us = hal_get_us() - start_us;
    if (duration_us == 0U) {
        return 0U;
    }

    return (unsigned short)((duration_us * 34U) / 2000U);
}

static void hcsr04_init(const void *config)
{
    hcsr04_config = (const hcsr04_driver_config_t *)config;
    if (hcsr04_config == 0) {
        return;
    }
    gpio_hal_config_pin(&hcsr04_config->trig);
    gpio_hal_config_pin(&hcsr04_config->echo);
    gpio_hal_write(hcsr04_config->trig.port, hcsr04_config->trig.pin, 0U);
    hcsr04_distance_cache = 0U;
}

static unsigned short hcsr04_read_distance_cm(void)
{
    hcsr04_distance_cache = hcsr04_measure_cm();
    return hcsr04_distance_cache;
}

static const distance_sensor_t hcsr04_drv = {
    "hcsr04",
    hcsr04_init,
    hcsr04_read_distance_cm
};

REGISTER_DRIVER(DISTANCE_SENSOR, hcsr04_drv);
