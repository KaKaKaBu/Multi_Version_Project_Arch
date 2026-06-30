/**
 * @file driver_core.h
 * @brief Linker section 驱动注册表与 REGISTER_DRIVER 静态发现宏。
 *
 * ## 设计目的
 *
 * 裸机工程常有多类传感器/执行器驱动，若用中央 switch-case 注册表则每增驱动需改 devmgr。
 * 本模块利用 GCC section 属性，让每个驱动 .c 文件通过 REGISTER_DRIVER 自注册，
 * 链接脚本将 .driver_list 收集为连续数组，devmgr 仅遍历 __driver_list_start..end。
 *
 * ## 链接脚本要求
 *
 * 需在 LD 脚本中定义：
 *   __driver_list_start = .;
 *   KEEP(*(.driver_list))
 *   __driver_list_end = .;
 *
 * ## 驱动作者步骤
 *
 * 1. 实现 common/interfaces/xxx_if.h 中的结构体（填函数指针）。
 * 2. 在驱动源文件末尾：REGISTER_DRIVER(类型后缀, 静态实例名);
 *    例如 REGISTER_DRIVER(display, g_oled) → type 为 DRIVER_TYPE_DISPLAY。
 * 3. 应用层 devmgr_get_xxx("g_oled.name 字符串") 获取实例。
 */

#ifndef DRIVER_CORE_H
#define DRIVER_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief linker 注册表内驱动实例的类型判别枚举。
 *
 * 与 devmgr.c 中 DEVMGR_INIT_CASE / DEVMGR_GET_IMPL 的 case 一一对应；
 * 新增类型须同时扩展本枚举、devmgr 宏展开及对应 *_if.h。
 */
typedef enum driver_type {
    DRIVER_TYPE_TEMP_HUM_SENSOR = 1,  ///< 温湿度（sensor_if.h → temp_hum_sensor_t）。
    DRIVER_TYPE_DISPLAY,              ///< 显示（display_if.h）。
    DRIVER_TYPE_SERVO,                ///< 舵机（actuator_if.h）。
    DRIVER_TYPE_RELAY,                ///< 继电器（actuator_if.h）。
    DRIVER_TYPE_COMM,                 ///< 通信（comm_if.h）。
    DRIVER_TYPE_MISC,                 ///< 杂项 GPIO（misc_if.h）。
    DRIVER_TYPE_DISTANCE_SENSOR,      ///< 测距（distance_if.h）。
    DRIVER_TYPE_WEIGHT_SENSOR,        ///< 称重（weight_if.h）。
    DRIVER_TYPE_GAS_SENSOR,           ///< 气体（gas_if.h）。
    DRIVER_TYPE_ANALOG_PROBE,         ///< 模拟探针（analog_probe_if.h）。
    DRIVER_TYPE_PRESSURE_SENSOR,      ///< 气压（pressure_if.h）。
    DRIVER_TYPE_IMU_SENSOR,           ///< IMU（imu_if.h）。
    DRIVER_TYPE_PULSE_OXIMETER,       ///< 血氧（health_if.h）。
    DRIVER_TYPE_RTC,                  ///< RTC（rtc_if.h）。
    DRIVER_TYPE_INPUT,                ///< 按键（input_if.h）。
    DRIVER_TYPE_STEPPER,              ///< 步进电机（stepper_if.h）。
    DRIVER_TYPE_SEGMENT_DISPLAY,      ///< 数码管（segment_if.h）。
    DRIVER_TYPE_RFID,                 ///< RFID（rfid_if.h）。
    DRIVER_TYPE_RADIO,                ///< 遗留射频（radio_if.h，新代码用 COMM）。
    DRIVER_TYPE_GNSS,                 ///< GNSS（gnss_if.h）。
    DRIVER_TYPE_BLOOD_PRESSURE        ///< 血压（bp_if.h）。
} driver_type_t;

/** @brief 小写遗留别名，兼容旧驱动 REGISTER_DRIVER 写法。 */
#define DRIVER_TYPE_temp_hum_sensor DRIVER_TYPE_TEMP_HUM_SENSOR
#define DRIVER_TYPE_display DRIVER_TYPE_DISPLAY
#define DRIVER_TYPE_servo DRIVER_TYPE_SERVO
#define DRIVER_TYPE_relay DRIVER_TYPE_RELAY
#define DRIVER_TYPE_comm DRIVER_TYPE_COMM
#define DRIVER_TYPE_misc DRIVER_TYPE_MISC
#define DRIVER_TYPE_distance_sensor DRIVER_TYPE_DISTANCE_SENSOR
#define DRIVER_TYPE_weight_sensor DRIVER_TYPE_WEIGHT_SENSOR
#define DRIVER_TYPE_gas_sensor DRIVER_TYPE_GAS_SENSOR
#define DRIVER_TYPE_analog_probe DRIVER_TYPE_ANALOG_PROBE
#define DRIVER_TYPE_pressure_sensor DRIVER_TYPE_PRESSURE_SENSOR
#define DRIVER_TYPE_imu_sensor DRIVER_TYPE_IMU_SENSOR
#define DRIVER_TYPE_pulse_oximeter DRIVER_TYPE_PULSE_OXIMETER
#define DRIVER_TYPE_rtc DRIVER_TYPE_RTC
#define DRIVER_TYPE_input DRIVER_TYPE_INPUT
#define DRIVER_TYPE_stepper DRIVER_TYPE_STEPPER
#define DRIVER_TYPE_segment_display DRIVER_TYPE_SEGMENT_DISPLAY
#define DRIVER_TYPE_rfid DRIVER_TYPE_RFID
#define DRIVER_TYPE_radio DRIVER_TYPE_RADIO
#define DRIVER_TYPE_gnss DRIVER_TYPE_GNSS
#define DRIVER_TYPE_blood_pressure DRIVER_TYPE_BLOOD_PRESSURE

/** @brief 链接器收集的单条注册记录：类型 + 实例指针。 */
typedef struct driver_registry_entry {
    driver_type_t type;      ///< 供 devmgr switch 分发 init/get。
    const void *instance;    ///< 指向 const 驱动对象（如 display_driver_t）。
} driver_registry_entry_t;

/** @brief 板级设备实例：把项目 board_config 映射到通用驱动配置。 */
typedef struct board_device_entry {
    driver_type_t type;      ///< 与驱动 REGISTER_DRIVER 的类型一致。
    const char *name;        ///< 与驱动对象 name 精确匹配。
    const void *config;      ///< 指向驱动私有 config 结构；允许为 null。
} board_device_entry_t;

#if defined(DRIVER_REGISTRY_STATIC)
#define DRIVER_USED
#define DRIVER_SECTION
#elif defined(__GNUC__)
/** @brief 防止 LTO/--gc-sections 丢弃未被 C 代码直接引用的注册项。 */
#define DRIVER_USED __attribute__((used))
/** @brief 将静态常量放入 .driver_list，由链接脚本聚合成数组。 */
#define DRIVER_SECTION __attribute__((section(".driver_list")))
/** @brief 将板级设备实例放入 .board_device_list，由链接脚本聚合成数组。 */
#define BOARD_DEVICE_SECTION __attribute__((section(".board_device_list")))
#else
#define DRIVER_USED
#define DRIVER_SECTION
#define BOARD_DEVICE_SECTION
#endif

#define DRIVER_CONCAT_INNER(a, b) a##b
#define DRIVER_CONCAT(a, b) DRIVER_CONCAT_INNER(a, b)

/**
 * @brief 在编译单元内注册一个静态驱动实例到全局表。
 *
 * @param type  类型 token，展开为 DRIVER_TYPE_##type（如 display → DRIVER_TYPE_DISPLAY）。
 * @param instance 驱动结构体变量名（取地址 &(instance) 写入注册表）。
 *
 * @code
 * static const display_driver_t g_oled = { .name = "oled", .init = oled_init, ... };
 * REGISTER_DRIVER(display, g_oled);
 * @endcode
 */
#if defined(DRIVER_REGISTRY_STATIC)
#define REGISTER_DRIVER(type, instance) \
    typedef char DRIVER_CONCAT(__driver_registry_static_marker_, instance)
#define REGISTER_BOARD_DEVICE(type, device_name, config_ptr) \
    typedef char DRIVER_CONCAT(__board_device_static_marker_, __LINE__)
#else
#define REGISTER_DRIVER(type, instance) \
    static const driver_registry_entry_t DRIVER_CONCAT(__driver_entry_, instance) DRIVER_USED DRIVER_SECTION = { \
        DRIVER_TYPE_##type, \
        &instance \
    }
#define REGISTER_BOARD_DEVICE(type, device_name, config_ptr) \
    static const board_device_entry_t DRIVER_CONCAT(__board_device_entry_, __LINE__) DRIVER_USED BOARD_DEVICE_SECTION = { \
        DRIVER_TYPE_##type, \
        device_name, \
        config_ptr \
    }
#endif

/** @brief 链接脚本提供的注册表起始（含首元素）。 */
extern const driver_registry_entry_t __driver_list_start[];
/** @brief 注册表尾后首地址（半开区间 [start, end)）。 */
extern const driver_registry_entry_t __driver_list_end[];

/** @brief 返回当前平台驱动注册表首元素。 */
const driver_registry_entry_t *driver_registry_begin(void);
/** @brief 返回当前平台驱动注册表尾后元素。 */
const driver_registry_entry_t *driver_registry_end(void);

/** @brief 链接脚本提供的板级设备实例表起始（含首元素）。 */
extern const board_device_entry_t __board_device_list_start[];
/** @brief 板级设备实例表尾后首地址（半开区间 [start, end)）。 */
extern const board_device_entry_t __board_device_list_end[];

/** @brief 返回当前平台板级设备实例表首元素。 */
const board_device_entry_t *board_device_registry_begin(void);
/** @brief 返回当前平台板级设备实例表尾后元素。 */
const board_device_entry_t *board_device_registry_end(void);

#ifdef __cplusplus
}
#endif

#endif
