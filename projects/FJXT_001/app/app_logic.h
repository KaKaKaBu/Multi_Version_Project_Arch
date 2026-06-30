/* FJXT_001 应用状态、控制命令和调度回调声明。 */
#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include "version_config.h"
#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_EVENT_SENSOR 0x00000010UL
#define APP_EVENT_KEY 0x00000020UL
#define APP_EVENT_MOTOR 0x00000040UL
#define APP_EVENT_ALARM 0x00000080UL
#define APP_EVENT_COMM_RX 0x00000100UL
#define APP_EVENT_COMM_TX 0x00000200UL

typedef enum fjxt_window_state {
    FJXT_STATE_STOPPED = 0,
    FJXT_STATE_OPENING,
    FJXT_STATE_CLOSING,
    FJXT_STATE_OPEN_DONE,
    FJXT_STATE_CLOSE_DONE,
    FJXT_STATE_PINCH_REVERSING
} fjxt_window_state_t;

typedef enum fjxt_command {
    FJXT_CMD_NONE = 0,
    FJXT_CMD_OPEN_FULL,
    FJXT_CMD_CLOSE_FULL,
    FJXT_CMD_OPEN_NUDGE,
    FJXT_CMD_CLOSE_NUDGE,
    FJXT_CMD_STOP
} fjxt_command_t;

typedef struct fjxt_context {
    fjxt_window_state_t state;
    fjxt_command_t pending_cmd;
    uint8_t open_limit;
    uint8_t close_limit;
    uint8_t pinch_detected;
    uint8_t alarm_active;
    uint8_t alarm_output_on;
    uint8_t display_dirty;
    uint16_t nudge_remaining_deg;
    uint16_t reverse_remaining_deg;
    uint32_t last_alarm_tick;
    uint32_t alarm_stop_tick;
    uint32_t last_telemetry_tick;
    uint8_t telemetry_pending;
#if VERSION_FEATURE_CAMERA
    char camera_ip[32];
    char camera_stream[96];
#endif
} fjxt_context_t;

extern fjxt_context_t g_fjxt;

void app_logic_init(void);
void app_logic_handle_key(uint8_t key_index);
void app_logic_on_remote_rx(const char *json_data);
void app_logic_request_telemetry(void);

void sensor_loop_run(sched_event_t events, void *ctx);
void motor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void display_loop_run(sched_event_t events, void *ctx);
void comm_loop_run(sched_event_t events, void *ctx);

#ifdef __cplusplus
}
#endif

#endif
