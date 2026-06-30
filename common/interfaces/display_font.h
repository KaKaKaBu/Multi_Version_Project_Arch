#ifndef DISPLAY_FONT_H
#define DISPLAY_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct display_glyph {
    uint16_t codepoint;
    uint8_t width;
    uint8_t height;
    const uint8_t *bitmap;
} display_glyph_t;

typedef struct display_font {
    uint8_t width;
    uint8_t height;
    uint8_t bytes_per_row;
    uint8_t first_ascii;
    uint8_t last_ascii;
    const uint8_t *ascii_table;
    const display_glyph_t *custom_glyphs;
    uint16_t custom_glyph_count;
} display_font_t;

const display_glyph_t *display_font_find_glyph(const display_font_t *font, uint16_t codepoint);
const uint8_t *display_font_get_ascii_glyph(const display_font_t *font, char ch);

#ifdef __cplusplus
}
#endif

#endif
