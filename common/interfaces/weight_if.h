/**
 * @file weight_if.h
 * @brief 称重传感器或秤驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(weight_sensor, 实例)；devmgr_get_weight_sensor。
 */

#ifndef WEIGHT_IF_H
#define WEIGHT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 称重传感器的虚函数表。 */
typedef struct weight_sensor {
    const char *name;                 ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);               ///< 一次性硬件初始化。
    /**
     * @brief 读取质量。
     * @return 质量值（克）。
     */
    float (*read_grams)(void);
} weight_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
