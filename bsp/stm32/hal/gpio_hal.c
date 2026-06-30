/**
 * @file gpio_hal.c
 * @brief GPIO clock, remap, mode, and bit I/O implementation for STM32F10x.
 */

#include "gpio_hal.h"
#include "stm32f1_hal_map.h"
#include "stm32f10x_rcc.h"

/**
 * @brief Enables the RCC clock for the given GPIO port.
 * @param port GPIO port base address (GPIOA–GPIOE).
 */
void gpio_hal_clock_enable(hal_gpio_port_t port_id)
{
    GPIO_TypeDef *port = stm32f1_gpio_port(port_id);

    if (port == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if (port == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (port == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    } else if (port == GPIOD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    } else if (port == GPIOE) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    }
}

/**
 * @brief Applies an AFIO pin remap when @p remap is not GPIO_HAL_REMAP_NONE.
 * @param remap Remap selector.
 */
void gpio_hal_apply_remap(gpio_hal_remap_t remap)
{
    if (remap == GPIO_HAL_REMAP_NONE) {
        return;
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    switch (remap) {
    case GPIO_HAL_REMAP_USART1:
        GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
        break;
    case GPIO_HAL_REMAP_I2C1:
        GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
        break;
    case GPIO_HAL_REMAP_TIM2_PARTIAL1:
        GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
        break;
    case GPIO_HAL_REMAP_TIM2_PARTIAL2:
        GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM2, ENABLE);
        break;
    case GPIO_HAL_REMAP_TIM2_FULL:
        GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);
        break;
    case GPIO_HAL_REMAP_SPI1:
        GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);
        break;
    default:
        break;
    }
}

/**
 * @brief Sets mode and speed for a single pin on @p port.
 * @param port GPIO port.
 * @param pin Pin bitmask (e.g. GPIO_Pin_0).
 * @param mode Logical GPIO mode.
 */
void gpio_hal_set_mode(hal_gpio_port_t port_id, hal_gpio_pin_t pin_id, gpio_hal_mode_t mode)
{
    GPIO_TypeDef *port = stm32f1_gpio_port(port_id);
    uint16_t pin = stm32f1_gpio_pin(pin_id);
    GPIO_InitTypeDef gpio;

    if ((port == 0) || (pin == 0U)) {
        return;
    }

    gpio_hal_clock_enable(port_id);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = pin;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    switch (mode) {
    case GPIO_HAL_MODE_IN_PULLUP:
        gpio.GPIO_Mode = GPIO_Mode_IPU;
        break;
    case GPIO_HAL_MODE_OUT_PP:
        gpio.GPIO_Mode = GPIO_Mode_Out_PP;
        break;
    case GPIO_HAL_MODE_OUT_OD:
        gpio.GPIO_Mode = GPIO_Mode_Out_OD;
        break;
    case GPIO_HAL_MODE_AF_PP:
        gpio.GPIO_Mode = GPIO_Mode_AF_PP;
        break;
    case GPIO_HAL_MODE_AF_OD:
        gpio.GPIO_Mode = GPIO_Mode_AF_OD;
        break;
    case GPIO_HAL_MODE_ANALOG:
        gpio.GPIO_Mode = GPIO_Mode_AIN;
        break;
    case GPIO_HAL_MODE_IN_FLOATING:
    default:
        gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        break;
    }

    GPIO_Init(port, &gpio);
}

/**
 * @brief Configures a pin from a hal_pin_t descriptor (no-op if @p pin is NULL).
 * @param pin Pin descriptor.
 */
void gpio_hal_config_pin(const hal_pin_t *pin)
{
    if (pin == 0) {
        return;
    }

    gpio_hal_set_mode(pin->port, pin->pin, pin->mode);
}

/**
 * @brief Drives an output pin high or low.
 * @param port GPIO port.
 * @param pin Pin bitmask.
 * @param value Non-zero for high, zero for low.
 */
void gpio_hal_write(hal_gpio_port_t port_id, hal_gpio_pin_t pin_id, uint8_t value)
{
    GPIO_TypeDef *port = stm32f1_gpio_port(port_id);
    uint16_t pin = stm32f1_gpio_pin(pin_id);

    if ((port == 0) || (pin == 0U)) {
        return;
    }

    if (value != 0U) {
        GPIO_SetBits(port, pin);
    } else {
        GPIO_ResetBits(port, pin);
    }
}

/**
 * @brief Reads the logical level of an input pin.
 * @param port GPIO port.
 * @param pin Pin bitmask.
 * @return 1 if the pin is high, 0 if low.
 */
uint8_t gpio_hal_read(hal_gpio_port_t port_id, hal_gpio_pin_t pin_id)
{
    GPIO_TypeDef *port = stm32f1_gpio_port(port_id);
    uint16_t pin = stm32f1_gpio_pin(pin_id);

    if ((port == 0) || (pin == 0U)) {
        return 0U;
    }

    return (uint8_t)GPIO_ReadInputDataBit(port, pin);
}

/**
 * @brief Toggles an output pin.
 * @param port GPIO port.
 * @param pin Pin bitmask.
 */
void gpio_hal_toggle(hal_gpio_port_t port_id, hal_gpio_pin_t pin_id)
{
    GPIO_TypeDef *port = stm32f1_gpio_port(port_id);
    uint16_t pin = stm32f1_gpio_pin(pin_id);

    if ((port == 0) || (pin == 0U)) {
        return;
    }

    if (GPIO_ReadOutputDataBit(port, pin) != Bit_RESET) {
        GPIO_ResetBits(port, pin);
    } else {
        GPIO_SetBits(port, pin);
    }
}
