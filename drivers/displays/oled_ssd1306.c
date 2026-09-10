/**
 * @file oled_ssd1306.c
 * @brief SSD1306 128x64 I2C OLED display driver implementing display_if.h.
 */

#include "display_if.h"
#include "oled_cn_font.h"
#include "oled_font.h"
#include "tiny_printf.h"
#include "i2c_hal.h"
#include "driver_configs.h"
#include "driver_core.h"
#if !defined(PLATFORM_MCS51)
#endif
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
static const i2c_device_config_t *oled_config;
static const display_font_t *oled_custom_font;

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

static void oled_draw_pixel_public(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;
    }
    oled_set_pixel((unsigned char)x, (unsigned char)y, (color != 0U) ? 1U : 0U);
}

static void oled_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t px;
    uint16_t py;
    uint16_t x_end = (uint16_t)(x + width);
    uint16_t y_end = (uint16_t)(y + height);

    if (x_end > OLED_WIDTH) {
        x_end = OLED_WIDTH;
    }
    if (y_end > OLED_HEIGHT) {
        y_end = OLED_HEIGHT;
    }

    for (py = y; py < y_end; ++py) {
        for (px = x; px < x_end; ++px) {
            oled_draw_pixel_public(px, py, color);
        }
    }
}

/**
 * @brief Sends a single SSD1306 command byte over I2C.
 * @param cmd SSD1306 command register value.
 */
static void oled_write_cmd(uint8_t cmd)
{
    if (oled_config == 0) {
        return;
    }
    (void)i2c_hal_write(oled_config->instance, oled_config->address, OLED_CTRL_CMD, &cmd, 1U);
}

/**
 * @brief Sends a block of display data bytes over I2C.
 * @param data Pointer to display RAM bytes to write.
 * @param len Number of bytes to transmit.
 */
static void oled_write_data(const uint8_t *data, uint16_t len)
{
    if (oled_config == 0) {
        return;
    }
    (void)i2c_hal_write(oled_config->instance, oled_config->address, OLED_CTRL_DATA, data, len);
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

    oled_custom_font = &oled_cn_font;
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

static uint16_t oled_decode_utf8(const char **text)
{
    const uint8_t *s = (const uint8_t *)(*text);
    uint16_t cp;

    if (s[0] < 0x80U) {
        *text += 1;
        return s[0];
    }
    if (((s[0] & 0xE0U) == 0xC0U) && ((s[1] & 0xC0U) == 0x80U)) {
        cp = (uint16_t)(((uint16_t)(s[0] & 0x1FU) << 6) | (uint16_t)(s[1] & 0x3FU));
        *text += 2;
        return cp;
    }
    if (((s[0] & 0xF0U) == 0xE0U) && ((s[1] & 0xC0U) == 0x80U) && ((s[2] & 0xC0U) == 0x80U)) {
        cp = (uint16_t)(((uint16_t)(s[0] & 0x0FU) << 12) |
                        ((uint16_t)(s[1] & 0x3FU) << 6) |
                        (uint16_t)(s[2] & 0x3FU));
        *text += 3;
        return cp;
    }

    *text += 1;
    return (uint16_t)'?';
}

static void oled_draw_ascii_pixel(uint16_t x, uint16_t y, char ch, unsigned char scale, uint16_t color)
{
    const uint8_t *glyph;
    unsigned char font_width = oled_font_get_width();
    unsigned char font_height = oled_font_get_height();
    unsigned char col_bytes = oled_font_get_col_bytes();
    unsigned char font_pages = (unsigned char)((font_height + 7U) / 8U);
    unsigned char page_offset;
    unsigned char col;
    unsigned char bit;
    unsigned char dx;
    unsigned char dy;

    if ((ch < 0x20) || (ch > 0x7E)) {
        ch = '?';
    }

    glyph = oled_font_get_glyph(ch);
    if (glyph == 0) {
        return;
    }

    for (page_offset = 0U; page_offset < font_pages; ++page_offset) {
        for (col = 0U; col < font_width; ++col) {
            uint8_t column_data = glyph[(col * col_bytes) + page_offset];

            for (bit = 0U; bit < 8U; ++bit) {
                if (((page_offset * 8U) + bit) >= font_height) {
                    continue;
                }
                if ((column_data & (uint8_t)(1U << bit)) == 0U) {
                    continue;
                }
                for (dy = 0U; dy < scale; ++dy) {
                    for (dx = 0U; dx < scale; ++dx) {
                        oled_draw_pixel_public((uint16_t)(x + (uint16_t)col * scale + dx),
                                               (uint16_t)(y + (uint16_t)(page_offset * 8U + bit) * scale + dy),
                                               color);
                    }
                }
            }
        }
    }
}

static void oled_draw_bitmap_glyph(uint16_t x, uint16_t y, uint8_t width, uint8_t height,
                                   const uint8_t *bitmap, uint8_t bytes_per_row,
                                   uint8_t scale, uint16_t color)
{
    uint8_t row;
    uint8_t col;
    uint8_t dx;
    uint8_t dy;

    if ((bitmap == 0) || (bytes_per_row == 0U)) {
        return;
    }

    for (row = 0U; row < height; ++row) {
        for (col = 0U; col < width; ++col) {
            uint8_t byte = bitmap[(uint16_t)row * bytes_per_row + (uint16_t)(col / 8U)];
            uint8_t bit = (uint8_t)(0x80U >> (col % 8U));
            if ((byte & bit) == 0U) {
                continue;
            }
            for (dy = 0U; dy < scale; ++dy) {
                for (dx = 0U; dx < scale; ++dx) {
                    oled_draw_pixel_public((uint16_t)(x + (uint16_t)col * scale + dx),
                                           (uint16_t)(y + (uint16_t)row * scale + dy),
                                           color);
                }
            }
        }
    }
}

static void oled_draw_text(uint16_t x, uint16_t y, display_font_size_t size, uint16_t color, const char *text)
{
    uint16_t cursor = x;
    uint8_t scale = oled_font_scale(size);

    if (text == 0) {
        return;
    }

    while (*text != '\0') {
        uint16_t cp = oled_decode_utf8(&text);
        const display_glyph_t *glyph = display_font_find_glyph(oled_custom_font, cp);

        if (glyph != 0) {
            uint8_t bytes_per_row = (uint8_t)((glyph->width + 7U) / 8U);
            oled_draw_bitmap_glyph(cursor, y, glyph->width, glyph->height, glyph->bitmap, bytes_per_row, scale, color);
            cursor = (uint16_t)(cursor + (uint16_t)glyph->width * scale + scale);
        } else if (cp < 0x80U) {
            oled_draw_ascii_pixel(cursor, y, (char)cp, scale, color);
            cursor = (uint16_t)(cursor + (uint16_t)oled_font_get_width() * scale + scale);
        } else {
            cursor = (uint16_t)(cursor + ((oled_custom_font != 0) ? oled_custom_font->width : oled_font_get_width()) * scale + scale);
        }

        if (cursor >= OLED_WIDTH) {
            break;
        }
    }
}

static void oled_set_font(const display_font_t *font)
{
    oled_custom_font = font;
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

    if (oled_custom_font != 0) {
        oled_draw_text((uint16_t)((uint16_t)x * oled_font_get_width() * scale),
                       (uint16_t)((uint16_t)y * oled_custom_font->height * scale),
                       size,
                       1U,
                       text);
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
#if defined(PLATFORM_MCS51)
static void oled_print(unsigned char x, unsigned char y, display_font_size_t size, const char *text) DISPLAY_IF_REENTRANT
{
    if (text == 0) {
        return;
    }

    oled_print_text(x, y, size, text);
}
#else
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
#endif

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

static uint16_t oled_width(void)
{
    return OLED_WIDTH;
}

static uint16_t oled_height(void)
{
    return OLED_HEIGHT;
}

/** @brief display_if.h driver instance registered as DISPLAY. */
const display_driver_t oled_drv = {
    "oled",
    oled_init,
    oled_clear,
    oled_update,
    oled_print,
    oled_width,
    oled_height,
    oled_set_font,
    oled_draw_pixel_public,
    oled_fill_rect,
    oled_draw_text
};

REGISTER_DRIVER(DISPLAY, oled_drv);
