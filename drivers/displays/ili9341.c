#include "lcd_color_spi.h"
#include "driver_core.h"

#if HAL_SPI_USE_SOFT || HAL_SPI_USE_HW

#define ILI9341_SWRESET 0x01U
#define ILI9341_SLPOUT  0x11U
#define ILI9341_DISPON  0x29U
#define ILI9341_CASET   0x2AU
#define ILI9341_PASET   0x2BU
#define ILI9341_RAMWR   0x2CU
#define ILI9341_MADCTL  0x36U
#define ILI9341_PIXFMT  0x3AU
#define ILI9341_INVON   0x21U
#define ILI9341_INVOFF  0x20U

static void ili9341_write_u16(uint16_t value)
{
    uint8_t data[2] = {
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFFU)
    };
    lcd_color_write_data(data, 2U);
}

static void ili9341_init_panel(const spi_lcd_driver_config_t *config)
{
    uint8_t madctl = 0x48U;

    lcd_color_write_cmd(ILI9341_SWRESET);
    lcd_color_delay_ms(120U);
    lcd_color_write_cmd(ILI9341_SLPOUT);
    lcd_color_delay_ms(120U);

    lcd_color_write_cmd(ILI9341_PIXFMT);
    lcd_color_write_data_u8(0x55U);

    switch (config->rotation & 0x03U) {
    case 1U:
        madctl = 0x28U;
        break;
    case 2U:
        madctl = 0x88U;
        break;
    case 3U:
        madctl = 0xE8U;
        break;
    default:
        madctl = 0x48U;
        break;
    }
    lcd_color_write_cmd(ILI9341_MADCTL);
    lcd_color_write_data_u8(madctl);

    lcd_color_write_cmd(config->invert_colors ? ILI9341_INVON : ILI9341_INVOFF);
    lcd_color_write_cmd(ILI9341_DISPON);
    lcd_color_delay_ms(20U);
}

static void ili9341_set_window(const spi_lcd_driver_config_t *config, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 = (uint16_t)(x0 + config->x_offset);
    x1 = (uint16_t)(x1 + config->x_offset);
    y0 = (uint16_t)(y0 + config->y_offset);
    y1 = (uint16_t)(y1 + config->y_offset);

    lcd_color_write_cmd(ILI9341_CASET);
    ili9341_write_u16(x0);
    ili9341_write_u16(x1);
    lcd_color_write_cmd(ILI9341_PASET);
    ili9341_write_u16(y0);
    ili9341_write_u16(y1);
    lcd_color_write_cmd(ILI9341_RAMWR);
}

static const lcd_color_panel_ops_t ili9341_ops = {
    "ili9341",
    ili9341_init_panel,
    ili9341_set_window
};

static void ili9341_init(const void *config)
{
    lcd_color_bind(&ili9341_ops);
    lcd_color_init(config);
}

const display_driver_t ili9341_drv = {
    "ili9341",
    ili9341_init,
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

REGISTER_DRIVER(DISPLAY, ili9341_drv);

#endif
