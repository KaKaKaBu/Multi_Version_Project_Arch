#ifndef LCD_COLOR_SPI_H
#define LCD_COLOR_SPI_H

#include <stdint.h>
#include "display_if.h"
#include "driver_configs.h"

typedef struct lcd_color_panel_ops {
    const char *name;
    void (*init_panel)(const spi_lcd_driver_config_t *config);
    void (*set_window)(const spi_lcd_driver_config_t *config, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
} lcd_color_panel_ops_t;

void lcd_color_bind(const lcd_color_panel_ops_t *ops);
void lcd_color_init(const void *config);
void lcd_color_clear(void);
void lcd_color_update(void);
uint16_t lcd_color_width(void);
uint16_t lcd_color_height(void);
void lcd_color_set_font(const display_font_t *font);
void lcd_color_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_color_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void lcd_color_draw_text(uint16_t x, uint16_t y, display_font_size_t size, uint16_t color, const char *text);
#if defined(PLATFORM_MCS51)
void lcd_color_print(unsigned char x, unsigned char y, display_font_size_t size, const char *text) DISPLAY_IF_REENTRANT;
#else
void lcd_color_print(unsigned char x, unsigned char y, display_font_size_t size, const char *fmt, ...);
#endif
void lcd_color_write_cmd(uint8_t cmd);
void lcd_color_write_data_u8(uint8_t data);
void lcd_color_write_data(const uint8_t *data, uint16_t len);
void lcd_color_delay_ms(uint16_t ms);

#endif
