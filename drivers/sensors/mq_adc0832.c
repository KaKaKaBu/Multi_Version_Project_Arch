#include "gas_if.h"
#include "adc0832.h"
#include "driver_configs.h"
#include "driver_core.h"

#ifndef MQ_ADC0832_READ_TIMES
#define MQ_ADC0832_READ_TIMES 8U
#endif

static const mq_adc0832_driver_config_t *mq135_config;
static const mq_adc0832_driver_config_t *mq7_config;

static unsigned short mq_adc0832_read_scaled(uint8_t channel, unsigned short max_ppm)
{
    unsigned int sum = 0U;
    uint8_t i;

    for (i = 0U; i < MQ_ADC0832_READ_TIMES; ++i) {
        sum += adc0832_read(channel);
    }

    return (unsigned short)(((sum / MQ_ADC0832_READ_TIMES) * (unsigned int)max_ppm) / 255U);
}

static void mq135_adc0832_init(const void *config)
{
    mq135_config = (const mq_adc0832_driver_config_t *)config;
    if (mq135_config == 0) {
        return;
    }
    adc0832_set_config(&mq135_config->adc);
    adc0832_init();
}

static unsigned short mq135_read_ppm(void)
{
    if (mq135_config == 0) {
        return 0U;
    }
    adc0832_set_config(&mq135_config->adc);
    return mq_adc0832_read_scaled(mq135_config->channel, mq135_config->max_ppm);
}

static void mq7_adc0832_init(const void *config)
{
    mq7_config = (const mq_adc0832_driver_config_t *)config;
    if (mq7_config == 0) {
        return;
    }
    adc0832_set_config(&mq7_config->adc);
    adc0832_init();
}

static unsigned short mq7_read_ppm(void)
{
    if (mq7_config == 0) {
        return 0U;
    }
    adc0832_set_config(&mq7_config->adc);
    return mq_adc0832_read_scaled(mq7_config->channel, mq7_config->max_ppm);
}

const gas_sensor_t mq135_drv = {
    "mq135",
    mq135_adc0832_init,
    mq135_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq135_drv);

const gas_sensor_t mq7_co_drv = {
    "mq7_co",
    mq7_adc0832_init,
    mq7_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq7_co_drv);
