/**
 * @file actuator_if.h
 * @brief 舵机与继电器执行器驱动接口。
 *
 * ## 驱动模型
 * - 舵机：REGISTER_DRIVER(servo, 实例)，devmgr_get_servo。
 * - 继电器：REGISTER_DRIVER(relay, 实例)，devmgr_get_relay。
 */

#ifndef ACTUATOR_IF_H
#define ACTUATOR_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 舵机驱动的虚函数表。 */
typedef struct servo_driver {
    const char *name;                         ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);                       ///< 一次性硬件初始化。
    /**
     * @brief 设置目标角度。
     * @param[in] angle 角度（度）。
     */
    void (*set_angle)(unsigned short angle);
} servo_driver_t;

/** @brief 继电器或数字输出驱动的虚函数表。 */
typedef struct relay_driver {
    const char *name;                        ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);                      ///< 一次性硬件初始化。
    /**
     * @brief 设置继电器线圈通电状态。
     * @param[in] on 非零表示吸合。
     */
    void (*set_state)(unsigned char on);
} relay_driver_t;

#ifdef __cplusplus
}
#endif

#endif
