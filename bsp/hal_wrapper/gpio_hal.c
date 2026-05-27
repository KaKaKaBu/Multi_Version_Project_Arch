#include "gpio_hal.h"
#include "stm32f10x_rcc.h"

void gpio_hal_clock_enable(GPIO_TypeDef *port)
{
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

void gpio_hal_set_mode(GPIO_TypeDef *port, uint16_t pin, gpio_hal_mode_t mode)
{
    GPIO_InitTypeDef gpio;

    gpio_hal_clock_enable(port);
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

void gpio_hal_config_pin(const hal_pin_t *pin)
{
    if (pin == 0) {
        return;
    }

    gpio_hal_set_mode(pin->port, pin->pin, pin->mode);
}

void gpio_hal_write(GPIO_TypeDef *port, uint16_t pin, uint8_t value)
{
    if (value != 0U) {
        GPIO_SetBits(port, pin);
    } else {
        GPIO_ResetBits(port, pin);
    }
}

uint8_t gpio_hal_read(GPIO_TypeDef *port, uint16_t pin)
{
    return (uint8_t)GPIO_ReadInputDataBit(port, pin);
}

void gpio_hal_toggle(GPIO_TypeDef *port, uint16_t pin)
{
    if (GPIO_ReadOutputDataBit(port, pin) != Bit_RESET) {
        GPIO_ResetBits(port, pin);
    } else {
        GPIO_SetBits(port, pin);
    }
}
