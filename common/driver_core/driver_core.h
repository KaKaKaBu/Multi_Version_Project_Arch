#ifndef DRIVER_CORE_H
#define DRIVER_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum driver_type {
    DRIVER_TYPE_TEMP_HUM_SENSOR = 1,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_SERVO,
    DRIVER_TYPE_RELAY,
    DRIVER_TYPE_COMM,
    DRIVER_TYPE_MISC,
    DRIVER_TYPE_DISTANCE_SENSOR,
    DRIVER_TYPE_WEIGHT_SENSOR,
    DRIVER_TYPE_GAS_SENSOR,
    DRIVER_TYPE_ANALOG_PROBE,
    DRIVER_TYPE_PRESSURE_SENSOR,
    DRIVER_TYPE_IMU_SENSOR,
    DRIVER_TYPE_PULSE_OXIMETER,
    DRIVER_TYPE_RTC,
    DRIVER_TYPE_INPUT,
    DRIVER_TYPE_STEPPER,
    DRIVER_TYPE_SEGMENT_DISPLAY,
    DRIVER_TYPE_RFID,
    DRIVER_TYPE_RADIO,
    DRIVER_TYPE_GNSS
} driver_type_t;

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

typedef struct driver_registry_entry {
    driver_type_t type;
    const void *instance;
} driver_registry_entry_t;

#if defined(__GNUC__)
#define DRIVER_USED __attribute__((used))
#define DRIVER_SECTION __attribute__((section(".driver_list")))
#else
#define DRIVER_USED
#define DRIVER_SECTION
#endif

#define DRIVER_CONCAT_INNER(a, b) a##b
#define DRIVER_CONCAT(a, b) DRIVER_CONCAT_INNER(a, b)
#define REGISTER_DRIVER(type, instance) \
    static const driver_registry_entry_t DRIVER_CONCAT(__driver_entry_, instance) DRIVER_USED DRIVER_SECTION = { \
        DRIVER_TYPE_##type, \
        &(instance) \
    }

extern const driver_registry_entry_t __driver_list_start[];
extern const driver_registry_entry_t __driver_list_end[];

#ifdef __cplusplus
}
#endif

#endif
