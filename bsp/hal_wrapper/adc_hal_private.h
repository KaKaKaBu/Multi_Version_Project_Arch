#ifndef ADC_HAL_PRIVATE_H
#define ADC_HAL_PRIVATE_H

#include "hal_features.h"

#if HAL_ADC_ENABLE

#include "stm32f10x_adc.h"

typedef struct adc_hal_runtime {
    ADC_TypeDef *instance;
    uint32_t timeout_us;
    uint8_t configured;
    uint8_t channel_count;
    uint8_t channels[16U];
    uint8_t sample_time_by_channel[16U];
#if HAL_ADC_ENABLE_DMA
    uint8_t dma_running;
    uint16_t *dma_buffer;
    uint16_t dma_buffer_len;
    DMA_Channel_TypeDef *dma_channel;
#endif
} adc_hal_runtime_t;

adc_hal_runtime_t *adc_hal_runtime_get(void);

#endif

#endif
