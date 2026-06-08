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
    void (*print)(unsigned char x, unsigned char y, display_font_size_t size, const char *fmt, ...);
} display_driver_t;

/**
 * @brief display_driver_t::print 的便捷封装。
 * @param[in] display 显示驱动指针。
 * @param[in] x 列坐标。
 * @param[in] y 行坐标。
 * @param[in] size 字号（display_font_size_t）。
 * @param[in] ... printf 风格格式串及参数。
 */
#define DISPLAY_PRINT(display, x, y, size, ...) \
    ((display)->print((x), (y), (size), __VA_ARGS__))

#ifdef __cplusplus
}
#endif

#endif
