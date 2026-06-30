#include "display_if.h"
#include "oled_font.h"
#include "i2c_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

#include <stdint.h>

#define OLED_WIDTH 128U
#define OLED_PAGES 8U
#define OLED_CTRL_CMD 0x00U
#define OLED_CTRL_DATA 0x40U

static const i2c_device_config_t *oled_config;

static void oled_write_cmd(uint8_t cmd)
{
    if (oled_config == 0) {
        return;
    }
    (void)i2c_hal_write(oled_config->instance, oled_config->address, OLED_CTRL_CMD, &cmd, 1U);
}

static void oled_write_data(const uint8_t *data, uint16_t len)
{
    if (oled_config == 0) {
        return;
    }
    (void)i2c_hal_write(oled_config->instance, oled_config->address, OLED_CTRL_DATA, data, len);
}

static void oled_set_pos(unsigned char col, unsigned char page)
{
    oled_write_cmd((uint8_t)(0xB0U + page));
    oled_write_cmd((uint8_t)(0x00U + (col & 0x0FU)));
    oled_write_cmd((uint8_t)(0x10U + ((col >> 4U) & 0x0FU)));
}

static void oled_hw_init(void)
{
    static const uint8_t init_cmds[] = {
        0xAEU, 0x20U, 0x02U, 0xB0U, 0xC8U, 0x00U, 0x10U, 0x40U,
        0x81U, 0x7FU, 0xA1U, 0xA6U, 0xA8U, 0x3FU, 0xA4U, 0xD3U,
        0x00U, 0xD5U, 0x80U, 0xD9U, 0xF1U, 0xDAU, 0x12U, 0xDBU,
        0x40U, 0x8DU, 0x14U, 0xAFU
    };
    uint8_t i;

    for (i = 0U; i < (uint8_t)(sizeof(init_cmds) / sizeof(init_cmds[0])); ++i) {
        oled_write_cmd(init_cmds[i]);
    }
}

static void oled_init(const void *config)
{
    i2c_hal_config_t cfg;

    oled_config = (const i2c_device_config_t *)config;
    if (oled_config == 0) {
        return;
    }

    cfg.instance = oled_config->instance;
    cfg.speed_hz = oled_config->speed_hz;
    cfg.scl = oled_config->scl;
    cfg.sda = oled_config->sda;
    cfg.remap = oled_config->remap;
    cfg.timeout_us = I2C_HAL_DEFAULT_TIMEOUT_US;
    (void)i2c_hal_init(&cfg);

    oled_hw_init();
}

static void oled_clear(void)
{
    uint8_t zero[16];
    unsigned char page;
    unsigned char block;
    unsigned char i;

    for (i = 0U; i < sizeof(zero); ++i) {
        zero[i] = 0U;
    }

    for (page = 0U; page < OLED_PAGES; ++page) {
        oled_set_pos(0U, page);
        for (block = 0U; block < (OLED_WIDTH / sizeof(zero)); ++block) {
            oled_write_data(zero, sizeof(zero));
        }
    }
}

static void oled_update(void)
{
}

static uint16_t oled_width(void)
{
    return OLED_WIDTH;
}

static uint16_t oled_height(void)
{
    return 64U;
}

static void oled_print_char(unsigned char col, unsigned char page, char ch)
{
    const uint8_t *glyph;
    unsigned char font_width;

    if ((page >= OLED_PAGES) || (col >= (OLED_WIDTH / oled_font_get_width()))) {
        return;
    }

    if ((ch < 0x20) || (ch > 0x7E)) {
        ch = '?';
    }

    font_width = oled_font_get_width();
    glyph = oled_font_get_glyph(ch);
    oled_set_pos((unsigned char)(col * font_width), page);
    oled_write_data(glyph, font_width);
}

static void oled_print(unsigned char x, unsigned char y, display_font_size_t size, const char *text) DISPLAY_IF_REENTRANT
{
    unsigned char col = x;

    (void)size;
    if (text == 0) {
        return;
    }

    while ((*text != '\0') && (col < DISPLAY_SMALL_MAX_CHARS)) {
        oled_print_char(col, y, *text);
        ++text;
        ++col;
    }
}

const display_driver_t oled_drv = {
    "oled",
    oled_init,
    oled_clear,
    oled_update,
    oled_print,
    oled_width,
    oled_height,
    0,
    0,
    0,
    0
};

REGISTER_DRIVER(DISPLAY, oled_drv);
