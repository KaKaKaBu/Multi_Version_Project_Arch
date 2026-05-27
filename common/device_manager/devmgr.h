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

void devmgr_init_all(void);
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
const rtc_driver_t *devmgr_get_rtc(const char *name);
const input_driver_t *devmgr_get_input(const char *name);
const stepper_driver_t *devmgr_get_stepper(const char *name);
const segment_display_t *devmgr_get_segment_display(const char *name);
const rfid_driver_t *devmgr_get_rfid(const char *name);
const radio_driver_t *devmgr_get_radio(const char *name);
const gnss_driver_t *devmgr_get_gnss(const char *name);
unsigned short devmgr_count(void);

#ifdef __cplusplus
}
#endif

#endif
