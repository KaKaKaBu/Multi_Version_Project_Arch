/**
 * @file analog_probe_if.h
 * @brief 通用模拟探头或 ADC 通道驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(analog_probe, 实例)；devmgr_get_analog_probe。
 */

#ifndef ANALOG_PROBE_IF_H
#define ANALOG_PROBE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 单通道模拟探头的虚函数表。 */
typedef struct analog_probe {
    const char *name;              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);            ///< 一次性硬件初始化。
    /**
     * @brief 读取换算后的工程量。
     * @return 数值（单位由驱动定义）。
     */
    float (*read_value)(void);
} analog_probe_t;

#ifdef __cplusplus
}
#endif

#endif
