#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include "gpio_hal.h"
#include "hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timer_hal_pwm_config {
    hal_timer_id_t instance;
    uint16_t period;
    uint16_t prescaler;
    uint8_t channel;
    hal_pin_t pin;
    gpio_hal_remap_t remap;
} timer_hal_pwm_config_t;

void timer_hal_init_us(hal_timer_id_t instance, uint16_t period_us);
void timer_hal_delay_us(uint32_t us);
hal_status_t timer_hal_pwm_init(const timer_hal_pwm_config_t *cfg);
void timer_hal_pwm_set_compare(hal_timer_id_t instance, uint8_t channel, uint16_t compare);
void timer_hal_enable_update_irq(hal_timer_id_t instance);
void timer_hal_irq_handler(hal_timer_id_t instance);

#ifdef __cplusplus
}
#endif

#endif
