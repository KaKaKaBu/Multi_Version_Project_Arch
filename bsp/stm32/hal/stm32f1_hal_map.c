#include "stm32f1_hal_map.h"

GPIO_TypeDef *stm32f1_gpio_port(hal_gpio_port_t port)
{
    switch (port) {
    case HAL_PORT_A: return GPIOA;
    case HAL_PORT_B: return GPIOB;
    case HAL_PORT_C: return GPIOC;
    case HAL_PORT_D: return GPIOD;
    case HAL_PORT_E: return GPIOE;
    default: return 0;
    }
}

uint16_t stm32f1_gpio_pin(hal_gpio_pin_t pin)
{
    return (pin < 16U) ? (uint16_t)(1U << pin) : 0U;
}

I2C_TypeDef *stm32f1_i2c(hal_i2c_id_t instance)
{
    switch (instance) {
    case HAL_I2C_ID_1: return I2C1;
    case HAL_I2C_ID_2: return I2C2;
    default: return 0;
    }
}

USART_TypeDef *stm32f1_usart(hal_usart_id_t instance)
{
    switch (instance) {
    case HAL_USART_ID_1: return USART1;
    case HAL_USART_ID_2: return USART2;
    case HAL_USART_ID_3: return USART3;
    default: return 0;
    }
}

TIM_TypeDef *stm32f1_timer(hal_timer_id_t instance)
{
    switch (instance) {
    case HAL_TIMER_ID_1: return TIM1;
    case HAL_TIMER_ID_2: return TIM2;
    case HAL_TIMER_ID_3: return TIM3;
    case HAL_TIMER_ID_4: return TIM4;
    default: return 0;
    }
}

ADC_TypeDef *stm32f1_adc(hal_adc_id_t instance)
{
    switch (instance) {
    case HAL_ADC_ID_1: return ADC1;
    case HAL_ADC_ID_2: return ADC2;
    default: return 0;
    }
}

SPI_TypeDef *stm32f1_spi(hal_spi_id_t instance)
{
    switch (instance) {
    case HAL_SPI_ID_1: return SPI1;
    case HAL_SPI_ID_2: return SPI2;
    default: return 0;
    }
}

DMA_Channel_TypeDef *stm32f1_dma_channel(hal_dma_channel_id_t channel)
{
    switch (channel) {
    case HAL_DMA_CH_1: return DMA1_Channel1;
    case HAL_DMA_CH_2: return DMA1_Channel2;
    case HAL_DMA_CH_3: return DMA1_Channel3;
    case HAL_DMA_CH_4: return DMA1_Channel4;
    case HAL_DMA_CH_5: return DMA1_Channel5;
    case HAL_DMA_CH_6: return DMA1_Channel6;
    case HAL_DMA_CH_7: return DMA1_Channel7;
    default: return 0;
    }
}

hal_dma_channel_id_t stm32f1_dma_channel_id(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel1) return HAL_DMA_CH_1;
    if (channel == DMA1_Channel2) return HAL_DMA_CH_2;
    if (channel == DMA1_Channel3) return HAL_DMA_CH_3;
    if (channel == DMA1_Channel4) return HAL_DMA_CH_4;
    if (channel == DMA1_Channel5) return HAL_DMA_CH_5;
    if (channel == DMA1_Channel6) return HAL_DMA_CH_6;
    if (channel == DMA1_Channel7) return HAL_DMA_CH_7;
    return 0U;
}
