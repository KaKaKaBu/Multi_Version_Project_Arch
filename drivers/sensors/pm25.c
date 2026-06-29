#include "gas_if.h"
#include "adc_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "board_config.h"
#include "driver_core.h"

#if HAL_ADC_ENABLE

#define PM25_READ_TIMES 10U
#define PM25_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG
#define PM25_LED_TO_SAMPLE_US 280U
#define PM25_SAMPLE_TO_LED_OFF_US 40U
#define PM25_SAMPLE_PERIOD_US 10000U

#ifndef BOARD_PM25_LED_ACTIVE_LEVEL
#define BOARD_PM25_LED_ACTIVE_LEVEL 0U
#endif

#ifndef BOARD_PM25_ADC_VREF_MV
#define BOARD_PM25_ADC_VREF_MV 3300.0f
#endif

#ifndef BOARD_PM25_VOUT_SCALE
#define BOARD_PM25_VOUT_SCALE 1.0f
#endif

#ifndef BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V
#define BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V 170.0f
#endif

#ifndef BOARD_PM25_DENSITY_OFFSET_UG_M3
#define BOARD_PM25_DENSITY_OFFSET_UG_M3 (-100.0f)
#endif

#ifndef BOARD_PM25_DENSITY_MAX_UG_M3
#define BOARD_PM25_DENSITY_MAX_UG_M3 500.0f
#endif

static unsigned short pm25_density_cache;

static void pm25_led_set(unsigned char on)
{
    unsigned char active_level;
    unsigned char level;

    active_level = (BOARD_PM25_LED_ACTIVE_LEVEL != 0U) ? 1U : 0U;
    level = (on != 0U) ? active_level : (unsigned char)(active_level ^ 1U);
    gpio_hal_write(board_pm25_led_pin.port, board_pm25_led_pin.pin, level);
}

static unsigned short pm25_adc_to_density(uint16_t adc_raw)
{
    float voltage;
    float density;

    voltage = ((float)adc_raw / 4095.0f) * BOARD_PM25_ADC_VREF_MV / 1000.0f;
    voltage *= BOARD_PM25_VOUT_SCALE;
    density = (BOARD_PM25_DENSITY_SLOPE_UG_M3_PER_V * voltage) + BOARD_PM25_DENSITY_OFFSET_UG_M3;

    if (density < 0.0f) {
        density = 0.0f;
    }
    if (density > BOARD_PM25_DENSITY_MAX_UG_M3) {
        density = BOARD_PM25_DENSITY_MAX_UG_M3;
    }

    return (unsigned short)(density + 0.5f);
}

static void pm25_init(void)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;

    gpio_hal_config_pin(&board_pm25_led_pin);
    pm25_led_set(0U);

    adc_cfg.instance = BOARD_ADC_INSTANCE;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = BOARD_PM25_ADC_CHANNEL;
    ch_cfg.pin = board_pm25_adc_pin;
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
    status = adc_hal_read(BOARD_PM25_ADC_CHANNEL, adc_raw);
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
