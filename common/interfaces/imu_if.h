/**
 * @file imu_if.h
 * @brief 惯性测量单元（加速度计/陀螺仪）驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(imu_sensor, 实例)；devmgr_get_imu_sensor。
 * - read_sample 一次填充六轴原始值（单位由驱动定义）。
 */

#ifndef IMU_IF_H
#define IMU_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 含原始加速度计与陀螺仪各轴的单次 IMU 采样。 */
typedef struct imu_sample {
    short ax;  ///< 加速度计 X 轴（驱动原始单位）。
    short ay;  ///< 加速度计 Y 轴。
    short az;  ///< 加速度计 Z 轴。
    short gx;  ///< 陀螺仪 X 轴。
    short gy;  ///< 陀螺仪 Y 轴。
    short gz;  ///< 陀螺仪 Z 轴。
} imu_sample_t;

/** @brief 六轴 IMU 驱动的虚函数表。 */
typedef struct imu_sensor {
    const char *name;                    ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);                  ///< 一次性硬件初始化。
    /**
     * @brief 填入最新加速度计与陀螺仪读数。
     * @param[out] sample 输出采样缓冲；不可为 null。
     */
    void (*read_sample)(imu_sample_t *sample);
} imu_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
