/**
 * @file segment_if.h
 * @brief 七段或多位 LED 显示驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(segment_display, 实例)；devmgr_get_segment_display。
 */

#ifndef SEGMENT_IF_H
#define SEGMENT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 两位（或更多）数码管显示的虚函数表。 */
typedef struct segment_display {
    const char *name;              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);            ///< 一次性硬件初始化。
    /**
     * @brief 显示十位与个位数字。
     * @param[in] ten 十位数字 0–9。
     * @param[in] one 个位数字 0–9。
     */
    void (*show_digits)(unsigned char ten, unsigned char one);
} segment_display_t;

#ifdef __cplusplus
}
#endif

#endif
