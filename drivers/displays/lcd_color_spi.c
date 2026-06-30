#include "lcd_color_spi.h"
#include "gpio_hal.h"
#include "spi_hal.h"
#include "hal_common.h"
#include "tiny_printf.h"
#include <stdarg.h>

#if HAL_SPI_USE_SOFT || HAL_SPI_USE_HW

#define LCD_PRINT_BUF_SIZE 96U
#define LCD_DEFAULT_WIDTH 240U
#define LCD_DEFAULT_HEIGHT 320U
#define LCD_FALLBACK_FONT_WIDTH 8U
#define LCD_FALLBACK_FONT_HEIGHT 16U
#define LCD_COLOR_WHITE 0xFFFFU

static const spi_lcd_driver_config_t *lcd_config;
static const lcd_color_panel_ops_t *lcd_ops;
static const display_font_t *lcd_font;

static uint16_t lcd_resolved_width(void)
{
    return ((lcd_config != 0) && (lcd_config->width != 0U)) ? lcd_config->width : LCD_DEFAULT_WIDTH;
}

static uint16_t lcd_resolved_height(void)
{
    return ((lcd_config != 0) && (lcd_config->height != 0U)) ? lcd_config->height : LCD_DEFAULT_HEIGHT;
}

static uint16_t lcd_fg_color(void)
{
    return (lcd_config != 0) ? lcd_config->foreground_color : LCD_COLOR_WHITE;
}

static uint16_t lcd_bg_color(void)
{
    return (lcd_config != 0) ? lcd_config->background_color : 0U;
}

static void lcd_cs(uint8_t selected)
{
    if (lcd_config == 0) {
        return;
    }
    gpio_hal_write(lcd_config->spi.cs.port, lcd_config->spi.cs.pin, selected ? 0U : 1U);
}

static void lcd_dc(uint8_t data_mode)
{
    if (lcd_config == 0) {
        return;
    }
    gpio_hal_write(lcd_config->dc.port, lcd_config->dc.pin, data_mode ? 1U : 0U);
}

void lcd_color_write_cmd(uint8_t cmd)
{
    if (lcd_config == 0) {
        return;
    }
    lcd_dc(0U);
    lcd_cs(1U);
    (void)spi_hal_write(lcd_config->spi.bus_id, &cmd, 1U);
    lcd_cs(0U);
}

void lcd_color_write_data_u8(uint8_t data)
{
    lcd_color_write_data(&data, 1U);
}

void lcd_color_write_data(const uint8_t *data, uint16_t len)
{
    if ((lcd_config == 0) || (data == 0) || (len == 0U)) {
        return;
    }
    lcd_dc(1U);
    lcd_cs(1U);
    (void)spi_hal_write(lcd_config->spi.bus_id, data, len);
    lcd_cs(0U);
}

void lcd_color_delay_ms(uint16_t ms)
{
    hal_delay_us((uint32_t)ms * 1000UL);
}

void lcd_color_bind(const lcd_color_panel_ops_t *ops)
{
    lcd_ops = ops;
}

static void lcd_write_color_repeat(uint16_t color, uint32_t count)
{
    uint8_t chunk[64];
    uint16_t i;

    for (i = 0U; i < (uint16_t)sizeof(chunk); i += 2U) {
        chunk[i] = (uint8_t)(color >> 8);
        chunk[(uint16_t)(i + 1U)] = (uint8_t)(color & 0xFFU);
    }

    while (count != 0U) {
        uint16_t pixels = (count > 32UL) ? 32U : (uint16_t)count;
        lcd_color_write_data(chunk, (uint16_t)(pixels * 2U));
        count -= pixels;
    }
}

void lcd_color_init(const void *config)
{
    spi_hal_config_t spi_cfg;

    lcd_config = (const spi_lcd_driver_config_t *)config;
    if (lcd_config == 0) {
        return;
    }

    gpio_hal_config_pin(&lcd_config->spi.cs);
    gpio_hal_config_pin(&lcd_config->dc);
    gpio_hal_config_pin(&lcd_config->rst);
    lcd_cs(0U);
    lcd_dc(1U);
    gpio_hal_write(lcd_config->rst.port, lcd_config->rst.pin, 1U);
    hal_delay_us(5000U);
    gpio_hal_write(lcd_config->rst.port, lcd_config->rst.pin, 0U);
    hal_delay_us(10000U);
    gpio_hal_write(lcd_config->rst.port, lcd_config->rst.pin, 1U);
    hal_delay_us(120000U);

#if HAL_SPI_USE_HW
    spi_cfg.instance = lcd_config->spi.instance;
#endif
    spi_cfg.sck = lcd_config->spi.sck;
    spi_cfg.mosi = lcd_config->spi.mosi;
    spi_cfg.miso = lcd_config->spi.miso;
#if HAL_SPI_USE_HW
    spi_cfg.remap = lcd_config->spi.remap;
    spi_cfg.prescaler = lcd_config->spi.prescaler;
#endif
    spi_cfg.cpol = lcd_config->spi.cpol;
    spi_cfg.cpha = lcd_config->spi.cpha;
#if HAL_SPI_USE_SOFT
    spi_cfg.half_period_us = 1U;
#endif
#if HAL_SPI_USE_HW
    spi_cfg.timeout_us = SPI_HAL_DEFAULT_TIMEOUT_US;
#if HAL_SPI_ENABLE_DMA
    spi_cfg.tx_dma_channel = lcd_config->spi.tx_dma_channel;
    spi_cfg.rx_dma_channel = lcd_config->spi.rx_dma_channel;
#endif
#endif
    (void)spi_hal_init(lcd_config->spi.bus_id, &spi_cfg);

    lcd_font = lcd_config->font;
    if ((lcd_ops != 0) && (lcd_ops->init_panel != 0)) {
        lcd_ops->init_panel(lcd_config);
    }
    lcd_color_clear();
}

void lcd_color_clear(void)
{
    lcd_color_fill_rect(0U, 0U, lcd_resolved_width(), lcd_resolved_height(), lcd_bg_color());
}

void lcd_color_update(void)
{
}

uint16_t lcd_color_width(void)
{
    return lcd_resolved_width();
}

uint16_t lcd_color_height(void)
{
    return lcd_resolved_height();
}

void lcd_color_set_font(const display_font_t *font)
{
    lcd_font = font;
}

void lcd_color_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= lcd_resolved_width()) || (y >= lcd_resolved_height())) {
        return;
    }
    if ((lcd_ops == 0) || (lcd_ops->set_window == 0) || (lcd_config == 0)) {
        return;
    }
    lcd_ops->set_window(lcd_config, x, y, x, y);
    lcd_write_color_repeat(color, 1U);
}

void lcd_color_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t x1;
    uint16_t y1;

    if ((width == 0U) || (height == 0U) || (x >= lcd_resolved_width()) || (y >= lcd_resolved_height())) {
        return;
    }
    if ((lcd_ops == 0) || (lcd_ops->set_window == 0) || (lcd_config == 0)) {
        return;
    }

    if ((uint32_t)x + width > lcd_resolved_width()) {
        width = (uint16_t)(lcd_resolved_width() - x);
    }
    if ((uint32_t)y + height > lcd_resolved_height()) {
        height = (uint16_t)(lcd_resolved_height() - y);
    }

    x1 = (uint16_t)(x + width - 1U);
    y1 = (uint16_t)(y + height - 1U);
    lcd_ops->set_window(lcd_config, x, y, x1, y1);
    lcd_write_color_repeat(color, (uint32_t)width * height);
}

static uint16_t lcd_decode_utf8(const char **text)
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

static void lcd_draw_bitmap_glyph(uint16_t x, uint16_t y, uint8_t width, uint8_t height,
                                  const uint8_t *bitmap, uint8_t bytes_per_row,
                                  uint8_t scale, uint16_t color)
{
    uint8_t row;
    uint8_t col;

    if ((bitmap == 0) || (bytes_per_row == 0U)) {
        return;
    }

    for (row = 0U; row < height; ++row) {
        for (col = 0U; col < width; ++col) {
            uint8_t byte = bitmap[(uint16_t)row * bytes_per_row + (uint16_t)(col / 8U)];
            uint8_t bit = (uint8_t)(0x80U >> (col % 8U));
            if ((byte & bit) != 0U) {
                lcd_color_fill_rect((uint16_t)(x + (uint16_t)col * scale),
                                    (uint16_t)(y + (uint16_t)row * scale),
                                    scale, scale, color);
            }
        }
    }
}

void lcd_color_draw_text(uint16_t x, uint16_t y, display_font_size_t size, uint16_t color, const char *text)
{
    uint16_t cursor = x;
    uint8_t scale = (size == DISPLAY_FONT_LARGE) ? 2U : 1U;

    if ((text == 0) || (lcd_font == 0)) {
        return;
    }

    while (*text != '\0') {
        uint16_t cp = lcd_decode_utf8(&text);
        const display_glyph_t *glyph = display_font_find_glyph(lcd_font, cp);
        const uint8_t *bitmap = 0;
        uint8_t width = lcd_font->width;
        uint8_t height = lcd_font->height;
        uint8_t bytes_per_row = lcd_font->bytes_per_row;

        if (glyph != 0) {
            bitmap = glyph->bitmap;
            width = glyph->width;
            height = glyph->height;
            bytes_per_row = (uint8_t)((width + 7U) / 8U);
        } else if (cp < 0x80U) {
            bitmap = display_font_get_ascii_glyph(lcd_font, (char)cp);
        }

        if (bitmap != 0) {
            lcd_draw_bitmap_glyph(cursor, y, width, height, bitmap, bytes_per_row, scale, color);
            cursor = (uint16_t)(cursor + (uint16_t)width * scale + scale);
        } else {
            cursor = (uint16_t)(cursor + (uint16_t)width * scale + scale);
        }

        if (cursor >= lcd_resolved_width()) {
            break;
        }
    }
}

static uint16_t lcd_grid_x_to_pixel(unsigned char x, display_font_size_t size)
{
    uint8_t scale = (size == DISPLAY_FONT_LARGE) ? 2U : 1U;
    uint8_t width = (lcd_font != 0) ? lcd_font->width : LCD_FALLBACK_FONT_WIDTH;

    return (uint16_t)((uint16_t)x * width * scale);
}

static uint16_t lcd_grid_y_to_pixel(unsigned char y, display_font_size_t size)
{
    uint8_t scale = (size == DISPLAY_FONT_LARGE) ? 2U : 1U;
    uint8_t height = (lcd_font != 0) ? lcd_font->height : LCD_FALLBACK_FONT_HEIGHT;

    return (uint16_t)((uint16_t)y * height * scale);
}

#if defined(PLATFORM_MCS51)
void lcd_color_print(unsigned char x, unsigned char y, display_font_size_t size, const char *text) DISPLAY_IF_REENTRANT
{
    lcd_color_draw_text(lcd_grid_x_to_pixel(x, size), lcd_grid_y_to_pixel(y, size), size, lcd_fg_color(), text);
}
#else
void lcd_color_print(unsigned char x, unsigned char y, display_font_size_t size, const char *fmt, ...)
{
    char buffer[LCD_PRINT_BUF_SIZE];
    va_list args;

    if (fmt == 0) {
        return;
    }

    va_start(args, fmt);
    (void)tiny_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[LCD_PRINT_BUF_SIZE - 1U] = '\0';

    lcd_color_draw_text(lcd_grid_x_to_pixel(x, size), lcd_grid_y_to_pixel(y, size), size, lcd_fg_color(), buffer);
}
#endif

#endif
