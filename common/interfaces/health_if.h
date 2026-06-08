/**
 * @file health_if.h
 * @brief 脉搏血氧仪（心率与 SpO2）驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(pulse_oximeter, 实例)；devmgr_get_pulse_oximeter。
 */

#ifndef HEALTH_IF_H
#define HEALTH_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 脉搏血氧仪传感器的虚函数表。 */
typedef struct pulse_oximeter {
    const char *name;                         ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);                       ///< 一次性硬件初始化。
    /**
     * @brief 读取心率。
     * @return 心率（次/分）。
     */
    unsigned char (*read_heart_rate)(void);
    /**
     * @brief 读取血氧饱和度。
     * @return SpO2（百分比）。
     */
    unsigned char (*read_spo2)(void);
} pulse_oximeter_t;

#ifdef __cplusplus
}
#endif

#endif
