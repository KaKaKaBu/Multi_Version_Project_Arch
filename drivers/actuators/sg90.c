#include "actuator_if.h"
#include "timer_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static unsigned short sg90_angle_cache;
static const servo_driver_config_t *sg90_config;
static const servo_driver_config_t *sg90_2_config;

static void sg90_init(const void *config)
{
    timer_hal_pwm_config_t cfg;

    sg90_config = (const servo_driver_config_t *)config;
    if (sg90_config == 0) {
        return;
    }

    cfg.instance = sg90_config->timer;
    cfg.period = sg90_config->period;
    cfg.prescaler = sg90_config->prescaler;
    cfg.channel = sg90_config->channel;
    cfg.pin = sg90_config->pin;
    cfg.remap = sg90_config->remap;
    (void)timer_hal_pwm_init(&cfg);
    sg90_angle_cache = 90U;
}

static void sg90_set_angle(unsigned short angle)
{
    unsigned short compare;

    if (angle > 180U) {
        angle = 180U;
    }

    sg90_angle_cache = angle;
    compare = (unsigned short)(500U + ((uint32_t)angle * 2000U) / 180U);
    if (sg90_config == 0) {
        return;
    }
    timer_hal_pwm_set_compare(sg90_config->timer, sg90_config->channel, compare);
}

static const servo_driver_t sg90_drv = {
    "sg90",
    sg90_init,
    sg90_set_angle
};

REGISTER_DRIVER(SERVO, sg90_drv);

static unsigned short sg90_2_angle_cache;

static void sg90_2_init(const void *config)
{
    timer_hal_pwm_config_t cfg;

    sg90_2_config = (const servo_driver_config_t *)config;
    if (sg90_2_config == 0) {
        return;
    }

    cfg.instance = sg90_2_config->timer;
    cfg.period = sg90_2_config->period;
    cfg.prescaler = sg90_2_config->prescaler;
    cfg.channel = sg90_2_config->channel;
    cfg.pin = sg90_2_config->pin;
    cfg.remap = sg90_2_config->remap;
    (void)timer_hal_pwm_init(&cfg);
    sg90_2_angle_cache = 90U;
}

static void sg90_2_set_angle(unsigned short angle)
{
    unsigned short compare;

    if (angle > 180U) {
        angle = 180U;
    }

    sg90_2_angle_cache = angle;
    compare = (unsigned short)(500U + ((uint32_t)angle * 2000U) / 180U);
    if (sg90_2_config == 0) {
        return;
    }
    timer_hal_pwm_set_compare(sg90_2_config->timer, sg90_2_config->channel, compare);
}

static const servo_driver_t sg90_2_drv = {
    "sg90_2",
    sg90_2_init,
    sg90_2_set_angle
};

REGISTER_DRIVER(SERVO, sg90_2_drv);
