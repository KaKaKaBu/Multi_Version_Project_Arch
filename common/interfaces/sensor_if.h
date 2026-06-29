/**
 * @file sensor_if.h
 * @brief 温湿度传感器驱动接口。
 *
 * ## 驱动模型
 * - 在 drivers/ 实现 temp_hum_sensor_t，设置 name 与各函数指针。
 * - REGISTER_DRIVER(temp_hum_sensor, 实例) 注册；devmgr_get_temp_hum_sensor("name") 获取。
 * - init 由 devmgr_init_all 调用；其余钩子调用前须判空。
 */

#ifndef SENSOR_IF_H
#define SENSOR_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 温湿度组合传感器驱动的虚函数表。 */
typedef struct temp_hum_sensor {
    const char *name;                    ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);                  ///< 一次性硬件初始化。
    /**
     * @brief 读取摄氏温度。
     * @return 温度值（摄氏度）。
     */
    #if defined(PLATFORM_MCS51)
    int (*read_temperature)(void);
    #else
    float (*read_temperature)(void);
    #endif
    /**
     * @brief 读取相对湿度。
     * @return 湿度值（百分比）。
     */
    #if defined(PLATFORM_MCS51)
    int (*read_humidity)(void);
    #else
    float (*read_humidity)(void);
    #endif
} temp_hum_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
