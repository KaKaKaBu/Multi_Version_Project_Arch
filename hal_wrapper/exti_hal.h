#ifndef EXTI_HAL_H
#define EXTI_HAL_H

#include "gpio_hal.h"
#include "hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum exti_hal_trigger {
    EXTI_HAL_TRIGGER_RISING = 0,
    EXTI_HAL_TRIGGER_FALLING,
    EXTI_HAL_TRIGGER_BOTH,
    EXTI_HAL_TRIGGER_RISING_FALLING = EXTI_HAL_TRIGGER_BOTH
} exti_hal_trigger_t;

typedef enum exti_hal_irq_channel {
    EXTI_HAL_IRQ_LINE0 = 0,
    EXTI_HAL_IRQ_LINE1,
    EXTI_HAL_IRQ_LINE2,
    EXTI_HAL_IRQ_LINE3,
    EXTI_HAL_IRQ_LINE4,
    EXTI_HAL_IRQ_LINE5_9,
    EXTI_HAL_IRQ_LINE10_15,
    EXTI_HAL_IRQ_CHANNEL_COUNT
} exti_hal_irq_channel_t;

void exti_hal_select_pin(uint8_t port_source, uint8_t pin_source);
uint8_t exti_hal_select_gpio_pin(const hal_pin_t *pin);
void exti_hal_configure_line(uint32_t line_mask, exti_hal_trigger_t trigger);
uint8_t exti_hal_configure_gpio_pin(const hal_pin_t *pin, exti_hal_trigger_t trigger);
uint32_t exti_hal_line_mask_from_pin(const hal_pin_t *pin);
uint8_t exti_hal_get_it_status(uint32_t line_mask);
void exti_hal_clear_pending(uint32_t line_mask);
void exti_hal_enable_irq(exti_hal_irq_channel_t channel, uint8_t priority);
exti_hal_irq_channel_t exti_hal_irq_channel_from_line(uint8_t line);

#ifdef __cplusplus
}
#endif

#endif
