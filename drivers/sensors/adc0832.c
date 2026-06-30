#include "adc0832.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"

static const adc0832_driver_config_t *adc0832_config;

void adc0832_set_config(const adc0832_driver_config_t *config)
{
    adc0832_config = config;
}

static void adc0832_clock_pulse(void)
{
    gpio_hal_write(adc0832_config->clk.port, adc0832_config->clk.pin, 1U);
    hal_delay_us(2U);
    gpio_hal_write(adc0832_config->clk.port, adc0832_config->clk.pin, 0U);
    hal_delay_us(2U);
}

static void adc0832_write_bit(uint8_t bit)
{
    gpio_hal_set_mode(adc0832_config->dio.port, adc0832_config->dio.pin, GPIO_HAL_MODE_OUT_PP);
    gpio_hal_write(adc0832_config->dio.port, adc0832_config->dio.pin, bit != 0U ? 1U : 0U);
    adc0832_clock_pulse();
}

static uint8_t adc0832_read_bit(void)
{
    uint8_t bit;

    gpio_hal_set_mode(adc0832_config->dio.port, adc0832_config->dio.pin, GPIO_HAL_MODE_IN_FLOATING);
    gpio_hal_write(adc0832_config->clk.port, adc0832_config->clk.pin, 1U);
    hal_delay_us(2U);
    bit = gpio_hal_read(adc0832_config->dio.port, adc0832_config->dio.pin);
    gpio_hal_write(adc0832_config->clk.port, adc0832_config->clk.pin, 0U);
    hal_delay_us(2U);
    return bit;
}

void adc0832_init(void)
{
    if (adc0832_config == 0) {
        return;
    }
    gpio_hal_config_pin(&adc0832_config->cs);
    gpio_hal_config_pin(&adc0832_config->clk);
    gpio_hal_config_pin(&adc0832_config->dio);
    gpio_hal_write(adc0832_config->cs.port, adc0832_config->cs.pin, 1U);
    gpio_hal_write(adc0832_config->clk.port, adc0832_config->clk.pin, 0U);
    gpio_hal_write(adc0832_config->dio.port, adc0832_config->dio.pin, 1U);
}

uint8_t adc0832_read(uint8_t channel)
{
    uint8_t i;
    uint8_t value = 0U;

    if (adc0832_config == 0) {
        return 0U;
    }

    channel = (channel == 0U) ? 0U : 1U;
    gpio_hal_write(adc0832_config->cs.port, adc0832_config->cs.pin, 0U);
    hal_delay_us(2U);

    adc0832_write_bit(1U);
    adc0832_write_bit(1U);
    adc0832_write_bit(channel);

    for (i = 0U; i < 8U; ++i) {
        value = (uint8_t)((value << 1U) | adc0832_read_bit());
    }

    gpio_hal_write(adc0832_config->cs.port, adc0832_config->cs.pin, 1U);
    gpio_hal_set_mode(adc0832_config->dio.port, adc0832_config->dio.pin, GPIO_HAL_MODE_OUT_PP);
    gpio_hal_write(adc0832_config->dio.port, adc0832_config->dio.pin, 1U);
    return value;
}
