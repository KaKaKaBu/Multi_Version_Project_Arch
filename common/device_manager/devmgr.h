/**
 * @file devmgr.h
 * @brief 设备管理器：遍历 linker 驱动注册表、统一 init、按名称与类型查找。
 *
 * 依赖 driver_core.h 的 __driver_list_start/end，不维护运行时链表；
 * 所有查找为 O(n) 线性扫描，适合嵌入式少量驱动场景。
 *
 * 典型启动序列：
 *   devmgr_init_all();
 *   const display_driver_t *disp = devmgr_get_display("oled");
 *   if (disp && disp->init) { ... }  // init 已由 devmgr_init_all 调用
 */

#ifndef DEVMGR_H
#define DEVMGR_H

#include "sensor_if.h"
#include "display_if.h"
#include "actuator_if.h"
#include "comm_if.h"
#include "misc_if.h"
#include "distance_if.h"
#include "weight_if.h"
#include "gas_if.h"
#include "analog_probe_if.h"
#include "pressure_if.h"
#include "imu_if.h"
#include "health_if.h"
#include "bp_if.h"
#include "rtc_if.h"
#include "input_if.h"
#include "stepper_if.h"
#include "segment_if.h"
#include "rfid_if.h"
#include "radio_if.h"
#include "gnss_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 遍历注册表，对每条目的 init 函数指针（非 null）调用一次。 */
void devmgr_init_all(void);

/** @brief 按 name 精确匹配查找温湿度传感器；未找到返回 null。 */
const temp_hum_sensor_t *devmgr_get_temp_hum_sensor(const char *name);
const display_driver_t *devmgr_get_display(const char *name);
const servo_driver_t *devmgr_get_servo(const char *name);
const relay_driver_t *devmgr_get_relay(const char *name);
const comm_driver_t *devmgr_get_comm(const char *name);
const misc_driver_t *devmgr_get_misc(const char *name);
const distance_sensor_t *devmgr_get_distance_sensor(const char *name);
const weight_sensor_t *devmgr_get_weight_sensor(const char *name);
const gas_sensor_t *devmgr_get_gas_sensor(const char *name);
const analog_probe_t *devmgr_get_analog_probe(const char *name);
const pressure_sensor_t *devmgr_get_pressure_sensor(const char *name);
const imu_sensor_t *devmgr_get_imu_sensor(const char *name);
const pulse_oximeter_t *devmgr_get_pulse_oximeter(const char *name);
const blood_pressure_sensor_t *devmgr_get_blood_pressure(const char *name);
const rtc_driver_t *devmgr_get_rtc(const char *name);
const input_driver_t *devmgr_get_input(const char *name);
const stepper_driver_t *devmgr_get_stepper(const char *name);
const segment_display_t *devmgr_get_segment_display(const char *name);
const rfid_driver_t *devmgr_get_rfid(const char *name);

/**
 * @brief 查找无线/分组通信驱动。
 *
 * 优先：同名 comm 驱动且 kind==COMM_KIND_PACKET；
 * 否则回退扫描 DRIVER_TYPE_RADIO 遗留条目。
 */
const radio_driver_t *devmgr_get_radio(const char *name);

const gnss_driver_t *devmgr_get_gnss(const char *name);

/** @brief 返回 (__driver_list_end - __driver_list_start) 注册项个数。 */
unsigned short devmgr_count(void);

#ifdef __cplusplus
}
#endif

#endif
