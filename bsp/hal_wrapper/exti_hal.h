/**
 * @file exti_hal.h
 * @brief EXTI line configuration, status, and NVIC helpers for STM32F10x.
 */

#ifndef EXTI_HAL_H
#define EXTI_HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Trigger selection for EXTI lines. */
typedef enum exti_hal_trigger {
    EXTI_HAL_TRIGGER_RISING = 0,
    EXTI_HAL_TRIGGER_FALLING,
    EXTI_HAL_TRIGGER_BOTH
} exti_hal_trigger_t;

/** @brief NVIC channel group for EXTI lines. */
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

/**
 * @brief Connects a GPIO port/pin to the AFIO EXTI line selector.
 * @param port_source GPIO_PortSourceGPIOx value.
 * @param pin_source GPIO_PinSourcex value.
 */
void exti_hal_select_pin(uint8_t port_source, uint8_t pin_source);

/**
 * @brief Configures an EXTI line for interrupt mode and clears pending status.
 * @param line_mask Bit mask for the EXTI line (1u << line number).
 * @param trigger Trigger edge selection.
 */
void exti_hal_configure_line(uint32_t line_mask, exti_hal_trigger_t trigger);

/**
 * @brief Returns SET when the specified EXTI line has a pending interrupt.
 */
uint8_t exti_hal_get_it_status(uint32_t line_mask);

/**
 * @brief Clears the pending flag for the specified EXTI line mask.
 */
void exti_hal_clear_pending(uint32_t line_mask);

/**
 * @brief Enables the NVIC channel that covers the given EXTI line group.
 * @param channel Channel enumeration derived from the EXTI line.
 * @param priority Preemption priority to apply.
 */
void exti_hal_enable_irq(exti_hal_irq_channel_t channel, uint8_t priority);

/**
 * @brief Maps an EXTI line number to its IRQ channel group.
 * @param line Line number 0-15.
 * @return Channel enumeration corresponding to the NVIC IRQ.
 */
exti_hal_irq_channel_t exti_hal_irq_channel_from_line(uint8_t line);

#ifdef __cplusplus
}
#endif

#endif /* EXTI_HAL_H */
