#include "timer_hal.h"
#include "usart_hal.h"
#include "hal_features.h"
#include "stm32f10x.h"

#if HAL_SPI_ENABLE_DMA
#include "spi_hal.h"
#endif

#if HAL_ADC_ENABLE_DMA
#include "adc_hal.h"
#endif

void USART1_IRQHandler(void)
{
    usart_hal_irq_handler(USART1);
}

void USART2_IRQHandler(void)
{
    usart_hal_irq_handler(USART2);
}

void USART3_IRQHandler(void)
{
    usart_hal_irq_handler(USART3);
}

#if HAL_ADC_ENABLE_DMA
void DMA1_Channel1_IRQHandler(void)
{
    adc_hal_dma_irq_handler(DMA1_Channel1);
}
#endif

#if HAL_SPI_ENABLE_DMA
void DMA1_Channel2_IRQHandler(void)
{
    spi_hal_dma_irq_handler(DMA1_Channel2);
}

void DMA1_Channel3_IRQHandler(void)
{
    spi_hal_dma_irq_handler(DMA1_Channel3);
}

void DMA1_Channel5_IRQHandler(void)
{
    spi_hal_dma_irq_handler(DMA1_Channel5);
}
#endif

#if HAL_USART_ENABLE_DMA || HAL_SPI_ENABLE_DMA
void DMA1_Channel4_IRQHandler(void)
{
#if HAL_USART_ENABLE_DMA
    usart_hal_dma_irq_handler(DMA1_Channel4);
#endif
#if HAL_SPI_ENABLE_DMA
    spi_hal_dma_irq_handler(DMA1_Channel4);
#endif
}

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

void TIM1_UP_IRQHandler(void)
{
    timer_hal_irq_handler(TIM1);
}

void TIM2_IRQHandler(void)
{
    timer_hal_irq_handler(TIM2);
}

void TIM3_IRQHandler(void)
{
    timer_hal_irq_handler(TIM3);
}

void TIM4_IRQHandler(void)
{
    timer_hal_irq_handler(TIM4);
}
