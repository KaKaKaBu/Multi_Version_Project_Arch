/**
 * @file stepper_if.h
 * @brief 步进电机驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(stepper, 实例)；devmgr_get_stepper。
 */

#ifndef STEPPER_IF_H
#define STEPPER_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 步进电机控制器的虚函数表。 */
typedef struct stepper_driver {
    const char *name;              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);            ///< 一次性硬件初始化。
    /**
     * @brief 按角度步进。
     * @param[in] direction 旋转方向。
     * @param[in] angle_deg 步进角度（度）。
     * @param[in] step_delay_ms 步间延时（毫秒）。
     */
    void (*step_angle)(unsigned char direction, unsigned short angle_deg, unsigned short step_delay_ms);
    void (*stop)(void);            ///< 停止运动；保持或释放线圈由驱动策略决定。
} stepper_driver_t;

#ifdef __cplusplus
}
#endif

#endif
