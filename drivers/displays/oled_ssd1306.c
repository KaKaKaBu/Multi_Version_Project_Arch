/**
 * @file oled_ssd1306.c
 * @brief SSD1306 128x64 I2C OLED display driver implementing display_if.h.
 */

#include "display_if.h"
#include "oled_font.h"
#include "tiny_printf.h"
#include "i2c_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"
#include <stdarg.h>

/** @brief Panel horizontal resolution in pixels. */
#define OLED_WIDTH 128U
/** @brief Panel vertical resolution in pixels. */
#define OLED_HEIGHT 64U
/** @brief Number of SSD1306 RAM pages (8 pixels per page). */
#define OLED_PAGES 8U
/** @brief I2C control byte selecting command stream mode. */
#define OLED_CTRL_CMD 0x00U
/** @brief I2C control byte selecting GDDRAM data stream mode. */
#define OLED_CTRL_DATA 0x40U
/** @brief Stack buffer size for tiny_vsnprintf in oled_print(). */
#define OLED_PRINT_BUF_SIZE 64U

/** @brief Internal 128x64 monochrome framebuffer in SSD1306 page layout. */
static uint8_t oled_framebuffer[OLED_WIDTH * OLED_PAGES];

/** @brief Forward declaration: zeroes the internal framebuffer. */
static void oled_clear(void);
/** @brief Forward declaration: flushes the framebuffer to the panel over I2C. */
static void oled_update(void);

/**
 * @brief Maps display font size to pixel scale factor (1 or 2).
 * @param size Font size selector from display_if.h.
 * @return Scale factor: 2 for DISPLAY_FONT_LARGE, 1 otherwise.
 */
static unsigned char oled_font_scale(display_font_size_t size)
{
    return (size == DISPLAY_FONT_LARGE) ? 2U : 1U;
}

/**
 * @brief Sets or clears a single pixel in the internal framebuffer.
 * @param x Horizontal pixel coordinate (0-127).
 * @param y Vertical pixel coordinate (0-63).
 * @param on Non-zero to set the pixel; zero to clear it.
 */
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

/**
 * @brief Sends a single SSD1306 command byte over I2C.
 * @param cmd SSD1306 command register value.
 */
static void oled_write_cmd(uint8_t cmd)
{
    (void)i2c_hal_write(BOARD_OLED_I2C, BOARD_OLED_I2C_ADDR, OLED_CTRL_CMD, &cmd, 1U);
}

/**
 * @brief Sends a block of display data bytes over I2C.
 * @param data Pointer to display RAM bytes to write.
 * @param len Number of bytes to transmit.
 */
static void oled_write_data(const uint8_t *data, uint16_t len)
{
    (void)i2c_hal_write(BOARD_OLED_I2C, BOARD_OLED_I2C_ADDR, OLED_CTRL_DATA, data, len);
}

/** @brief Applies the SSD1306 power-on initialization command sequence. */
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

/** @brief Initializes I2C and the SSD1306 panel, then clears the screen. */
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

/** @brief Zeroes the internal 128x64 pixel framebuffer. */
static void oled_clear(void)
{
    uint16_t i;

    for (i = 0U; i < (uint16_t)(sizeof(oled_framebuffer)); ++i) {
        oled_framebuffer[i] = 0U;
    }
}

/**
 * @brief Renders one scaled ASCII character into the framebuffer.
 * @param col Character grid column index.
 * @param page Starting SSD1306 page row for the glyph.
 * @param ch ASCII character to draw.
 * @param scale Pixel scale factor (1 or 2).
 */
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

/**
 * @brief Draws a null-terminated string at a grid column and row.
 * @param x Starting character column.
 * @param y Starting character row.
 * @param size Font size selector.
 * @param text Null-terminated string to render.
 */
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

/**
 * @brief Formats text with tiny_vsnprintf and draws it on the display.
 * @param x Starting character column.
 * @param y Starting character row.
 * @param size Font size selector.
 * @param fmt printf-style format string.
 * @param ... Arguments for fmt.
 */
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

/** @brief Flushes the framebuffer to the SSD1306 over I2C page by page. */
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

/** @brief display_if.h driver instance registered as DISPLAY. */
static const display_driver_t oled_drv = {
    "oled",
    oled_init,
    oled_clear,
    oled_update,
    oled_print
};

REGISTER_DRIVER(DISPLAY, oled_drv);
