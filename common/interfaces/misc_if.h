/**
 * @file misc_if.h
 * @brief 杂项 GPIO 或简单开关类设备驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(misc, 实例)；devmgr_get_misc("name")。
 */

#ifndef MISC_IF_H
#define MISC_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 通用杂项 GPIO 驱动的虚函数表。 */
typedef struct misc_driver {
    const char *name;                    ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);                  ///< 一次性硬件初始化。
    /**
     * @brief 设置 GPIO 输出状态。
     * @param[in] on 非零表示有效（极性由驱动定义）。
     */
    void (*set_state)(unsigned char on);
} misc_driver_t;

#ifdef __cplusplus
}
#endif

#endif
