/**
 * @file oled_font.h
 * @brief Public accessors for the 6x8 ASCII OLED bitmap font.
 */

#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the width of each glyph in pixels.
 * @return Glyph width in pixels.
 */
uint8_t oled_font_get_width(void);

/**
 * @brief Returns the height of each glyph in pixels.
 * @return Glyph height in pixels.
 */
uint8_t oled_font_get_height(void);

/**
 * @brief Returns the number of bytes per vertical page column.
 * @return Bytes per glyph column page.
 */
uint8_t oled_font_get_col_bytes(void);

/**
 * @brief Returns the bitmap data for a printable ASCII character.
 *
 * @param ch Character code; out-of-range values map to '?'.
 * @return Pointer to a static 6-byte glyph column array.
 */
const uint8_t *oled_font_get_glyph(char ch);

#ifdef __cplusplus
}
#endif

#endif
