#include "analog_probe_if.h"
#include "adc_hal.h"
#include "board_config.h"
#include "driver_core.h"

#if HAL_ADC_ENABLE

#define PH_SENSOR_FILTER_SIZE 8U
#define PH_SENSOR_ADC_SAMPLE_TIME ADC_HAL_SAMPLE_TIME_LONG
#define PH_SENSOR_DEFAULT_VALUE 7.0f
#define PH_SENSOR_CALIBRATION_FACTOR 1.0f
#define PH_SENSOR_TEMPERATURE 25.0f

static float ph_filter_buffer[PH_SENSOR_FILTER_SIZE];
static unsigned char ph_filter_index;
static unsigned char ph_filter_count;
static float ph_value_cache;

static float ph_adc_to_value(uint16_t adc_raw)
{
    float voltage;
    float ph_raw;

    voltage = (float)adc_raw / 4096.0f * 3.3f;
    ph_raw = (BOARD_PH_VOLTAGE_COEFF * voltage + BOARD_PH_VOLTAGE_OFFSET) * PH_SENSOR_CALIBRATION_FACTOR;
    ph_raw += (PH_SENSOR_TEMPERATURE - 25.0f) * 0.015f;

    if (ph_raw < 0.0f) {
        ph_raw = 0.0f;
    }
    if (ph_raw > 14.0f) {
        ph_raw = 14.0f;
    }

    return ph_raw;
}

static void ph_sensor_init(void)
{
    adc_hal_config_t adc_cfg;
    adc_hal_channel_cfg_t ch_cfg;
    unsigned char i;

    adc_cfg.instance = BOARD_ADC_INSTANCE;
    adc_cfg.timeout_us = ADC_HAL_DEFAULT_TIMEOUT_US;
    (void)adc_hal_init(&adc_cfg);

    ch_cfg.channel = BOARD_PH_ADC_CHANNEL;
    ch_cfg.pin = board_ph_adc_pin;
    ch_cfg.sample_time = PH_SENSOR_ADC_SAMPLE_TIME;
    ch_cfg.rank = 1U;
    (void)adc_hal_config_channel(&ch_cfg);

    for (i = 0U; i < PH_SENSOR_FILTER_SIZE; i++) {
        ph_filter_buffer[i] = PH_SENSOR_DEFAULT_VALUE;
    }
    ph_filter_index = 0U;
    ph_filter_count = 0U;
    ph_value_cache = PH_SENSOR_DEFAULT_VALUE;
}

static float ph_sensor_read_value(void)
{
    uint16_t adc_raw;
    float sum;
    unsigned char i;

    if (adc_hal_read(BOARD_PH_ADC_CHANNEL, &adc_raw) != HAL_OK) {
        return ph_value_cache;
    }

    ph_filter_buffer[ph_filter_index] = ph_adc_to_value(adc_raw);
    ph_filter_index = (unsigned char)((ph_filter_index + 1U) % PH_SENSOR_FILTER_SIZE);
    if (ph_filter_count < PH_SENSOR_FILTER_SIZE) {
        ph_filter_count++;
    }

    sum = 0.0f;
    for (i = 0U; i < ph_filter_count; i++) {
        sum += ph_filter_buffer[i];
    }

    ph_value_cache = sum / (float)ph_filter_count;
    return ph_value_cache;
}

static const analog_probe_t ph_sensor_drv = {
    "ph_sensor",
    ph_sensor_init,
    ph_sensor_read_value
};

REGISTER_DRIVER(ANALOG_PROBE, ph_sensor_drv);

#else
typedef char ph_sensor_adc_disabled[-1];
#endif
