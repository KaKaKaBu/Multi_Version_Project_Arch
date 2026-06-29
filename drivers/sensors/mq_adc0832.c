#include "gas_if.h"
#include "adc0832.h"
#include "board_config.h"
#include "driver_core.h"

#ifndef MQ_ADC0832_READ_TIMES
#define MQ_ADC0832_READ_TIMES 8U
#endif

static unsigned short mq_adc0832_read_scaled(uint8_t channel, unsigned short max_ppm)
{
    unsigned int sum = 0U;
    uint8_t i;

    for (i = 0U; i < MQ_ADC0832_READ_TIMES; ++i) {
        sum += adc0832_read(channel);
    }

    return (unsigned short)(((sum / MQ_ADC0832_READ_TIMES) * (unsigned int)max_ppm) / 255U);
}

static void mq_adc0832_init(void)
{
    adc0832_init();
}

static unsigned short mq135_read_ppm(void)
{
    return mq_adc0832_read_scaled(BOARD_MQ135_ADC0832_CHANNEL, BOARD_MQ135_ADC0832_MAX_PPM);
}

static unsigned short mq7_read_ppm(void)
{
    return mq_adc0832_read_scaled(BOARD_MQ7_ADC0832_CHANNEL, BOARD_MQ7_ADC0832_MAX_PPM);
}

const gas_sensor_t mq135_drv = {
    "mq135",
    mq_adc0832_init,
    mq135_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq135_drv);

const gas_sensor_t mq7_co_drv = {
    "mq7_co",
    mq_adc0832_init,
    mq7_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, mq7_co_drv);
