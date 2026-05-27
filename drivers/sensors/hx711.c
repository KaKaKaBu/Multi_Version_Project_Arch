#include "weight_if.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "board_config.h"
#include "driver_core.h"

static float hx711_weight_cache;

static void hx711_sck_write(uint8_t level)
{
    gpio_hal_write(board_hx711_sck_pin.port, board_hx711_sck_pin.pin, level);
}

static uint8_t hx711_dt_read(void)
{
    return gpio_hal_read(board_hx711_dt_pin.port, board_hx711_dt_pin.pin);
}

static int32_t hx711_read_raw(void)
{
    uint8_t i;
    int32_t count = 0;
    uint32_t timeout = 200000U;

    hx711_sck_write(0U);
    hal_delay_us(1U);

    while ((hx711_dt_read() != 0U) && (timeout > 0U)) {
        --timeout;
        hal_delay_us(1U);
    }
    if (timeout == 0U) {
        return 0;
    }

    for (i = 0U; i < 24U; ++i) {
        hx711_sck_write(1U);
        hal_delay_us(1U);
        count = (int32_t)(count << 1);
        hx711_sck_write(0U);
        hal_delay_us(1U);
        if (hx711_dt_read() != 0U) {
            ++count;
        }
    }

    hx711_sck_write(1U);
    hal_delay_us(1U);
    count ^= 0x800000;
    hx711_sck_write(0U);
    hal_delay_us(1U);

    return count;
}

static float hx711_sample_grams(void)
{
    int32_t raw = hx711_read_raw();

    return ((float)(raw - (int32_t)BOARD_HX711_OFFSET)) / BOARD_HX711_SCALE;
}

static void hx711_init(void)
{
    gpio_hal_config_pin(&board_hx711_sck_pin);
    gpio_hal_config_pin(&board_hx711_dt_pin);
    hx711_sck_write(0U);
    hal_delay_us(1000000U);
    hx711_weight_cache = hx711_sample_grams();
}

static float hx711_read_grams(void)
{
    hx711_weight_cache = hx711_sample_grams();
    return hx711_weight_cache;
}

static const weight_sensor_t hx711_drv = {
    "hx711",
    hx711_init,
    hx711_read_grams
};

REGISTER_DRIVER(WEIGHT_SENSOR, hx711_drv);
