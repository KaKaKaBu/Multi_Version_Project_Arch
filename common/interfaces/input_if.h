/**
 * @file input_if.h
 * @brief 键盘或按键矩阵输入驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(input, 实例)；devmgr_get_input。
 * - read_key：0 无键，1–N 为键索引（由驱动定义映射）。
 */

#ifndef INPUT_IF_H
#define INPUT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 键盘或按键输入驱动的虚函数表。 */
typedef struct input_driver {
    const char *name;                    ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);                  ///< 一次性硬件初始化。
    /**
     * @brief 读取当前按键。
     * @return 按下时返回键索引 1–N，无按键时返回 0。
     */
    unsigned char (*read_key)(void);
} input_driver_t;

#ifdef __cplusplus
}
#endif

#endif
