#include "devmgr.h"
#include "driver_core.h"

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

#define DEVMGR_INIT_CASE(type_enum, struct_type, init_member) \
    case type_enum: \
        if (((const struct_type *)entry->instance)->init_member != 0) { \
            ((const struct_type *)entry->instance)->init_member(); \
        } \
        break

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
            DEVMGR_INIT_CASE(DRIVER_TYPE_RTC, rtc_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_INPUT, input_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_STEPPER, stepper_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_SEGMENT_DISPLAY, segment_display_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_RFID, rfid_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_RADIO, radio_driver_t, init);
            DEVMGR_INIT_CASE(DRIVER_TYPE_GNSS, gnss_driver_t, init);
            default:
                break;
            }
        }
        ++entry;
    }
}

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
DEVMGR_GET_IMPL(rtc_driver_t, rtc_driver_t, DRIVER_TYPE_RTC, rtc)
DEVMGR_GET_IMPL(input_driver_t, input_driver_t, DRIVER_TYPE_INPUT, input)
DEVMGR_GET_IMPL(stepper_driver_t, stepper_driver_t, DRIVER_TYPE_STEPPER, stepper)
DEVMGR_GET_IMPL(segment_display_t, segment_display_t, DRIVER_TYPE_SEGMENT_DISPLAY, segment_display)
DEVMGR_GET_IMPL(rfid_driver_t, rfid_driver_t, DRIVER_TYPE_RFID, rfid)
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
