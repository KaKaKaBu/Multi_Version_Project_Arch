/**
 * @file water_level.c
 * @brief 模拟式水位传感器 ADC 采集，输出水位百分比 0–100。
 */

#include "analog_probe_if.h"
#include "adc_hal.h"
#include "board_config.h"
#include "driver_core.h"

#if HAL_ADC_ENABLE

#define WATER_LEVEL_FILTER_SIZE 8U
#define WATER_LEVEL_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG

static float water_level_filter_buffer[WATER_LEVEL_FILTER_SIZE];
static unsigned char water_level_filter_index;
static unsigned char water_level_filter_count;
static float water_level_value_cache;

static float water_level_adc_to_percent(uint16_t adc_raw)
{
    float percent;

#if BOARD_WATER_LEVEL_INVERT
    percent = (1.0f - (float)adc_raw / 4095.0f) * 100.0f;
#else
    percent = (float)adc_raw / 4095.0f * 100.0f;
#endif

    if (percent < 0.0f) {
        percent = 0.0f;
    }
    if (percent > 100.0f) {
        percent = 100.0f;
    }

    return percent;
}

static void water_level_init(void)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;
    unsigned char i;

    adc_cfg.instance = BOARD_ADC_INSTANCE;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = BOARD_WATER_LEVEL_ADC_CHANNEL;
    ch_cfg.pin = board_water_level_adc_pin;
    ch_cfg.sample_time = WATER_LEVEL_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    for (i = 0U; i < WATER_LEVEL_FILTER_SIZE; i++) {
        water_level_filter_buffer[i] = 0.0f;
    }
    water_level_filter_index = 0U;
    water_level_filter_count = 0U;
    water_level_value_cache = 0.0f;
}

static float water_level_read_value(void)
{
    uint16_t adc_raw;
    float sum;
    unsigned char i;

    if (adc_hal_read(BOARD_WATER_LEVEL_ADC_CHANNEL, &adc_raw) != HAL_OK) {
        return water_level_value_cache;
    }

    water_level_filter_buffer[water_level_filter_index] = water_level_adc_to_percent(adc_raw);
    water_level_filter_index = (unsigned char)((water_level_filter_index + 1U) % WATER_LEVEL_FILTER_SIZE);
    if (water_level_filter_count < WATER_LEVEL_FILTER_SIZE) {
        water_level_filter_count++;
    }

    sum = 0.0f;
    for (i = 0U; i < water_level_filter_count; i++) {
        sum += water_level_filter_buffer[i];
    }

    water_level_value_cache = sum / (float)water_level_filter_count;
    return water_level_value_cache;
}

static const analog_probe_t water_level_drv = {
    "water_level",
    water_level_init,
    water_level_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, water_level_drv);

#else
typedef char water_level_adc_disabled[-1];
#endif
