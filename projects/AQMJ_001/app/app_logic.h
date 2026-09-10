#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include "rtc_if.h"
#include "scheduler.h"

#define APP_EVENT_SENSOR  0x00000002U
#define APP_EVENT_KEY     0x00000004U
#define APP_EVENT_ALARM   0x00000008U
#define APP_EVENT_DISPLAY 0x00000010U
#define APP_EVENT_COMM_RX 0x00000020U
#define APP_EVENT_COMM_TX 0x00000040U
#define APP_EVENT_RTC     0x00000080U
#define APP_EVENT_IR      0x00000100U

typedef struct aqmj_state {
    rtc_time_t now;
    uint8_t owner_home;
    uint8_t armed;
    uint8_t presence;
    uint8_t last_presence;
    uint8_t doorbell_pressed;
    uint8_t alarm_active;
    uint8_t alarm_output_on;
    uint8_t lamp_on;
    uint8_t message_pending;
    uint8_t recording;
    uint8_t playing_message;
    uint8_t ir_alarm_latched;
    uint8_t video_enabled;
    uint8_t display_dirty;
    uint8_t telemetry_pending;
    uint8_t stay_seconds;
    uint16_t light_value;
    uint32_t presence_start_tick;
    uint32_t last_alarm_toggle_tick;
    uint32_t last_telemetry_tick;
    uint32_t record_stop_tick;
    uint32_t play_stop_tick;
    rtc_time_t last_call_time;
} aqmj_state_t;

void app_logic_init(void);
void app_logic_handle_key(uint8_t key_index);
void app_logic_on_remote_rx(const char *json_data);
void app_logic_request_telemetry(void);
void app_comm_poll(sched_event_t events);

void rtc_loop_run(sched_event_t events, void *ctx);
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void display_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);
void ir_loop_run(sched_event_t events, void *ctx);

#endif
