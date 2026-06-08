/**
 * @file bp_if.h
 * @brief 血压计传感器驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(blood_pressure, 实例)；devmgr_get_blood_pressure。
 */

#ifndef BP_IF_H
#define BP_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 血压测量设备的虚函数表。 */
typedef struct blood_pressure_sensor {
    const char *name;                            ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);                          ///< 一次性硬件初始化。
    /**
     * @brief 读取收缩压。
     * @return 收缩压（mmHg）。
     */
    unsigned char (*read_systolic)(void);
    /**
     * @brief 读取舒张压。
     * @return 舒张压（mmHg）。
     */
    unsigned char (*read_diastolic)(void);
} blood_pressure_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
