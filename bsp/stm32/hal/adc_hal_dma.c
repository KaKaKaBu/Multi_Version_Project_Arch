#if HAL_ADC_ENABLE_DMA
#include "adc_hal_private.h"
#include "adc_hal.h"
#include "hal_common.h"
#include "stm32f1_hal_map.h"
#include "misc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_rcc.h"
#else
typedef char adc_hal_dma_disabled[-1];
#endif

#if HAL_ADC_ENABLE_DMA

static uint32_t adc_hal_dma_tc_flag(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel1) {
        return DMA1_IT_TC1;
    }
    if (channel == DMA1_Channel2) {
        return DMA1_IT_TC2;
    }
    if (channel == DMA1_Channel3) {
        return DMA1_IT_TC3;
    }
    if (channel == DMA1_Channel4) {
        return DMA1_IT_TC4;
    }
    if (channel == DMA1_Channel5) {
        return DMA1_IT_TC5;
    }
    if (channel == DMA1_Channel6) {
        return DMA1_IT_TC6;
    }
    return DMA1_IT_TC7;
}

static IRQn_Type adc_hal_dma_irqn(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel1) {
        return DMA1_Channel1_IRQn;
    }
    if (channel == DMA1_Channel2) {
        return DMA1_Channel2_IRQn;
    }
    if (channel == DMA1_Channel3) {
        return DMA1_Channel3_IRQn;
    }
    if (channel == DMA1_Channel4) {
        return DMA1_Channel4_IRQn;
    }
    if (channel == DMA1_Channel5) {
        return DMA1_Channel5_IRQn;
    }
    if (channel == DMA1_Channel6) {
        return DMA1_Channel6_IRQn;
    }
    return DMA1_Channel7_IRQn;
}

hal_status_t adc_hal_dma_setup(const adc_hal_dma_config_t *cfg)
{
    DMA_Channel_TypeDef *dma_channel;
    DMA_InitTypeDef dma;
    adc_hal_runtime_t *runtime = adc_hal_runtime_get();
    uint8_t i;

    if ((cfg == 0) || (runtime == 0) || (runtime->configured == 0U) ||
        (cfg->buffer == 0) || (cfg->channels == 0) || (cfg->channel_count == 0U)) {
        return HAL_ERR_PARAM;
    }

    dma_channel = stm32f1_dma_channel(cfg->dma_channel);
    if (dma_channel == 0) {
        return HAL_ERR_PARAM;
    }

    if ((cfg->channel_count > ADC_HAL_DMA_BUFFER_MAX) || (cfg->buffer_len < cfg->channel_count)) {
        return HAL_ERR_PARAM;
    }

    for (i = 0U; i < cfg->channel_count; ++i) {
        ADC_RegularChannelConfig(runtime->instance, cfg->channels[i], (uint8_t)(i + 1U), ADC_SampleTime_55Cycles5);
        runtime->channels[i] = cfg->channels[i];
        if (cfg->channels[i] < ADC_HAL_DMA_BUFFER_MAX) {
            runtime->sample_time_by_channel[cfg->channels[i]] = ADC_SampleTime_55Cycles5;
        }
    }
    runtime->channel_count = cfg->channel_count;

    runtime->dma_buffer = cfg->buffer;
    runtime->dma_buffer_len = cfg->buffer_len;
    runtime->dma_channel = dma_channel;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(dma_channel);
    DMA_StructInit(&dma);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&runtime->instance->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)cfg->buffer;
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma.DMA_BufferSize = cfg->channel_count;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma.DMA_Mode = (cfg->continuous != 0U) ? DMA_Mode_Circular : DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(dma_channel, &dma);

    ADC_DMACmd(runtime->instance, ENABLE);
    ADC_InitTypeDef adc;
    ADC_StructInit(&adc);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = ENABLE;
    adc.ADC_ContinuousConvMode = (cfg->continuous != 0U) ? ENABLE : DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = cfg->channel_count;
    ADC_Init(runtime->instance, &adc);

    return HAL_OK;
}

hal_status_t adc_hal_dma_start(void)
{
    adc_hal_runtime_t *runtime = adc_hal_runtime_get();

    if ((runtime == 0) || (runtime->dma_channel == 0) || (runtime->dma_buffer == 0)) {
        return HAL_ERR_PARAM;
    }

    runtime->dma_running = 1U;
    DMA_Cmd(runtime->dma_channel, ENABLE);
    ADC_SoftwareStartConvCmd(runtime->instance, ENABLE);

    return HAL_OK;
}

hal_status_t adc_hal_dma_stop(void)
{
    adc_hal_runtime_t *runtime = adc_hal_runtime_get();

    if ((runtime == 0) || (runtime->dma_channel == 0)) {
        return HAL_ERR_PARAM;
    }

    ADC_SoftwareStartConvCmd(runtime->instance, DISABLE);
    DMA_Cmd(runtime->dma_channel, DISABLE);
    runtime->dma_running = 0U;

    return HAL_OK;
}

uint8_t adc_hal_dma_is_running(void)
{
    adc_hal_runtime_t *runtime = adc_hal_runtime_get();

    if (runtime == 0) {
        return 0U;
    }

    return runtime->dma_running;
}

const uint16_t *adc_hal_dma_get_buffer(void)
{
    adc_hal_runtime_t *runtime = adc_hal_runtime_get();

    if (runtime == 0) {
        return 0;
    }

    return runtime->dma_buffer;
}

void adc_hal_dma_irq_handler(DMA_Channel_TypeDef *channel)
{
    adc_hal_runtime_t *runtime = adc_hal_runtime_get();

    if ((runtime == 0) || (runtime->dma_channel != channel)) {
        return;
    }

    if (DMA_GetITStatus(adc_hal_dma_tc_flag(channel)) == RESET) {
        return;
    }

    DMA_ClearITPendingBit(adc_hal_dma_tc_flag(channel));
    runtime->dma_running = 0U;
}

#endif
