/**
 * @file exti_hal.c
 * @brief EXTI helper implementation for STM32F10x SPL.
 */

#include "exti_hal.h"

#include "stm32f10x_exti.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "misc.h"

static uint8_t exti_hal_afio_enabled;

static void exti_hal_enable_afio_clock(void)
{
    if (exti_hal_afio_enabled == 0U) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        exti_hal_afio_enabled = 1U;
    }
}

void exti_hal_select_pin(uint8_t port_source, uint8_t pin_source)
{
    exti_hal_enable_afio_clock();
    GPIO_EXTILineConfig(port_source, pin_source);
}

void exti_hal_configure_line(uint32_t line_mask, exti_hal_trigger_t trigger)
{
    EXTI_InitTypeDef exti;

    exti_hal_enable_afio_clock();
    EXTI_StructInit(&exti);
    exti.EXTI_Line = line_mask;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    switch (trigger) {
    case EXTI_HAL_TRIGGER_RISING:
        exti.EXTI_Trigger = EXTI_Trigger_Rising;
        break;
    case EXTI_HAL_TRIGGER_FALLING:
        exti.EXTI_Trigger = EXTI_Trigger_Falling;
        break;
    case EXTI_HAL_TRIGGER_BOTH:
    default:
        exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
        break;
    }
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    EXTI_ClearITPendingBit(line_mask);
}

uint8_t exti_hal_get_it_status(uint32_t line_mask)
{
    return (uint8_t)(EXTI_GetITStatus(line_mask) == SET);
}

void exti_hal_clear_pending(uint32_t line_mask)
{
    EXTI_ClearITPendingBit(line_mask);
}

static IRQn_Type exti_hal_irq_from_channel(exti_hal_irq_channel_t channel)
{
    static const IRQn_Type map[EXTI_HAL_IRQ_CHANNEL_COUNT] = {
        EXTI0_IRQn,
        EXTI1_IRQn,
        EXTI2_IRQn,
        EXTI3_IRQn,
        EXTI4_IRQn,
        EXTI9_5_IRQn,
        EXTI15_10_IRQn
    };

    if ((uint32_t)channel < (uint32_t)EXTI_HAL_IRQ_CHANNEL_COUNT) {
        return map[channel];
    }
    return EXTI0_IRQn;
}

void exti_hal_enable_irq(exti_hal_irq_channel_t channel, uint8_t priority)
{
    NVIC_InitTypeDef nvic;

    nvic.NVIC_IRQChannel = exti_hal_irq_from_channel(channel);
    nvic.NVIC_IRQChannelPreemptionPriority = priority;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

exti_hal_irq_channel_t exti_hal_irq_channel_from_line(uint8_t line)
{
    if (line <= 4U) {
        return (exti_hal_irq_channel_t)line;
    }
    if (line <= 9U) {
        return EXTI_HAL_IRQ_LINE5_9;
    }
    return EXTI_HAL_IRQ_LINE10_15;
}
