#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include "scheduler.h"
#include "version_config.h"

/* Operating modes */
typedef enum {
    MODE_AUTO = 0,      /* Automatic control based on thresholds */
    MODE_MANUAL,        /* Manual device control */
    MODE_THRESHOLD      /* Threshold setting mode */
} system_mode_t;

/* Manual mode device selection */
typedef enum {
    DEVICE_FAN = 0,
    DEVICE_BUZZER,
    DEVICE_LIGHT,
    DEVICE_COUNT
} device_type_t;

/* Threshold selection for threshold mode */
typedef enum {
    THRESH_PM25 = 0,
    THRESH_TEMP,
    THRESH_HUMIDITY,
    THRESH_SMOKE,
    THRESH_CO,
    THRESH_COUNT
} threshold_type_t;

/* Application context structure */
typedef struct {
    /* System mode */
    system_mode_t mode;
    uint8_t mode_changed;       /* Flag to trigger display refresh */

    /* Manual mode state */
    uint8_t manual_selected_device;

    /* Threshold mode state */
    uint8_t thresh_selected_item;

    /* Sensor readings */
    uint16_t pm25_ppm;
    int8_t temperature;
    uint8_t humidity;
    uint16_t smoke_ppm;
    uint16_t co_ppm;
    uint8_t sensor_updated;     /* Flag to trigger display refresh */

    /* Thresholds (stored in flash/config) */
    uint16_t pm25_threshold;
    int8_t temp_threshold;
    uint8_t humidity_threshold;
    uint16_t smoke_threshold;
    uint16_t co_threshold;

    /* Device states */
    uint8_t fan_on;
    uint8_t buzzer_on;
    uint8_t light_on;
    uint8_t alarm_active;       /* Any threshold exceeded in auto mode */

    /* Display state */
    uint8_t display_refresh;
} app_context_t;

/* Global application context */
extern app_context_t g_app;

/* Application initialization */
void app_logic_init(void);

/* sched_loop callback functions */
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void display_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);

/* Key event handlers - called from key_loop */
void app_logic_on_key1_press(void);  /* Mode switch */
void app_logic_on_key2_press(void);  /* Select */
void app_logic_on_key3_press(void);  /* Add/On */
void app_logic_on_key4_press(void);  /* Sub/Off */

/* Communication handlers */
void app_logic_on_mqtt_rx(const char *json_data);
void app_logic_build_telemetry(char *buf, size_t bufsize);

#endif /* APP_LOGIC_H */
