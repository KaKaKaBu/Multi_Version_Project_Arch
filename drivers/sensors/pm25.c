#include "gas_if.h"
#include "adc_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

#if HAL_ADC_ENABLE

#define PM25_READ_TIMES 10U
#define PM25_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG
#define PM25_LED_TO_SAMPLE_US 280U
#define PM25_SAMPLE_TO_LED_OFF_US 40U
#define PM25_SAMPLE_PERIOD_US 10000U

static unsigned short pm25_density_cache;
static const pm25_driver_config_t *pm25_config;

static void pm25_led_set(unsigned char on)
{
    unsigned char active_level;
    unsigned char level;

    if (pm25_config == 0) {
        return;
    }
    active_level = (pm25_config->led_active_level != 0U) ? 1U : 0U;
    level = (on != 0U) ? active_level : (unsigned char)(active_level ^ 1U);
    gpio_hal_write(pm25_config->led_pin.port, pm25_config->led_pin.pin, level);
}

static unsigned short pm25_adc_to_density(uint16_t adc_raw)
{
    float voltage;
    float density;

    voltage = ((float)adc_raw / 4095.0f) * pm25_config->vref_mv / 1000.0f;
    voltage *= pm25_config->vout_scale;
    density = (pm25_config->density_slope * voltage) + pm25_config->density_offset;

    if (density < 0.0f) {
        density = 0.0f;
    }
    if (density > pm25_config->density_max) {
        density = pm25_config->density_max;
    }

    return (unsigned short)(density + 0.5f);
}

static void pm25_init(const void *config)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;

    pm25_config = (const pm25_driver_config_t *)config;
    if (pm25_config == 0) {
        return;
    }

    gpio_hal_config_pin(&pm25_config->led_pin);
    pm25_led_set(0U);

    adc_cfg.instance = pm25_config->adc.instance;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = pm25_config->adc.channel;
    ch_cfg.pin = pm25_config->adc.pin;
    ch_cfg.sample_time = PM25_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    pm25_density_cache = 0U;
}

static hal_status_t pm25_sample_adc(uint16_t *adc_raw)
{
    hal_status_t status;

    pm25_led_set(1U);
    timer_hal_delay_us(PM25_LED_TO_SAMPLE_US);
    if (pm25_config == 0) {
        return HAL_ERR_PARAM;
    }
    status = adc_hal_read(pm25_config->adc.channel, adc_raw);
    timer_hal_delay_us(PM25_SAMPLE_TO_LED_OFF_US);
    pm25_led_set(0U);

    return status;
}

static unsigned short pm25_read_ppm(void)
{
    uint16_t adc_raw;
    unsigned int sum;
    unsigned char samples;
    unsigned char i;

    sum = 0U;
    samples = 0U;
    for (i = 0U; i < PM25_READ_TIMES; i++) {
        if (pm25_sample_adc(&adc_raw) == HAL_OK) {
            sum += pm25_adc_to_density(adc_raw);
            samples++;
        }
        timer_hal_delay_us(PM25_SAMPLE_PERIOD_US - PM25_LED_TO_SAMPLE_US - PM25_SAMPLE_TO_LED_OFF_US);
    }

    if (samples == 0U) {
        return pm25_density_cache;
    }

    pm25_density_cache = (unsigned short)(sum / samples);
    return pm25_density_cache;
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
