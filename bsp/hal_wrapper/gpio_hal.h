#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include "stm32f10x_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum gpio_hal_mode {
    GPIO_HAL_MODE_IN_FLOATING = 0,
    GPIO_HAL_MODE_IN_PULLUP,
    GPIO_HAL_MODE_OUT_PP,
    GPIO_HAL_MODE_OUT_OD,
    GPIO_HAL_MODE_AF_PP,
    GPIO_HAL_MODE_AF_OD,
    GPIO_HAL_MODE_ANALOG
} gpio_hal_mode_t;

typedef struct hal_pin {
    GPIO_TypeDef *port;
    uint16_t pin;
    gpio_hal_mode_t mode;
} hal_pin_t;

typedef enum gpio_hal_remap {
    GPIO_HAL_REMAP_NONE = 0,
    GPIO_HAL_REMAP_USART1,
    GPIO_HAL_REMAP_I2C1,
    GPIO_HAL_REMAP_TIM2_PARTIAL1,
    GPIO_HAL_REMAP_TIM2_PARTIAL2,
    GPIO_HAL_REMAP_TIM2_FULL,
    GPIO_HAL_REMAP_SPI1
} gpio_hal_remap_t;

void gpio_hal_clock_enable(GPIO_TypeDef *port);
void gpio_hal_apply_remap(gpio_hal_remap_t remap);
void gpio_hal_config_pin(const hal_pin_t *pin);
void gpio_hal_set_mode(GPIO_TypeDef *port, uint16_t pin, gpio_hal_mode_t mode);
void gpio_hal_write(GPIO_TypeDef *port, uint16_t pin, uint8_t value);
uint8_t gpio_hal_read(GPIO_TypeDef *port, uint16_t pin);
void gpio_hal_toggle(GPIO_TypeDef *port, uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif
