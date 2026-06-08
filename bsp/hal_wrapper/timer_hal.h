/**
 * @file timer_hal.h
 * @brief Timer microsecond delay, PWM output, and update-interrupt helpers.
 */

#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include "gpio_hal.h"
#include "hal_status.h"
#include "stm32f10x_tim.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PWM timebase, channel, output pin, and optional TIM remap. */
typedef struct timer_hal_pwm_config {
    TIM_TypeDef *instance;
    uint16_t period;
    uint16_t prescaler;
    uint8_t channel;
    hal_pin_t pin;
    gpio_hal_remap_t remap;
} timer_hal_pwm_config_t;

/**
 * @brief Configures a timer for 1 MHz tick rate with update period @p period_us.
 * @param TIMx Timer instance (TIM1–TIM4).
 * @param period_us Auto-reload value in microseconds (ARR = period_us - 1).
 */
void timer_hal_init_us(TIM_TypeDef *TIMx, uint16_t period_us);

/**
 * @brief Busy-waits for @p us microseconds using TIM4 as a free-running counter.
 * @param us Delay in microseconds.
 */
void timer_hal_delay_us(uint16_t us);

/**
 * @brief Initializes PWM on @p cfg->channel with GPIO and timebase from @p cfg.
 * @param cfg PWM configuration (instance and pin must be valid).
 * @return HAL_OK or HAL_ERR_PARAM.
 */
hal_status_t timer_hal_pwm_init(const timer_hal_pwm_config_t *cfg);

/**
 * @brief Sets the compare register for PWM channel 1–4.
 * @param TIMx Timer instance.
 * @param channel Channel number (1–4).
 * @param compare Compare value (pulse width).
 */
void timer_hal_pwm_set_compare(TIM_TypeDef *TIMx, uint8_t channel, uint16_t compare);

/**
 * @brief Enables timer update interrupt and NVIC for @p TIMx.
 * @param TIMx Timer instance.
 */
void timer_hal_enable_update_irq(TIM_TypeDef *TIMx);

/**
 * @brief Timer update IRQ handler; posts irq_event and clears update flag.
 * @param TIMx Timer instance.
 */
void timer_hal_irq_handler(TIM_TypeDef *TIMx);

#ifdef __cplusplus
}
#endif

#endif
