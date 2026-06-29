#ifndef ADC_HAL_H
#define ADC_HAL_H

#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "hal_status.h"

#if HAL_ADC_ENABLE

#include "stm32f10x_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_HAL_DEFAULT_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US
#define ADC_HAL_DMA_BUFFER_MAX 16U
#define ADC_HAL_SAMPLE_TIME_DEFAULT 0U
#define ADC_HAL_SAMPLE_TIME_LONG ADC_SampleTime_239Cycles5

typedef struct adc_hal_config {
    ADC_TypeDef *instance;
    uint32_t timeout_us;
} adc_hal_config_t;

typedef struct adc_hal_channel_cfg {
    uint8_t channel;
    hal_pin_t pin;
    uint8_t sample_time;
    uint8_t rank;
} adc_hal_channel_cfg_t;

hal_status_t adc_hal_init(const adc_hal_config_t *cfg);
hal_status_t adc_hal_config_channel(const adc_hal_channel_cfg_t *ch_cfg);
hal_status_t adc_hal_read(uint8_t channel, uint16_t *value);

#if HAL_ADC_ENABLE_DMA
#include "stm32f10x_dma.h"

typedef struct adc_hal_dma_config {
    DMA_Channel_TypeDef *dma_channel;
    uint16_t *buffer;
    uint16_t buffer_len;
    const uint8_t *channels;
    uint8_t channel_count;
    uint8_t continuous;
} adc_hal_dma_config_t;

hal_status_t adc_hal_dma_setup(const adc_hal_dma_config_t *cfg);
hal_status_t adc_hal_dma_start(void);
hal_status_t adc_hal_dma_stop(void);
uint8_t adc_hal_dma_is_running(void);
const uint16_t *adc_hal_dma_get_buffer(void);
void adc_hal_dma_irq_handler(DMA_Channel_TypeDef *channel);
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
