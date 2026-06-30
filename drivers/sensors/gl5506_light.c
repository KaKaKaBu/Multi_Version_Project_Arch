/**
 * @file gl5506_light.c
 * @brief GL5506 光敏电阻 ADC 采集，输出环境光照百分比 0–100。
 */

#include "analog_probe_if.h"
#include "adc_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

#if HAL_ADC_ENABLE

#define GL5506_FILTER_SIZE 8U
#define GL5506_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG

static float gl5506_filter_buffer[GL5506_FILTER_SIZE];
static unsigned char gl5506_filter_index;
static unsigned char gl5506_filter_count;
static float gl5506_value_cache;
static const adc_channel_driver_config_t *gl5506_config;

static float gl5506_adc_to_percent(uint16_t adc_raw)
{
    float percent;

    if ((gl5506_config != 0) && (gl5506_config->invert != 0U)) {
        percent = (float)adc_raw / 4095.0f * 100.0f;
    } else {
        percent = (1.0f - (float)adc_raw / 4095.0f) * 100.0f;
    }

    if (percent < 0.0f) {
        percent = 0.0f;
    }
    if (percent > 100.0f) {
        percent = 100.0f;
    }

    return percent;
}

static void gl5506_init(const void *config)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;
    unsigned char i;

    gl5506_config = (const adc_channel_driver_config_t *)config;
    if (gl5506_config == 0) {
        return;
    }

    adc_cfg.instance = gl5506_config->instance;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = gl5506_config->channel;
    ch_cfg.pin = gl5506_config->pin;
    ch_cfg.sample_time = GL5506_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    for (i = 0U; i < GL5506_FILTER_SIZE; i++) {
        gl5506_filter_buffer[i] = 0.0f;
    }
    gl5506_filter_index = 0U;
    gl5506_filter_count = 0U;
    gl5506_value_cache = 0.0f;
}

static float gl5506_read_value(void)
{
    uint16_t adc_raw;
    float sum;
    unsigned char i;

    if ((gl5506_config == 0) || (adc_hal_read(gl5506_config->channel, &adc_raw) != HAL_OK)) {
        return gl5506_value_cache;
    }

    gl5506_filter_buffer[gl5506_filter_index] = gl5506_adc_to_percent(adc_raw);
    gl5506_filter_index = (unsigned char)((gl5506_filter_index + 1U) % GL5506_FILTER_SIZE);
    if (gl5506_filter_count < GL5506_FILTER_SIZE) {
        gl5506_filter_count++;
    }

    sum = 0.0f;
    for (i = 0U; i < gl5506_filter_count; i++) {
        sum += gl5506_filter_buffer[i];
    }

    gl5506_value_cache = sum / (float)gl5506_filter_count;
    return gl5506_value_cache;
}

static const analog_probe_t gl5506_drv = {
    "gl5506",
    gl5506_init,
    gl5506_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, gl5506_drv);

#else
typedef char gl5506_adc_disabled[-1];
#endif
