#include "adc_hal.h"

#if !HAL_ADC_ENABLE
typedef char adc_hal_disabled[-1];
#else

#include "adc_hal_private.h"
#include "hal_common.h"
#include "stm32f1_hal_map.h"
#include "stm32f10x_rcc.h"

#define ADC_HAL_CHANNEL_TABLE_MAX 16U

static adc_hal_runtime_t adc_hal_runtime;

adc_hal_runtime_t *adc_hal_runtime_get(void)
{
    return &adc_hal_runtime;
}

static void adc_hal_clock_enable(ADC_TypeDef *ADCx)
{
    if (ADCx == ADC1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    } else if (ADCx == ADC2) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
    }
}

static uint8_t adc_hal_sample_time_value(uint8_t sample_time)
{
    if (sample_time == ADC_HAL_SAMPLE_TIME_DEFAULT) {
        return ADC_SampleTime_55Cycles5;
    }
    if (sample_time == ADC_HAL_SAMPLE_TIME_LONG) {
        return ADC_SampleTime_239Cycles5;
    }

    return sample_time;
}

typedef struct adc_hal_flag_ctx {
    ADC_TypeDef *instance;
    uint32_t flag;
} adc_hal_flag_ctx_t;

static uint8_t adc_hal_poll_flag(void *ctx)
{
    const adc_hal_flag_ctx_t *flag_ctx = (const adc_hal_flag_ctx_t *)ctx;

    if (flag_ctx == 0) {
        return 0U;
    }

    return (ADC_GetFlagStatus(flag_ctx->instance, flag_ctx->flag) != RESET) ? 1U : 0U;
}

static hal_status_t adc_hal_wait_flag(ADC_TypeDef *ADCx, uint32_t flag, uint32_t timeout_us)
{
    adc_hal_flag_ctx_t ctx;

    ctx.instance = ADCx;
    ctx.flag = flag;
    return hal_wait_flag_us(adc_hal_poll_flag, &ctx, timeout_us);
}

static hal_status_t adc_hal_calibrate(ADC_TypeDef *ADCx, uint32_t timeout_us)
{
    (void)timeout_us;

    ADC_ResetCalibration(ADCx);
    while (ADC_GetResetCalibrationStatus(ADCx) != RESET) {
    }

    ADC_StartCalibration(ADCx);
    while (ADC_GetCalibrationStatus(ADCx) != RESET) {
    }

    return HAL_OK;
}

hal_status_t adc_hal_init(const adc_hal_config_t *cfg)
{
    ADC_TypeDef *ADCx;
    ADC_InitTypeDef adc;

    if (cfg == 0) {
        return HAL_ERR_PARAM;
    }

    ADCx = stm32f1_adc(cfg->instance);
    if (ADCx == 0) {
        return HAL_ERR_PARAM;
    }

    adc_hal_runtime.instance = ADCx;
    adc_hal_runtime.timeout_us = (cfg->timeout_us == 0U) ? ADC_HAL_DEFAULT_TIMEOUT_US : cfg->timeout_us;
    adc_hal_runtime.configured = 0U;
    adc_hal_runtime.channel_count = 0U;
    for (uint8_t i = 0U; i < ADC_HAL_CHANNEL_TABLE_MAX; ++i) {
        adc_hal_runtime.channels[i] = 0U;
        adc_hal_runtime.sample_time_by_channel[i] = ADC_SampleTime_55Cycles5;
    }

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    adc_hal_clock_enable(ADCx);

    ADC_DeInit(ADCx);
    ADC_StructInit(&adc);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADCx, &adc);
    ADC_Cmd(ADCx, ENABLE);

    if (adc_hal_calibrate(ADCx, adc_hal_runtime.timeout_us) != HAL_OK) {
        return HAL_ERR_TIMEOUT;
    }

    adc_hal_runtime.configured = 1U;
    return HAL_OK;
}

hal_status_t adc_hal_config_channel(const adc_hal_channel_cfg_t *ch_cfg)
{
    hal_pin_t analog_pin;
    uint8_t rank;

    if ((ch_cfg == 0) || (adc_hal_runtime.configured == 0U)) {
        return HAL_ERR_PARAM;
    }

    analog_pin = ch_cfg->pin;
    analog_pin.mode = GPIO_HAL_MODE_ANALOG;
    gpio_hal_config_pin(&analog_pin);

    rank = (ch_cfg->rank == 0U) ? 1U : ch_cfg->rank;
    ADC_RegularChannelConfig(adc_hal_runtime.instance, ch_cfg->channel, rank,
                             adc_hal_sample_time_value(ch_cfg->sample_time));

    if (rank <= ADC_HAL_CHANNEL_TABLE_MAX) {
        adc_hal_runtime.channels[rank - 1U] = ch_cfg->channel;
        if (ch_cfg->channel < ADC_HAL_CHANNEL_TABLE_MAX) {
            adc_hal_runtime.sample_time_by_channel[ch_cfg->channel] = adc_hal_sample_time_value(ch_cfg->sample_time);
        }
        if (rank > adc_hal_runtime.channel_count) {
            adc_hal_runtime.channel_count = rank;
        }
    }

    return HAL_OK;
}

static uint8_t adc_hal_sample_time_for_channel(uint8_t channel)
{
    if (channel < ADC_HAL_CHANNEL_TABLE_MAX) {
        return adc_hal_runtime.sample_time_by_channel[channel];
    }

    return ADC_SampleTime_55Cycles5;
}

hal_status_t adc_hal_read(uint8_t channel, uint16_t *value)
{
    hal_status_t status;

    if ((value == 0) || (adc_hal_runtime.configured == 0U)) {
        return HAL_ERR_PARAM;
    }

    ADC_RegularChannelConfig(adc_hal_runtime.instance, channel, 1U, adc_hal_sample_time_for_channel(channel));
    ADC_SoftwareStartConvCmd(adc_hal_runtime.instance, ENABLE);

    status = adc_hal_wait_flag(adc_hal_runtime.instance, ADC_FLAG_EOC, adc_hal_runtime.timeout_us);
    if (status != HAL_OK) {
        return status;
    }

    *value = (uint16_t)ADC_GetConversionValue(adc_hal_runtime.instance);
    return HAL_OK;
}

#endif
