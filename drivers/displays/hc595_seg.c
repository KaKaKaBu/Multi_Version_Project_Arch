#include "segment_if.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static const unsigned char hc595_seg_tab[] = {
    0xC0U, 0xCFU, 0xA4U, 0xB0U, 0x99U, 0x92U, 0x82U, 0xF8U, 0x80U, 0x90U,
    0x88U, 0x83U, 0xC6U, 0xA1U, 0x86U, 0x8EU, 0xFFU, 0x7FU
};

static const hc595_driver_config_t *hc595_1_config;

static void hc595_1_shift_byte(unsigned char value)
{
    unsigned char i;

    for (i = 0U; i < 8U; i++) {
        gpio_hal_write(hc595_1_config->sclk.port, hc595_1_config->sclk.pin, 0U);
        gpio_hal_write(hc595_1_config->sdi.port, hc595_1_config->sdi.pin,
                       (value & 0x80U) ? 1U : 0U);
        value = (unsigned char)(value << 1);
        gpio_hal_write(hc595_1_config->sclk.port, hc595_1_config->sclk.pin, 1U);
    }

    gpio_hal_write(hc595_1_config->sclk.port, hc595_1_config->sclk.pin, 0U);
    gpio_hal_write(hc595_1_config->sdi.port, hc595_1_config->sdi.pin, 0U);
}

static void hc595_1_load(void)
{
    gpio_hal_write(hc595_1_config->load.port, hc595_1_config->load.pin, 1U);
    timer_hal_delay_us(1U);
    gpio_hal_write(hc595_1_config->load.port, hc595_1_config->load.pin, 0U);
}

static void hc595_1_init(const void *config)
{
    hc595_1_config = (const hc595_driver_config_t *)config;
    if (hc595_1_config == 0) {
        return;
    }

    gpio_hal_config_pin(&hc595_1_config->load);
    gpio_hal_config_pin(&hc595_1_config->sclk);
    gpio_hal_config_pin(&hc595_1_config->sdi);

    gpio_hal_write(hc595_1_config->load.port, hc595_1_config->load.pin, 0U);
    gpio_hal_write(hc595_1_config->sclk.port, hc595_1_config->sclk.pin, 0U);
    gpio_hal_write(hc595_1_config->sdi.port, hc595_1_config->sdi.pin, 0U);
}

static void hc595_1_show_digits(unsigned char ten, unsigned char one)
{
    if (hc595_1_config == 0) {
        return;
    }
    if (ten >= (sizeof(hc595_seg_tab) / sizeof(hc595_seg_tab[0]))) {
        ten = 0U;
    }
    if (one >= (sizeof(hc595_seg_tab) / sizeof(hc595_seg_tab[0]))) {
        one = 0U;
    }

    hc595_1_shift_byte(hc595_seg_tab[one]);
    hc595_1_shift_byte(hc595_seg_tab[ten]);
    hc595_1_load();
}

static const segment_display_t hc595_1_drv = {
    "hc595_1",
    hc595_1_init,
    hc595_1_show_digits
};

REGISTER_DRIVER(SEGMENT_DISPLAY, hc595_1_drv);
