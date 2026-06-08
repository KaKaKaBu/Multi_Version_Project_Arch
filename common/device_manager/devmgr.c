/**
 * @file devmgr.c
 * @brief 设备管理器实现：注册表遍历、类型安全 init、宏生成的 get 查找函数。
 */

#include "devmgr.h"
#include "driver_core.h"

/**
 * @brief 区分大小写的精确字符串比较（无 libc strcmp 依赖也可内联）。
 * @return 1 表示相等；任一为 null 或长度/内容不同返回 0。
 */
static int devmgr_name_match(const char *a, const char *b)
{
    if ((a == 0) || (b == 0)) {
        return 0;
    }

    while ((*a != '\0') && (*b != '\0')) {
        if (*a != *b) {
            return 0;
        }
        ++a;
        ++b;
    }

    return (*a == '\0') && (*b == '\0');
}

/**
 * @brief switch-case 分支模板：若实例的 init 成员非 null 则调用。
 * @param type_enum   DRIVER_TYPE_* 枚举值。
 * @param struct_type 对应 interfaces 结构体类型。
 * @param init_member 结构体内 init 函数指针字段名。
 */
#define DEVMGR_INIT_CASE(type_enum, struct_type, init_member) \
    case type_enum: \
        if (((const struct_type *)entry->instance)->init_member != 0) { \
            ((const struct_type *)entry->instance)->init_member(); \
        } \
        break

/**
 * @brief 生成 devmgr_get_<name_field>(const char *name) 函数体。
 *
 * 扫描 [__driver_list_start, __driver_list_end)，匹配 type_enum 且 obj->name 与参数相等。
 */
#define DEVMGR_GET_IMPL(return_type, struct_type, type_enum, name_field) \
    const return_type *devmgr_get_##name_field(const char *name) \
    { \
        const driver_registry_entry_t *entry = __driver_list_start; \
        while (entry < __driver_list_end) { \
            if (entry->type == type_enum) { \
                const struct_type *obj = (const struct_type *)entry->instance; \
                if ((obj != 0) && devmgr_name_match(obj->name, name)) { \
                    return obj; \
                } \
            } \
            ++entry; \
        } \
        return 0; \
    }

/**
 * @brief 启动时一次性初始化：按条目类型调用对应驱动的 init()。
 *
 * instance 为 null 的条目跳过；init 为 null 的驱动不调用（允许纯查询型占位）。
 */
void devmgr_init_all(void)
{
    const driver_registry_entry_t *entry = __driver_list_start;

    while (entry < __driver_list_end) {
        if (entry->instance != 0) {
            switch (entry->type) {
            DEVMGR_INIT_CASE(DRIVER_TYPE_TEMP_HUM_SENSOR, temp_hum_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_DISPLAY, display_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_SERVO, servo_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_RELAY, relay_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_COMM, comm_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_MISC, misc_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_DISTANCE_SENSOR, distance_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_WEIGHT_SENSOR, weight_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_GAS_SENSOR, gas_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_ANALOG_PROBE, analog_probe_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_PRESSURE_SENSOR, pressure_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_IMU_SENSOR, imu_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_PULSE_OXIMETER, pulse_oximeter_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_BLOOD_PRESSURE, blood_pressure_sensor_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_RTC, rtc_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_INPUT, input_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_STEPPER, stepper_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_SEGMENT_DISPLAY, segment_display_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_RFID, rfid_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_RADIO, radio_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_GNSS, gnss_driver_t, init);
            default:
                /* 未知 type：忽略，便于渐进式扩展枚举 */
                break;
            }
        }
        ++entry;
    }
}

/* 以下由宏批量生成各类型 devmgr_get_* */
DEVMGR_GET_IMPL(temp_hum_sensor_t, temp_hum_sensor_t, DRIVER_TYPE_TEMP_HUM_SENSOR, temp_hum_sensor)
DEVMGR_GET_IMPL(display_driver_t, display_driver_t, DRIVER_TYPE_DISPLAY, display)
DEVMGR_GET_IMPL(servo_driver_t, servo_driver_t, DRIVER_TYPE_SERVO, servo)
DEVMGR_GET_IMPL(relay_driver_t, relay_driver_t, DRIVER_TYPE_RELAY, relay)
DEVMGR_GET_IMPL(comm_driver_t, comm_driver_t, DRIVER_TYPE_COMM, comm)
DEVMGR_GET_IMPL(misc_driver_t, misc_driver_t, DRIVER_TYPE_MISC, misc)
DEVMGR_GET_IMPL(distance_sensor_t, distance_sensor_t, DRIVER_TYPE_DISTANCE_SENSOR, distance_sensor)
DEVMGR_GET_IMPL(weight_sensor_t, weight_sensor_t, DRIVER_TYPE_WEIGHT_SENSOR, weight_sensor)
DEVMGR_GET_IMPL(gas_sensor_t, gas_sensor_t, DRIVER_TYPE_GAS_SENSOR, gas_sensor)
DEVMGR_GET_IMPL(analog_probe_t, analog_probe_t, DRIVER_TYPE_ANALOG_PROBE, analog_probe)
DEVMGR_GET_IMPL(pressure_sensor_t, pressure_sensor_t, DRIVER_TYPE_PRESSURE_SENSOR, pressure_sensor)
DEVMGR_GET_IMPL(imu_sensor_t, imu_sensor_t, DRIVER_TYPE_IMU_SENSOR, imu_sensor)
DEVMGR_GET_IMPL(pulse_oximeter_t, pulse_oximeter_t, DRIVER_TYPE_PULSE_OXIMETER, pulse_oximeter)
DEVMGR_GET_IMPL(blood_pressure_sensor_t, blood_pressure_sensor_t, DRIVER_TYPE_BLOOD_PRESSURE, blood_pressure)
DEVMGR_GET_IMPL(rtc_driver_t, rtc_driver_t, DRIVER_TYPE_RTC, rtc)
DEVMGR_GET_IMPL(input_driver_t, input_driver_t, DRIVER_TYPE_INPUT, input)
DEVMGR_GET_IMPL(stepper_driver_t, stepper_driver_t, DRIVER_TYPE_STEPPER, stepper)
DEVMGR_GET_IMPL(segment_display_t, segment_display_t, DRIVER_TYPE_SEGMENT_DISPLAY, segment_display)
DEVMGR_GET_IMPL(rfid_driver_t, rfid_driver_t, DRIVER_TYPE_RFID, rfid)

/**
 * @brief 无线驱动查找的特殊逻辑（非宏生成）。
 *
 * 1. 先 devmgr_get_comm(name)，若存在且 kind==COMM_KIND_PACKET，视为 radio（Wi-Fi 模块等）。
 * 2. 否则扫描 DRIVER_TYPE_RADIO 遗留注册项。
 */
const radio_driver_t *devmgr_get_radio(const char *name)
{
    const comm_driver_t *comm = devmgr_get_comm(name);

    if ((comm != 0) && (comm->kind == COMM_KIND_PACKET)) {
        return (const radio_driver_t *)comm;
    }

    {
        const driver_registry_entry_t *entry = __driver_list_start;
        while (entry < __driver_list_end) {
            if (entry->type == DRIVER_TYPE_RADIO) {
                const radio_driver_t *obj = (const radio_driver_t *)entry->instance;
                if ((obj != 0) && devmgr_name_match(obj->name, name)) {
                    return obj;
                }
            }
            ++entry;
        }
    }

    return 0;
}
DEVMGR_GET_IMPL(gnss_driver_t, gnss_driver_t, DRIVER_TYPE_GNSS, gnss)

unsigned short devmgr_count(void)
{
    return (unsigned short)(__driver_list_end - __driver_list_start);
}
