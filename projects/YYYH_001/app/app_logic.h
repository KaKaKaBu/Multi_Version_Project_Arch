#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include "scheduler.h"
#include "rtc_if.h"
#include "version_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YYYH_TIMER_COUNT 3U
#define YYYH_MEDICINE_SLOT_COUNT 3U

typedef enum yyyh_mode {
    YYYH_MODE_NORMAL = 0,
    YYYH_MODE_TIMER_SET,
    YYYH_MODE_MEDICINE_SET,
    YYYH_MODE_MANUAL
} yyyh_mode_t;

typedef struct yyyh_timer {
    uint8_t enabled;
    uint8_t hour;
    uint8_t minute;
    uint8_t dose;
    uint8_t category;
    uint8_t fired_day;
    uint8_t fired_hour;
    uint8_t fired_minute;
} yyyh_timer_t;

typedef struct yyyh_context {
    yyyh_mode_t mode;
    rtc_time_t now;
    yyyh_timer_t timers[YYYH_TIMER_COUNT];
    uint8_t selected_timer;
    uint8_t selected_field;
    uint8_t medicine_counts[YYYH_MEDICINE_SLOT_COUNT];
    uint8_t selected_medicine;
    uint8_t alarm_active;
    uint8_t alarm_output_on;
    uint8_t medicine_taken;
    uint8_t box_open;
    float weight_g;
    uint8_t low_medicine;
    float temperature;
    float humidity;
    uint8_t display_dirty;
    uint8_t telemetry_pending;
    uint32_t last_alarm_toggle_tick;
    uint32_t last_sensor_tick;
    uint32_t last_telemetry_tick;
    uint32_t last_voice_tick;
} yyyh_context_t;

extern yyyh_context_t g_yyyh;

void app_logic_init(void);
void app_logic_handle_key(uint8_t key_index);
void app_logic_on_remote_rx(const char *json_data);
void app_logic_request_telemetry(void);

void rtc_loop_run(sched_event_t events, void *ctx);
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void display_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);

#ifdef __cplusplus
}
#endif

#endif

