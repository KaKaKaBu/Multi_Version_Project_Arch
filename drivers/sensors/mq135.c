#include "gas_if.h"
#include "adc_hal.h"
#include "timer_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include <math.h>
#include "stm32f10x.h"

#if HAL_ADC_ENABLE

#define MQ135_READ_TIMES 10U
#define MQ135_ADC_SAMPLE_TIME ADC_SampleTime_239Cycles5

static unsigned short mq135_ppm_cache;

static unsigned short mq135_adc_to_ppm(uint16_t adc_raw)
{
    float voltage;
    float rs;
    float ratio;
    float ppm;

    voltage = 3.3f * (float)adc_raw / 4096.0f;
    rs = (3.3f - voltage) / voltage * 10.0f;
    ratio = rs / 10.0f;

    ppm = 116.6020682f * (float)pow(ratio, -2.769034857f);

    if (ppm < 0) {
        ppm = 0;
    }
    if (ppm > 10000) {
        ppm = 10000;
    }

    return (unsigned short)ppm;
}

static void mq135_init(void)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;

    adc_cfg.instance = BOARD_ADC_INSTANCE;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = BOARD_MQ135_ADC_CHANNEL;
    ch_cfg.pin = board_mq135_adc_pin;
    ch_cfg.sample_time = MQ135_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    mq135_ppm_cache = 0U;
}

static unsigned short mq135_read_ppm(void)
{
    uint16_t adc_raw;
    unsigned int sum;
    unsigned char i;

    sum = 0U;
    for (i = 0U; i < MQ135_READ_TIMES; i++) {
        if (adc_hal_read(BOARD_MQ135_ADC_CHANNEL, &adc_raw) == HAL_OK) {
            sum += mq135_adc_to_ppm(adc_raw);
        }
        timer_hal_delay_us(5000U);
    }

    mq135_ppm_cache = (unsigned short)(sum / MQ135_READ_TIMES);
    return mq135_ppm_cache;
}

static const gas_sensor_t mq135_drv = {
    "mq135",
    mq135_init,
    mq135_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq135_drv);

#else
typedef char mq135_adc_disabled[-1];
#endif
