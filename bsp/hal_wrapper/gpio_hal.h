/**
 * @file gpio_hal.h
 * @brief GPIO pin configuration, I/O, and AFIO remap helpers for STM32F10x.
 */

#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include "stm32f10x_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Logical GPIO mode mapped to SPL GPIO_Mode values. */
typedef enum gpio_hal_mode {
    GPIO_HAL_MODE_IN_FLOATING = 0,
    GPIO_HAL_MODE_IN_PULLUP,
    GPIO_HAL_MODE_OUT_PP,
    GPIO_HAL_MODE_OUT_OD,
    GPIO_HAL_MODE_AF_PP,
    GPIO_HAL_MODE_AF_OD,
    GPIO_HAL_MODE_ANALOG
} gpio_hal_mode_t;

/** @brief Port, pin mask, and mode for a single GPIO line. */
typedef struct hal_pin {
    GPIO_TypeDef *port;
    uint16_t pin;
    gpio_hal_mode_t mode;
} hal_pin_t;

/** @brief AFIO alternate-function remap selections supported by the HAL. */
typedef enum gpio_hal_remap {
    GPIO_HAL_REMAP_NONE = 0,
    GPIO_HAL_REMAP_USART1,
    GPIO_HAL_REMAP_I2C1,
    GPIO_HAL_REMAP_TIM2_PARTIAL1,
    GPIO_HAL_REMAP_TIM2_PARTIAL2,
    GPIO_HAL_REMAP_TIM2_FULL,
    GPIO_HAL_REMAP_SPI1
} gpio_hal_remap_t;

/**
 * @brief Enables the RCC clock for the given GPIO port.
 * @param port GPIO port base address (GPIOA–GPIOE).
 */
void gpio_hal_clock_enable(GPIO_TypeDef *port);

/**
 * @brief Applies an AFIO pin remap when @p remap is not GPIO_HAL_REMAP_NONE.
 * @param remap Remap selector.
 */
void gpio_hal_apply_remap(gpio_hal_remap_t remap);

/**
 * @brief Configures a pin from a hal_pin_t descriptor (no-op if @p pin is NULL).
 * @param pin Pin descriptor.
 */
void gpio_hal_config_pin(const hal_pin_t *pin);

/**
 * @brief Sets mode and speed for a single pin on @p port.
 * @param port GPIO port.
 * @param pin Pin bitmask (e.g. GPIO_Pin_0).
 * @param mode Logical GPIO mode.
 */
void gpio_hal_set_mode(GPIO_TypeDef *port, uint16_t pin, gpio_hal_mode_t mode);

/**
 * @brief Drives an output pin high or low.
 * @param port GPIO port.
 * @param pin Pin bitmask.
 * @param value Non-zero for high, zero for low.
 */
void gpio_hal_write(GPIO_TypeDef *port, uint16_t pin, uint8_t value);

/**
 * @brief Reads the logical level of an input pin.
 * @param port GPIO port.
 * @param pin Pin bitmask.
 * @return 1 if the pin is high, 0 if low.
 */
uint8_t gpio_hal_read(GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief Toggles an output pin.
 * @param port GPIO port.
 * @param pin Pin bitmask.
 */
void gpio_hal_toggle(GPIO_TypeDef *port, uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif
