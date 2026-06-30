#ifndef STM32F1_HAL_MAP_H
#define STM32F1_HAL_MAP_H

#include "hal_platform_types.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_spi.h"

GPIO_TypeDef *stm32f1_gpio_port(hal_gpio_port_t port);
uint16_t stm32f1_gpio_pin(hal_gpio_pin_t pin);
I2C_TypeDef *stm32f1_i2c(hal_i2c_id_t instance);
USART_TypeDef *stm32f1_usart(hal_usart_id_t instance);
TIM_TypeDef *stm32f1_timer(hal_timer_id_t instance);
ADC_TypeDef *stm32f1_adc(hal_adc_id_t instance);
SPI_TypeDef *stm32f1_spi(hal_spi_id_t instance);
DMA_Channel_TypeDef *stm32f1_dma_channel(hal_dma_channel_id_t channel);
hal_dma_channel_id_t stm32f1_dma_channel_id(DMA_Channel_TypeDef *channel);

#endif
