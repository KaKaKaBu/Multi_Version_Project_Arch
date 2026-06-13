#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include "scheduler.h"
#include "version_config.h"

typedef enum dcld_mode {
    DCLD_MODE_AUTO = 0,
    DCLD_MODE_THRESHOLD
} dcld_mode_t;

typedef struct dcld_context {
    dcld_mode_t mode;
    uint16_t raw_distance_cm;
    uint16_t distance_cm;
    uint16_t threshold_cm;
    float temperature_c;
    uint8_t alarm_active;
    uint8_t alarm_output_on;
    uint8_t display_dirty;
    uint8_t telemetry_pending;
    uint8_t voice_level;
    uint32_t last_alarm_toggle_tick;
    uint32_t alarm_period_ms;
    uint32_t last_temp_sample_tick;
    uint32_t last_voice_tick;
} dcld_context_t;

extern dcld_context_t g_dcld;

void app_logic_init(void);
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void display_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);
void app_logic_on_key1_press(void);
void app_logic_on_key2_press(void);
void app_logic_on_key3_press(void);
void app_logic_on_mqtt_rx(const char *json_data);
void app_logic_request_telemetry(void);

#endif
