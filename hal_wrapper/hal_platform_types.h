#ifndef HAL_PLATFORM_TYPES_H
#define HAL_PLATFORM_TYPES_H

#include <stdint.h>

typedef uint8_t hal_gpio_port_t;
typedef uint8_t hal_gpio_pin_t;
typedef uint8_t hal_i2c_id_t;
typedef uint8_t hal_usart_id_t;
typedef uint8_t hal_timer_id_t;
typedef uint8_t hal_adc_id_t;
typedef uint8_t hal_adc_channel_t;
typedef uint8_t hal_spi_id_t;
typedef uint8_t hal_dma_channel_id_t;

#define HAL_PORT_A 0U
#define HAL_PORT_B 1U
#define HAL_PORT_C 2U
#define HAL_PORT_D 3U
#define HAL_PORT_E 4U

#define HAL_PORT_P0 0U
#define HAL_PORT_P1 1U
#define HAL_PORT_P2 2U
#define HAL_PORT_P3 3U

#define HAL_PIN_0 0U
#define HAL_PIN_1 1U
#define HAL_PIN_2 2U
#define HAL_PIN_3 3U
#define HAL_PIN_4 4U
#define HAL_PIN_5 5U
#define HAL_PIN_6 6U
#define HAL_PIN_7 7U
#define HAL_PIN_8 8U
#define HAL_PIN_9 9U
#define HAL_PIN_10 10U
#define HAL_PIN_11 11U
#define HAL_PIN_12 12U
#define HAL_PIN_13 13U
#define HAL_PIN_14 14U
#define HAL_PIN_15 15U

#define HAL_I2C_ID_1 1U
#define HAL_I2C_ID_2 2U

#define HAL_USART_ID_1 1U
#define HAL_USART_ID_2 2U
#define HAL_USART_ID_3 3U

#define HAL_TIMER_ID_1 1U
#define HAL_TIMER_ID_2 2U
#define HAL_TIMER_ID_3 3U
#define HAL_TIMER_ID_4 4U

#define HAL_ADC_ID_1 1U
#define HAL_ADC_ID_2 2U

#define HAL_SPI_ID_1 1U
#define HAL_SPI_ID_2 2U

#define HAL_DMA_CH_1 1U
#define HAL_DMA_CH_2 2U
#define HAL_DMA_CH_3 3U
#define HAL_DMA_CH_4 4U
#define HAL_DMA_CH_5 5U
#define HAL_DMA_CH_6 6U
#define HAL_DMA_CH_7 7U

#define HAL_ADC_CH_0 0U
#define HAL_ADC_CH_1 1U
#define HAL_ADC_CH_2 2U
#define HAL_ADC_CH_3 3U
#define HAL_ADC_CH_4 4U
#define HAL_ADC_CH_5 5U
#define HAL_ADC_CH_6 6U
#define HAL_ADC_CH_7 7U
#define HAL_ADC_CH_8 8U
#define HAL_ADC_CH_9 9U
#define HAL_ADC_CH_10 10U
#define HAL_ADC_CH_11 11U
#define HAL_ADC_CH_12 12U
#define HAL_ADC_CH_13 13U
#define HAL_ADC_CH_14 14U
#define HAL_ADC_CH_15 15U

#endif
