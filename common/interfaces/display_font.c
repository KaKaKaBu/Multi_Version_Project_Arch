#include "display_font.h"

const display_glyph_t *display_font_find_glyph(const display_font_t *font, uint16_t codepoint)
{
    uint16_t i;

    if ((font == 0) || (font->custom_glyphs == 0)) {
        return 0;
    }

    for (i = 0U; i < font->custom_glyph_count; ++i) {
        if (font->custom_glyphs[i].codepoint == codepoint) {
            return &font->custom_glyphs[i];
        }
    }

    return 0;
}

const uint8_t *display_font_get_ascii_glyph(const display_font_t *font, char ch)
{
    uint8_t c = (uint8_t)ch;
    uint16_t index;
    uint16_t glyph_size;

    if ((font == 0) || (font->ascii_table == 0)) {
        return 0;
    }

    if ((c < font->first_ascii) || (c > font->last_ascii)) {
        c = (uint8_t)'?';
    }

    if ((c < font->first_ascii) || (c > font->last_ascii)) {
        return 0;
    }

    glyph_size = (uint16_t)font->bytes_per_row * font->height;
    index = (uint16_t)(c - font->first_ascii);
    return &font->ascii_table[(uint16_t)(index * glyph_size)];
}
