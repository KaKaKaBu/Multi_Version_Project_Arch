#include "display_if.h"
#include "oled_font.h"
#include "tiny_printf.h"
#include "i2c_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"
#include <stdarg.h>

#define OLED_WIDTH 128U
#define OLED_HEIGHT 64U
#define OLED_PAGES 8U
#define OLED_CTRL_CMD 0x00U
#define OLED_CTRL_DATA 0x40U
#define OLED_PRINT_BUF_SIZE 64U

static uint8_t oled_framebuffer[OLED_WIDTH * OLED_PAGES];

static void oled_clear(void);
static void oled_update(void);

static unsigned char oled_font_scale(display_font_size_t size)
{
    return (size == DISPLAY_FONT_LARGE) ? 2U : 1U;
}

static void oled_set_pixel(unsigned char x, unsigned char y, unsigned char on)
{
    uint16_t index;
    unsigned char page;
    unsigned char bit;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;
    }

    page = (unsigned char)(y / 8U);
    bit = (unsigned char)(y % 8U);
    index = (uint16_t)((page * OLED_WIDTH) + x);

    if (on != 0U) {
        oled_framebuffer[index] |= (uint8_t)(1U << bit);
    } else {
        oled_framebuffer[index] &= (uint8_t)~(1U << bit);
    }
}

static void oled_write_cmd(uint8_t cmd)
{
    (void)i2c_hal_write(BOARD_OLED_I2C, BOARD_OLED_I2C_ADDR, OLED_CTRL_CMD, &cmd, 1U);
}

static void oled_write_data(const uint8_t *data, uint16_t len)
{
    (void)i2c_hal_write(BOARD_OLED_I2C, BOARD_OLED_I2C_ADDR, OLED_CTRL_DATA, data, len);
}

static void oled_hw_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    uint8_t i;

    for (i = 0U; i < (uint8_t)(sizeof(init_seq) / sizeof(init_seq[0])); ++i) {
        oled_write_cmd(init_seq[i]);
    }
}

static void oled_init(void)
{
    i2c_hal_config_t cfg;

    cfg.instance = BOARD_OLED_I2C;
    cfg.speed_hz = BOARD_OLED_I2C_SPEED;
    cfg.scl = board_oled_i2c_scl;
    cfg.sda = board_oled_i2c_sda;
    cfg.remap = BOARD_OLED_I2C_REMAP;
    cfg.timeout_us = I2C_HAL_DEFAULT_TIMEOUT_US;
    (void)i2c_hal_init(&cfg);

    oled_hw_init();
    oled_clear();
    oled_update();
}

static void oled_clear(void)
{
    uint16_t i;

    for (i = 0U; i < (uint16_t)(sizeof(oled_framebuffer)); ++i) {
        oled_framebuffer[i] = 0U;
    }
}

static void oled_draw_char(unsigned char col, unsigned char page, char ch, unsigned char scale)
{
    const uint8_t *glyph;
    unsigned char font_width;
    unsigned char font_height;
    unsigned char col_bytes;
    unsigned char font_pages;
    unsigned char glyph_width;
    unsigned char glyph_pages;
    unsigned char max_cols;
    unsigned char base_x;
    unsigned char base_y;
    unsigned char x;
    unsigned char page_offset;
    unsigned char bit;
    unsigned char dx;
    unsigned char dy;

    font_width = oled_font_get_width();
    font_height = oled_font_get_height();
    col_bytes = oled_font_get_col_bytes();
    font_pages = (unsigned char)((font_height + 7U) / 8U);
    glyph_width = (unsigned char)(font_width * scale);
    glyph_pages = (unsigned char)(font_pages * scale);
    max_cols = (unsigned char)(OLED_WIDTH / glyph_width);

    if ((page + glyph_pages) > OLED_PAGES) {
        return;
    }

    if (col >= max_cols) {
        return;
    }

    if ((ch < 0x20) || (ch > 0x7E)) {
        ch = '?';
    }

    glyph = oled_font_get_glyph(ch);
    base_x = (unsigned char)(col * glyph_width);
    base_y = (unsigned char)(page * 8U);

    for (page_offset = 0U; page_offset < font_pages; ++page_offset) {
        for (x = 0U; x < font_width; ++x) {
            uint8_t column_data = glyph[(x * col_bytes) + page_offset];

            for (bit = 0U; bit < 8U; ++bit) {
                if ((column_data & (uint8_t)(1U << bit)) == 0U) {
                    continue;
                }

                if ((page_offset * 8U) + bit >= font_height) {
                    continue;
                }

                for (dy = 0U; dy < scale; ++dy) {
                    for (dx = 0U; dx < scale; ++dx) {
                        oled_set_pixel(
                            (unsigned char)(base_x + (x * scale) + dx),
                            (unsigned char)(base_y + (page_offset * 8U * scale) + (bit * scale) + dy),
                            1U);
                    }
                }
            }
        }
    }
}

static void oled_print_text(unsigned char x, unsigned char y, display_font_size_t size, const char *text)
{
    unsigned char col = x;
    unsigned char scale = oled_font_scale(size);
    unsigned char font_width = oled_font_get_width();
    unsigned char font_pages = (unsigned char)((oled_font_get_height() + 7U) / 8U);
    unsigned char glyph_width = (unsigned char)(font_width * scale);
    unsigned char max_cols = (unsigned char)(OLED_WIDTH / glyph_width);

    if (text == 0) {
        return;
    }

    while (*text != '\0') {
        if (col >= max_cols) {
            break;
        }

        oled_draw_char(col, (unsigned char)(y * font_pages * scale), *text, scale);
        ++text;
        ++col;
    }
}

static void oled_print(unsigned char x, unsigned char y, display_font_size_t size, const char *fmt, ...)
{
    char buffer[OLED_PRINT_BUF_SIZE];
    va_list args;

    if (fmt == 0) {
        return;
    }

    va_start(args, fmt);
    (void)tiny_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    buffer[OLED_PRINT_BUF_SIZE - 1U] = '\0';
    oled_print_text(x, y, size, buffer);
}

static void oled_update(void)
{
    unsigned char page;

    for (page = 0U; page < OLED_PAGES; ++page) {
        oled_write_cmd((uint8_t)(0xB0U + page));
        oled_write_cmd(0x00U);
        oled_write_cmd(0x10U);
        oled_write_data(&oled_framebuffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}

static const display_driver_t oled_drv = {
    "oled",
    oled_init,
    oled_clear,
    oled_update,
    oled_print
};

REGISTER_DRIVER(DISPLAY, oled_drv);
