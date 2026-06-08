/**
 * @file distance_if.h
 * @brief 超声波或飞行时间测距传感器驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(distance_sensor, 实例)；devmgr_get_distance_sensor。
 */

#ifndef DISTANCE_IF_H
#define DISTANCE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 测距传感器的虚函数表。 */
typedef struct distance_sensor {
    const char *name;                              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);                            ///< 一次性硬件初始化。
    /**
     * @brief 读取距离。
     * @return 距离值（厘米）。
     */
    unsigned short (*read_distance_cm)(void);
} distance_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
