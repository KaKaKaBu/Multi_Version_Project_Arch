#include "segment_if.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "board_config.h"
#include "driver_core.h"

static const unsigned char hc595_seg_tab[] = {
    0xC0U, 0xCFU, 0xA4U, 0xB0U, 0x99U, 0x92U, 0x82U, 0xF8U, 0x80U, 0x90U,
    0x88U, 0x83U, 0xC6U, 0xA1U, 0x86U, 0x8EU, 0xFFU, 0x7FU
};

static void hc595_1_shift_byte(unsigned char value)
{
    unsigned char i;

    for (i = 0U; i < 8U; i++) {
        gpio_hal_write(board_hc595_1_sclk_pin.port, board_hc595_1_sclk_pin.pin, 0U);
        gpio_hal_write(board_hc595_1_sdi_pin.port, board_hc595_1_sdi_pin.pin,
                       (value & 0x80U) ? 1U : 0U);
        value = (unsigned char)(value << 1);
        gpio_hal_write(board_hc595_1_sclk_pin.port, board_hc595_1_sclk_pin.pin, 1U);
    }

    gpio_hal_write(board_hc595_1_sclk_pin.port, board_hc595_1_sclk_pin.pin, 0U);
    gpio_hal_write(board_hc595_1_sdi_pin.port, board_hc595_1_sdi_pin.pin, 0U);
}

static void hc595_1_load(void)
{
    gpio_hal_write(board_hc595_1_load_pin.port, board_hc595_1_load_pin.pin, 1U);
    timer_hal_delay_us(1U);
    gpio_hal_write(board_hc595_1_load_pin.port, board_hc595_1_load_pin.pin, 0U);
}

static void hc595_1_init(void)
{
    gpio_hal_config_pin(&board_hc595_1_load_pin);
    gpio_hal_config_pin(&board_hc595_1_sclk_pin);
    gpio_hal_config_pin(&board_hc595_1_sdi_pin);

    gpio_hal_write(board_hc595_1_load_pin.port, board_hc595_1_load_pin.pin, 0U);
    gpio_hal_write(board_hc595_1_sclk_pin.port, board_hc595_1_sclk_pin.pin, 0U);
    gpio_hal_write(board_hc595_1_sdi_pin.port, board_hc595_1_sdi_pin.pin, 0U);
}

static void hc595_1_show_digits(unsigned char ten, unsigned char one)
{
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
