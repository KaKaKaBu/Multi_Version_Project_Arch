#include "stepper_if.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

static void stepmotor_write_phase(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
    gpio_hal_write(board_stepmotor_a_pin.port, board_stepmotor_a_pin.pin, a);
    gpio_hal_write(board_stepmotor_b_pin.port, board_stepmotor_b_pin.pin, b);
    gpio_hal_write(board_stepmotor_c_pin.port, board_stepmotor_c_pin.pin, c);
    gpio_hal_write(board_stepmotor_d_pin.port, board_stepmotor_d_pin.pin, d);
}

static void stepmotor_delay_ms(unsigned short ms)
{
    while (ms > 0U) {
        unsigned short chunk = (ms > 50U) ? 50U : ms;

        timer_hal_delay_us((unsigned short)(chunk * 1000U));
        ms = (unsigned short)(ms - chunk);
    }
}

static void stepmotor_rhythm_4_1_4(unsigned char step, unsigned short delay_ms)
{
    switch (step) {
    case 1U:
        stepmotor_write_phase(0U, 1U, 1U, 1U);
        break;
    case 2U:
        stepmotor_write_phase(1U, 0U, 1U, 1U);
        break;
    case 3U:
        stepmotor_write_phase(1U, 1U, 0U, 1U);
        break;
    case 4U:
        stepmotor_write_phase(1U, 1U, 1U, 0U);
        break;
    default:
        break;
    }

    if (delay_ms > 0U) {
        stepmotor_delay_ms(delay_ms);
    }
}

static void stepmotor_direction(unsigned char dir, unsigned short delay_ms)
{
    unsigned char step;

    if (dir != 0U) {
        for (step = 1U; step <= 4U; step++) {
            stepmotor_rhythm_4_1_4(step, delay_ms);
        }
    } else {
        for (step = 4U; step > 0U; step--) {
            stepmotor_rhythm_4_1_4(step, delay_ms);
        }
    }
}

static void stepmotor_init(void)
{
    gpio_hal_config_pin(&board_stepmotor_a_pin);
    gpio_hal_config_pin(&board_stepmotor_b_pin);
    gpio_hal_config_pin(&board_stepmotor_c_pin);
    gpio_hal_config_pin(&board_stepmotor_d_pin);
    stepmotor_write_phase(0U, 0U, 0U, 0U);
}

static void stepmotor_step_angle(unsigned char direction, unsigned short angle_deg, unsigned short step_delay_ms)
{
    unsigned short cycles;
    unsigned short i;

    if (angle_deg == 0U) {
        return;
    }

    cycles = (unsigned short)((64U * angle_deg) / 45U);
    if (cycles == 0U) {
        cycles = 1U;
    }

    for (i = 0U; i < cycles; i++) {
        stepmotor_direction(direction, step_delay_ms);
    }
}

static void stepmotor_stop(void)
{
    stepmotor_write_phase(0U, 0U, 0U, 0U);
}

static const stepper_driver_t stepmotor_drv = {
    "stepmotor",
    stepmotor_init,
    stepmotor_step_angle,
    stepmotor_stop
};

REGISTER_DRIVER(STEPPER, stepmotor_drv);
