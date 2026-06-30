#include "weight_if.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

static float hx711_weight_cache;
static const hx711_driver_config_t *hx711_config;

static void hx711_sck_write(uint8_t level)
{
    gpio_hal_write(hx711_config->sck.port, hx711_config->sck.pin, level);
}

static uint8_t hx711_dt_read(void)
{
    return gpio_hal_read(hx711_config->data.port, hx711_config->data.pin);
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

    return ((float)(raw - (int32_t)hx711_config->offset)) / hx711_config->scale;
}

static void hx711_init(const void *config)
{
    hx711_config = (const hx711_driver_config_t *)config;
    if (hx711_config == 0) {
        return;
    }
    gpio_hal_config_pin(&hx711_config->sck);
    gpio_hal_config_pin(&hx711_config->data);
    hx711_sck_write(0U);
    hal_delay_us(1000000U);
    hx711_weight_cache = hx711_sample_grams();
}

static float hx711_read_grams(void)
{
    if (hx711_config == 0) {
        return hx711_weight_cache;
    }
    hx711_weight_cache = hx711_sample_grams();
    return hx711_weight_cache;
}

static const weight_sensor_t hx711_drv = {
    "hx711",
    hx711_init,
    hx711_read_grams
};

REGISTER_DRIVER(WEIGHT_SENSOR, hx711_drv);
