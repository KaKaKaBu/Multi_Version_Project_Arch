/**
 * @file pressure_if.h
 * @brief 气压与温度传感器驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(pressure_sensor, 实例)；devmgr_get_pressure_sensor。
 */

#ifndef PRESSURE_IF_H
#define PRESSURE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 带可选温度读数的气压传感器虚函数表。 */
typedef struct pressure_sensor {
    const char *name;                      ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);                    ///< 一次性硬件初始化。
    /**
     * @brief 读取气压。
     * @return 气压值（帕斯卡）。
     */
    float (*read_pressure_pa)(void);
    /**
     * @brief 读取芯片或环境温度。
     * @return 温度值（摄氏度）。
     */
    float (*read_temperature)(void);
} pressure_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
