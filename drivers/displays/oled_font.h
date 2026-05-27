#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t oled_font_get_width(void);
uint8_t oled_font_get_height(void);
uint8_t oled_font_get_col_bytes(void);
const uint8_t *oled_font_get_glyph(char ch);

#ifdef __cplusplus
}
#endif

#endif
