#include "bp_if.h"
#include "adc_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

#if HAL_ADC_ENABLE

#define MSP20_FILTER_SIZE 8U
#define MSP20_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG

static float msp20_volt_filter[MSP20_FILTER_SIZE];
static unsigned char msp20_filter_index;
static unsigned char msp20_filter_count;
static unsigned char msp20_sys_cache;
static unsigned char msp20_dia_cache;
static const msp20_driver_config_t *msp20_config;

static float msp20_adc_to_voltage(uint16_t adc_raw)
{
    return (float)adc_raw / 4096.0f * 3.3f;
}

static float msp20_read_voltage_avg(void)
{
    uint16_t adc_raw;
    float sum;
    unsigned char i;

    if ((msp20_config == 0) || (adc_hal_read(msp20_config->adc.channel, &adc_raw) != HAL_OK)) {
        return 0.0f;
    }

    msp20_volt_filter[msp20_filter_index] = msp20_adc_to_voltage(adc_raw);
    msp20_filter_index = (unsigned char)((msp20_filter_index + 1U) % MSP20_FILTER_SIZE);
    if (msp20_filter_count < MSP20_FILTER_SIZE) {
        msp20_filter_count++;
    }

    sum = 0.0f;
    for (i = 0U; i < msp20_filter_count; ++i) {
        sum += msp20_volt_filter[i];
    }

    return sum / (float)msp20_filter_count;
}

static unsigned char msp20_volt_to_sys(float voltage)
{
    int value;

    value = (int)(msp20_config->sys_k * voltage + msp20_config->sys_offset);
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }

    return (unsigned char)value;
}

static unsigned char msp20_volt_to_dia(float voltage)
{
    int value;

    value = (int)(msp20_config->dia_k * voltage + msp20_config->dia_offset);
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }

    return (unsigned char)value;
}

static void msp20_refresh(void)
{
    float voltage;

    voltage = msp20_read_voltage_avg();
    msp20_sys_cache = msp20_volt_to_sys(voltage);
    msp20_dia_cache = msp20_volt_to_dia(voltage * msp20_config->dia_ratio);
}

static void msp20_init(const void *config)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;
    unsigned char i;

    msp20_config = (const msp20_driver_config_t *)config;
    if (msp20_config == 0) {
        return;
    }

    adc_cfg.instance = msp20_config->adc.instance;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = msp20_config->adc.channel;
    ch_cfg.pin = msp20_config->adc.pin;
    ch_cfg.sample_time = MSP20_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    for (i = 0U; i < MSP20_FILTER_SIZE; ++i) {
        msp20_volt_filter[i] = 0.0f;
    }
    msp20_filter_index = 0U;
    msp20_filter_count = 0U;
    msp20_sys_cache = 120U;
    msp20_dia_cache = 80U;
}

static unsigned char msp20_read_systolic(void)
{
    msp20_refresh();
    return msp20_sys_cache;
}

static unsigned char msp20_read_diastolic(void)
{
    msp20_refresh();
    return msp20_dia_cache;
}

static const blood_pressure_sensor_t msp20_drv = {
    "msp20",
    msp20_init,
    msp20_read_systolic,
    msp20_read_diastolic
};

REGISTER_DRIVER(BLOOD_PRESSURE, msp20_drv);

#else
typedef char msp20_adc_disabled[-1];
#endif
