#include "lcd_color_spi.h"
#include "driver_core.h"

#if HAL_SPI_USE_SOFT || HAL_SPI_USE_HW

#define ST7789_SWRESET 0x01U
#define ST7789_SLPOUT  0x11U
#define ST7789_DISPON  0x29U
#define ST7789_CASET   0x2AU
#define ST7789_RASET   0x2BU
#define ST7789_RAMWR   0x2CU
#define ST7789_MADCTL  0x36U
#define ST7789_COLMOD  0x3AU
#define ST7789_INVON   0x21U
#define ST7789_INVOFF  0x20U

static void st7789_write_u16(uint16_t value)
{
    uint8_t data[2] = {
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFFU)
    };
    lcd_color_write_data(data, 2U);
}

static void st7789_init_panel(const spi_lcd_driver_config_t *config)
{
    uint8_t madctl = 0x00U;

    lcd_color_write_cmd(ST7789_SWRESET);
    lcd_color_delay_ms(120U);
    lcd_color_write_cmd(ST7789_SLPOUT);
    lcd_color_delay_ms(120U);

    lcd_color_write_cmd(ST7789_COLMOD);
    lcd_color_write_data_u8(0x55U);

    switch (config->rotation & 0x03U) {
    case 1U:
        madctl = 0x60U;
        break;
    case 2U:
        madctl = 0xC0U;
        break;
    case 3U:
        madctl = 0xA0U;
        break;
    default:
        madctl = 0x00U;
        break;
    }
    lcd_color_write_cmd(ST7789_MADCTL);
    lcd_color_write_data_u8(madctl);

    lcd_color_write_cmd(config->invert_colors ? ST7789_INVON : ST7789_INVOFF);
    lcd_color_write_cmd(ST7789_DISPON);
    lcd_color_delay_ms(20U);
}

static void st7789_set_window(const spi_lcd_driver_config_t *config, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 = (uint16_t)(x0 + config->x_offset);
    x1 = (uint16_t)(x1 + config->x_offset);
    y0 = (uint16_t)(y0 + config->y_offset);
    y1 = (uint16_t)(y1 + config->y_offset);

    lcd_color_write_cmd(ST7789_CASET);
    st7789_write_u16(x0);
    st7789_write_u16(x1);
    lcd_color_write_cmd(ST7789_RASET);
    st7789_write_u16(y0);
    st7789_write_u16(y1);
    lcd_color_write_cmd(ST7789_RAMWR);
}

static const lcd_color_panel_ops_t st7789_ops = {
    "st7789",
    st7789_init_panel,
    st7789_set_window
};

static void st7789_init(const void *config)
{
    lcd_color_bind(&st7789_ops);
    lcd_color_init(config);
}

const display_driver_t st7789_drv = {
    "st7789",
    st7789_init,
    lcd_color_clear,
    lcd_color_update,
    lcd_color_print,
    lcd_color_width,
    lcd_color_height,
    lcd_color_set_font,
    lcd_color_draw_pixel,
    lcd_color_fill_rect,
    lcd_color_draw_text
};

REGISTER_DRIVER(DISPLAY, st7789_drv);

#endif
