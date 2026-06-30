/**
 * @file irq_handlers.c
 * @brief NVIC ISR stubs that delegate to HAL IRQ handlers (USART, TIM, optional DMA).
 */

#include "timer_hal.h"
#include "usart_hal.h"
#include "hal_features.h"
#include "irq_event.h"
#include "stm32f10x.h"
#include "stm32f10x_exti.h"

#if HAL_SPI_ENABLE_DMA
#include "spi_hal.h"
#include "spi_hal_private.h"
#endif

#if HAL_ADC_ENABLE_DMA
#include "adc_hal.h"
#include "adc_hal_private.h"
#endif

#if HAL_USART_ENABLE_DMA
#include "usart_hal_private.h"
#endif

__attribute__((weak)) void key_exti_irq_handler(uint32_t pending_mask)
{
    /* 默认无按键驱动，避免链接错误 */
    (void)pending_mask;
}

static void handle_exti_group(uint32_t line_mask)
{
    uint32_t pending = EXTI->PR & line_mask;

    if (pending == 0U) {
        return;
    }

    key_exti_irq_handler(pending);
    EXTI->PR = pending;
    irq_event_post_from_isr(IRQ_EVENT_SOURCE_KEY_EXTI, pending);
}

/**
 * @brief USART1 RX/TX/error interrupt entry point.
 */
void USART1_IRQHandler(void)
{
    usart_hal_irq_handler(HAL_USART_ID_1);
}

/**
 * @brief USART2 RX/TX/error interrupt entry point.
 */
void USART2_IRQHandler(void)
{
    usart_hal_irq_handler(HAL_USART_ID_2);
}

/**
 * @brief USART3 RX/TX/error interrupt entry point.
 */
void USART3_IRQHandler(void)
{
    usart_hal_irq_handler(HAL_USART_ID_3);
}

#if HAL_ADC_ENABLE_DMA
/**
 * @brief DMA1 channel 1 interrupt (ADC DMA when enabled).
 */
void DMA1_Channel1_IRQHandler(void)
{
    adc_hal_dma_irq_handler(DMA1_Channel1);
}
#endif

#if HAL_SPI_ENABLE_DMA
/**
 * @brief DMA1 channel 2 interrupt (SPI RX DMA when enabled).
 */
void DMA1_Channel2_IRQHandler(void)
{
    spi_hal_dma_irq_handler(DMA1_Channel2);
}

/**
 * @brief DMA1 channel 3 interrupt (SPI TX DMA when enabled).
 */
void DMA1_Channel3_IRQHandler(void)
{
    spi_hal_dma_irq_handler(DMA1_Channel3);
}

/**
 * @brief DMA1 channel 5 interrupt (SPI DMA when enabled).
 */
void DMA1_Channel5_IRQHandler(void)
{
    spi_hal_dma_irq_handler(DMA1_Channel5);
}
#endif

#if HAL_USART_ENABLE_DMA || HAL_SPI_ENABLE_DMA
/**
 * @brief DMA1 channel 4 interrupt (USART/SPI TX DMA when enabled).
 */
void DMA1_Channel4_IRQHandler(void)
{
#if HAL_USART_ENABLE_DMA
    usart_hal_dma_irq_handler(DMA1_Channel4);
#endif
#if HAL_SPI_ENABLE_DMA
    spi_hal_dma_irq_handler(DMA1_Channel4);
#endif
}

/**
 * @brief DMA1 channel 7 interrupt (USART/SPI DMA when enabled).
 */
void DMA1_Channel7_IRQHandler(void)
{
#if HAL_USART_ENABLE_DMA
    usart_hal_dma_irq_handler(DMA1_Channel7);
#endif
#if HAL_SPI_ENABLE_DMA
    spi_hal_dma_irq_handler(DMA1_Channel7);
#endif
}
#endif

/**
 * @brief TIM1 update interrupt entry point.
 */
void TIM1_UP_IRQHandler(void)
{
    timer_hal_irq_handler(HAL_TIMER_ID_1);
}

/**
 * @brief TIM2 update interrupt entry point.
 */
void TIM2_IRQHandler(void)
{
    timer_hal_irq_handler(HAL_TIMER_ID_2);
}

/**
 * @brief TIM3 update interrupt entry point.
 */
void TIM3_IRQHandler(void)
{
    timer_hal_irq_handler(HAL_TIMER_ID_3);
}

/**
 * @brief TIM4 update interrupt entry point.
 */
void TIM4_IRQHandler(void)
{
    timer_hal_irq_handler(HAL_TIMER_ID_4);
}

void EXTI0_IRQHandler(void)
{
    handle_exti_group(EXTI_Line0);
}

void EXTI1_IRQHandler(void)
{
    handle_exti_group(EXTI_Line1);
}

void EXTI2_IRQHandler(void)
{
    handle_exti_group(EXTI_Line2);
}

void EXTI3_IRQHandler(void)
{
    handle_exti_group(EXTI_Line3);
}

void EXTI4_IRQHandler(void)
{
    handle_exti_group(EXTI_Line4);
}

void EXTI9_5_IRQHandler(void)
{
    handle_exti_group(EXTI_Line5 | EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9);
}

void EXTI15_10_IRQHandler(void)
{
    handle_exti_group(EXTI_Line10 | EXTI_Line11 | EXTI_Line12 | EXTI_Line13 | EXTI_Line14 | EXTI_Line15);
}
