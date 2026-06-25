#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include "scheduler.h"
#include "version_config.h"

#if VERSION_FEATURE_GPS
#include "gnss_if.h"
#endif

typedef enum zhjg_mode {
    ZHJG_MODE_AUTO = 0,
    ZHJG_MODE_THRESHOLD
} zhjg_mode_t;

typedef enum zhjg_threshold_item {
    ZHJG_THRESHOLD_METHANE = 0,
    ZHJG_THRESHOLD_WATER,
    ZHJG_THRESHOLD_TILT
} zhjg_threshold_item_t;

typedef struct zhjg_context {
    zhjg_mode_t mode;
    zhjg_threshold_item_t selected_threshold;
    uint16_t methane_ppm;
    float water_level_percent;
    float tilt_degree;
    uint16_t methane_threshold_ppm;
    float water_threshold_percent;
    float tilt_threshold_degree;
    uint8_t methane_alarm;
    uint8_t water_alarm;
    uint8_t tilt_alarm;
    uint8_t alarm_active;
    uint8_t alarm_output_on;
    uint8_t display_dirty;
    uint8_t telemetry_pending;
    uint32_t last_alarm_toggle_tick;
    uint32_t last_telemetry_tick;
#if VERSION_FEATURE_GPS
    gnss_fix_t gps_fix;
#endif
#if VERSION_FEATURE_SMS
    uint32_t last_sms_tick;
    uint8_t sms_sent_for_alarm;
#endif
} zhjg_context_t;

extern zhjg_context_t g_zhjg;

void app_logic_init(void);
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void display_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);
void app_logic_on_key1_press(void);
void app_logic_on_key2_press(void);
void app_logic_on_key3_press(void);
void app_logic_on_key4_press(void);
void app_logic_on_mqtt_rx(const char *json_data);
void app_logic_request_telemetry(void);

#endif
