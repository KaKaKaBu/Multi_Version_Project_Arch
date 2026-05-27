#include "gas_if.h"
#include "adc_hal.h"
#include "timer_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

#if HAL_ADC_ENABLE

#define PM25_READ_TIMES 10U
#define PM25_ADC_SAMPLE_TIME ADC_SampleTime_239Cycles5

static unsigned short pm25_ppm_cache;

static unsigned short pm25_adc_to_ppm(uint16_t adc_raw)
{
    float voltage;
    int dust_val;

    voltage = 3.3f * (float)adc_raw / 4096.0f * 2.0f;
    dust_val = (int)((0.17f * voltage - 0.1f) * 1000.0f);

    if (dust_val < 0) {
        dust_val = 0;
    }
    if (dust_val > 500) {
        dust_val = 500;
    }

    return (unsigned short)dust_val;
}

static void pm25_init(void)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;

    adc_cfg.instance = BOARD_ADC_INSTANCE;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = BOARD_PM25_ADC_CHANNEL;
    ch_cfg.pin = board_pm25_adc_pin;
    ch_cfg.sample_time = PM25_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    pm25_ppm_cache = 0U;
}

static unsigned short pm25_read_ppm(void)
{
    uint16_t adc_raw;
    unsigned int sum;
    unsigned char i;

    sum = 0U;
    for (i = 0U; i < PM25_READ_TIMES; i++) {
        if (adc_hal_read(BOARD_PM25_ADC_CHANNEL, &adc_raw) == HAL_OK) {
            sum += pm25_adc_to_ppm(adc_raw);
        }
        timer_hal_delay_us(5000U);
    }

    pm25_ppm_cache = (unsigned short)(sum / PM25_READ_TIMES);
    return pm25_ppm_cache;
}

static const gas_sensor_t pm25_drv = {
    "pm25",
    pm25_init,
    pm25_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, pm25_drv);

#else
typedef char pm25_adc_disabled[-1];
#endif
