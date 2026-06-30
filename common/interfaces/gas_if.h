/**
 * @file gas_if.h
 * @brief 空气质量或气体浓度传感器驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(gas_sensor, 实例)；devmgr_get_gas_sensor。
 */

#ifndef GAS_IF_H
#define GAS_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 气体或 VOC 传感器的虚函数表。 */
typedef struct gas_sensor {
    const char *name;                    ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);                  ///< 一次性硬件初始化。
    /**
     * @brief 读取浓度。
     * @return 浓度值（ppm）。
     */
    unsigned short (*read_ppm)(void);
} gas_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
