#include "gas_if.h"
#include "adc_hal.h"
#include "timer_hal.h"
#include "driver_configs.h"
#include "driver_core.h"
#include <math.h>

#if HAL_ADC_ENABLE

#define MQ7_READ_TIMES 10U
#define MQ7_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG

static unsigned short mq7_ppm_cache;
static const adc_channel_driver_config_t *mq7_config;

static unsigned short mq7_adc_to_ppm(uint16_t adc_raw)
{
    float voltage;
    float rs;
    float ratio;
    float ppm;

    voltage = 3.3f * (float)adc_raw / 4096.0f;
    rs = (3.3f - voltage) / voltage * 10.0f;
    ratio = rs / 4.4f;

    ppm = 98.322f * (float)pow(ratio, -1.458f);

    if (ppm < 0) {
        ppm = 0;
    }
    if (ppm > 1000) {
        ppm = 1000;
    }

    return (unsigned short)ppm;
}

static void mq7_init(const void *config)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;

    mq7_config = (const adc_channel_driver_config_t *)config;
    if (mq7_config == 0) {
        return;
    }

    adc_cfg.instance = mq7_config->instance;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = mq7_config->channel;
    ch_cfg.pin = mq7_config->pin;
    ch_cfg.sample_time = MQ7_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    mq7_ppm_cache = 0U;
}

static unsigned short mq7_read_ppm(void)
{
    uint16_t adc_raw;
    unsigned int sum;
    unsigned char i;

    sum = 0U;
    for (i = 0U; i < MQ7_READ_TIMES; i++) {
        if ((mq7_config != 0) && (adc_hal_read(mq7_config->channel, &adc_raw) == HAL_OK)) {
            sum += mq7_adc_to_ppm(adc_raw);
        }
        timer_hal_delay_us(5000U);
    }

    mq7_ppm_cache = (unsigned short)(sum / MQ7_READ_TIMES);
    return mq7_ppm_cache;
}

static const gas_sensor_t mq7_co_drv = {
    "mq7_co",
    mq7_init,
    mq7_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq7_co_drv);

#else
typedef char mq7_adc_disabled[-1];
#endif
