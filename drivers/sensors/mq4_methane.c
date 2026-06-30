/**
 * @file mq4_methane.c
 * @brief MQ-4 沼气/甲烷传感器 ADC 采集，输出 CH4 浓度 ppm。
 */

#include "gas_if.h"
#include "adc_hal.h"
#include "timer_hal.h"
#include "driver_configs.h"
#include "driver_core.h"
#include <math.h>

#if HAL_ADC_ENABLE

#define MQ4_READ_TIMES 10U
#define MQ4_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG
#define MQ4_RL_KOHM 10.0f
#define MQ4_R0_KOHM 4.4f
#define MQ4_PPM_MAX 10000.0f

static unsigned short mq4_ppm_cache;
static const adc_channel_driver_config_t *mq4_config;

static unsigned short mq4_adc_to_ppm(uint16_t adc_raw)
{
    float voltage;
    float rs;
    float ratio;
    float ppm;

    voltage = 3.3f * (float)adc_raw / 4096.0f;
    if (voltage <= 0.0f) {
        return (unsigned short)MQ4_PPM_MAX;
    }

    rs = (3.3f - voltage) / voltage * MQ4_RL_KOHM;
    ratio = rs / MQ4_R0_KOHM;

    /* MQ-4 CH4 拟合曲线: ppm = 1012.7 * (Rs/R0)^-2.786 */
    ppm = 1012.7f * (float)pow(ratio, -2.786f);

    if (ppm < 0.0f) {
        ppm = 0.0f;
    }
    if (ppm > MQ4_PPM_MAX) {
        ppm = MQ4_PPM_MAX;
    }

    return (unsigned short)ppm;
}

static void mq4_init(const void *config)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;

    mq4_config = (const adc_channel_driver_config_t *)config;
    if (mq4_config == 0) {
        return;
    }

    adc_cfg.instance = mq4_config->instance;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = mq4_config->channel;
    ch_cfg.pin = mq4_config->pin;
    ch_cfg.sample_time = MQ4_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    mq4_ppm_cache = 0U;
}

static unsigned short mq4_read_ppm(void)
{
    uint16_t adc_raw;
    unsigned int sum;
    unsigned char i;

    sum = 0U;
    for (i = 0U; i < MQ4_READ_TIMES; i++) {
        if ((mq4_config != 0) && (adc_hal_read(mq4_config->channel, &adc_raw) == HAL_OK)) {
            sum += mq4_adc_to_ppm(adc_raw);
        }
        timer_hal_delay_us(5000U);
    }

    mq4_ppm_cache = (unsigned short)(sum / MQ4_READ_TIMES);
    return mq4_ppm_cache;
}

static const gas_sensor_t mq4_methane_drv = {
    "mq4_methane",
    mq4_init,
    mq4_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq4_methane_drv);

#else
typedef char mq4_adc_disabled[-1];
#endif
