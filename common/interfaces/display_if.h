#ifndef DISPLAY_IF_H
#define DISPLAY_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum display_font_size {
    DISPLAY_FONT_SMALL = 0U,
    DISPLAY_FONT_LARGE = 1U
} display_font_size_t;

typedef struct display_driver {
    const char *name;
    void (*init)(void);
    void (*clear)(void);
    void (*update)(void);
    void (*print)(unsigned char x, unsigned char y, display_font_size_t size, const char *fmt, ...);
} display_driver_t;

#define DISPLAY_PRINT(display, x, y, size, ...) \
    ((display)->print((x), (y), (size), __VA_ARGS__))

#ifdef __cplusplus
}
#endif

#endif
