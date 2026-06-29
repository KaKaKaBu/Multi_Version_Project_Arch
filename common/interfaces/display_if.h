/**
 * @file display_if.h
 * @brief 图形或字符显示驱动接口（OLED/LCD 等）。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(display, 实例)；devmgr_get_display("name")。
 * - 典型流程：clear → print → update 刷新到面板。
 */

#ifndef DISPLAY_IF_H
#define DISPLAY_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__SDCC)
#define DISPLAY_IF_REENTRANT __reentrant
#else
#define DISPLAY_IF_REENTRANT
#endif

/** @brief display_print 使用的字号选择。 */
typedef enum display_font_size {
    DISPLAY_FONT_SMALL = 0U,  ///< 小号字体。
    DISPLAY_FONT_LARGE = 1U   ///< 大号字体。
} display_font_size_t;

/** @brief 显示驱动（OLED、LCD 等）的虚函数表。 */
typedef struct display_driver {
    const char *name;              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);            ///< 一次性硬件初始化。
    void (*clear)(void);          ///< 清空帧缓冲或屏幕。
    void (*update)(void);         ///< 将待绘制内容刷新到面板。
    /**
     * @brief 在网格坐标 (x, y) 处按格式输出文本。
     * @param[in] x 列坐标。
     * @param[in] y 行坐标。
     * @param[in] size 字号（display_font_size_t）。
     * @param[in] fmt printf 风格格式串。
     */
    #if defined(PLATFORM_MCS51)
    void (*print)(unsigned char x, unsigned char y, display_font_size_t size, const char *text) DISPLAY_IF_REENTRANT;
    #else
    void (*print)(unsigned char x, unsigned char y, display_font_size_t size, const char *fmt, ...);
    #endif
} display_driver_t;

#define DISPLAY_SMALL_MAX_CHARS 21U
#define DISPLAY_LARGE_MAX_CHARS 10U
#define DISPLAY_MAX_CHARS(size) (((size) == DISPLAY_FONT_LARGE) ? DISPLAY_LARGE_MAX_CHARS : DISPLAY_SMALL_MAX_CHARS)

#if defined(__GNUC__)
#define DISPLAY_REQUIRE_LITERAL(fmt) \
    ((void)sizeof(char[(!__builtin_types_compatible_p(__typeof__(fmt), char *) && \
                         !__builtin_types_compatible_p(__typeof__(fmt), const char *)) ? 1 : -1]))
#else
#define DISPLAY_REQUIRE_LITERAL(fmt) ((void)0)
#endif

#define DISPLAY_CHECK_TEXT_FITS(x, size, fmt) \
    ((void)sizeof(char[(((x) < DISPLAY_MAX_CHARS(size)) && \
                         ((sizeof(fmt) - 1U) <= (DISPLAY_MAX_CHARS(size) - (x)))) ? 1 : -1]))

#define DISPLAY_PRINT(display, x, y, size, fmt, ...) \
    do { \
        DISPLAY_REQUIRE_LITERAL(fmt); \
        DISPLAY_CHECK_TEXT_FITS((x), (size), (fmt)); \
        (display)->print((x), (y), (size), (fmt), ##__VA_ARGS__); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
