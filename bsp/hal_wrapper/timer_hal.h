#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include "gpio_hal.h"
#include "hal_status.h"
#include "stm32f10x_tim.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timer_hal_pwm_config {
    TIM_TypeDef *instance;
    uint16_t period;
    uint16_t prescaler;
    uint8_t channel;
    hal_pin_t pin;
    gpio_hal_remap_t remap;
} timer_hal_pwm_config_t;

void timer_hal_init_us(TIM_TypeDef *TIMx, uint16_t period_us);
void timer_hal_delay_us(uint16_t us);
hal_status_t timer_hal_pwm_init(const timer_hal_pwm_config_t *cfg);
void timer_hal_pwm_set_compare(TIM_TypeDef *TIMx, uint8_t channel, uint16_t compare);
void timer_hal_enable_update_irq(TIM_TypeDef *TIMx);
void timer_hal_irq_handler(TIM_TypeDef *TIMx);

#ifdef __cplusplus
}
#endif

#endif
