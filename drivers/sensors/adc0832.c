#include "adc0832.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "board_config.h"

static void adc0832_clock_pulse(void)
{
    gpio_hal_write(board_adc0832_clk_pin.port, board_adc0832_clk_pin.pin, 1U);
    hal_delay_us(2U);
    gpio_hal_write(board_adc0832_clk_pin.port, board_adc0832_clk_pin.pin, 0U);
    hal_delay_us(2U);
}

static void adc0832_write_bit(uint8_t bit)
{
    gpio_hal_set_mode(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin, GPIO_HAL_MODE_OUT_PP);
    gpio_hal_write(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin, bit != 0U ? 1U : 0U);
    adc0832_clock_pulse();
}

static uint8_t adc0832_read_bit(void)
{
    uint8_t bit;

    gpio_hal_set_mode(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin, GPIO_HAL_MODE_IN_FLOATING);
    gpio_hal_write(board_adc0832_clk_pin.port, board_adc0832_clk_pin.pin, 1U);
    hal_delay_us(2U);
    bit = gpio_hal_read(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin);
    gpio_hal_write(board_adc0832_clk_pin.port, board_adc0832_clk_pin.pin, 0U);
    hal_delay_us(2U);
    return bit;
}

void adc0832_init(void)
{
    gpio_hal_config_pin(&board_adc0832_cs_pin);
    gpio_hal_config_pin(&board_adc0832_clk_pin);
    gpio_hal_config_pin(&board_adc0832_dio_pin);
    gpio_hal_write(board_adc0832_cs_pin.port, board_adc0832_cs_pin.pin, 1U);
    gpio_hal_write(board_adc0832_clk_pin.port, board_adc0832_clk_pin.pin, 0U);
    gpio_hal_write(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin, 1U);
}

uint8_t adc0832_read(uint8_t channel)
{
    uint8_t i;
    uint8_t value = 0U;

    channel = (channel == 0U) ? 0U : 1U;
    gpio_hal_write(board_adc0832_cs_pin.port, board_adc0832_cs_pin.pin, 0U);
    hal_delay_us(2U);

    adc0832_write_bit(1U);
    adc0832_write_bit(1U);
    adc0832_write_bit(channel);

    for (i = 0U; i < 8U; ++i) {
        value = (uint8_t)((value << 1U) | adc0832_read_bit());
    }

    gpio_hal_write(board_adc0832_cs_pin.port, board_adc0832_cs_pin.pin, 1U);
    gpio_hal_set_mode(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin, GPIO_HAL_MODE_OUT_PP);
    gpio_hal_write(board_adc0832_dio_pin.port, board_adc0832_dio_pin.pin, 1U);
    return value;
}
