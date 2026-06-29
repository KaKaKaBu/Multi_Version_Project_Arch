#include "actuator_if.h"
#include "timer_hal.h"
#include "board_config.h"
#include "driver_core.h"

static unsigned short sg90_angle_cache;

static void sg90_init(void)
{
    timer_hal_pwm_config_t cfg;

    cfg.instance = BOARD_SG90_TIM;
    cfg.period = BOARD_SG90_TIM_PERIOD;
    cfg.prescaler = BOARD_SG90_TIM_PRESCALER;
    cfg.channel = BOARD_SG90_TIM_CHANNEL;
    cfg.pin = board_sg90_pwm_pin;
    cfg.remap = BOARD_SG90_TIM_REMAP;
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
    timer_hal_pwm_set_compare(BOARD_SG90_TIM, BOARD_SG90_TIM_CHANNEL, compare);
}

static const servo_driver_t sg90_drv = {
    "sg90",
    sg90_init,
    sg90_set_angle
};

REGISTER_DRIVER(SERVO, sg90_drv);

#ifdef BOARD_SG90_2

static unsigned short sg90_2_angle_cache;

static void sg90_2_init(void)
{
    timer_hal_pwm_config_t cfg;

    cfg.instance = BOARD_SG90_2_TIM;
    cfg.period = BOARD_SG90_TIM_PERIOD;
    cfg.prescaler = BOARD_SG90_TIM_PRESCALER;
    cfg.channel = BOARD_SG90_2_TIM_CHANNEL;
    cfg.pin = board_sg90_2_pwm_pin;
    cfg.remap = BOARD_SG90_TIM_REMAP;
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
    timer_hal_pwm_set_compare(BOARD_SG90_2_TIM, BOARD_SG90_2_TIM_CHANNEL, compare);
}

static const servo_driver_t sg90_2_drv = {
    "sg90_2",
    sg90_2_init,
    sg90_2_set_angle
};

REGISTER_DRIVER(SERVO, sg90_2_drv);

#endif
