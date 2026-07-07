#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "version_config.h"

#if defined(PLATFORM_MCS51)
#include "mcs51_memory.h"
typedef unsigned long sched_event_t;
#else
#include "scheduler.h"
#endif

typedef enum app_bmi_state {
    APP_BMI_UNKNOWN = 0,
    APP_BMI_LIGHT = 1,
    APP_BMI_NORMAL = 2,
    APP_BMI_HEAVY = 3
} app_bmi_state_t;

typedef struct app_context {
    unsigned long weight_g;
    signed long tare_g;
    unsigned short distance_cm;
    unsigned short height_cm;
    unsigned short bmi_x10;
    unsigned long threshold_g;
    app_bmi_state_t bmi_state;
    unsigned char fan_on;
    unsigned char alarm_on;
    unsigned char manual_mode;
    unsigned char locked;
    unsigned char voice_paused;
    unsigned char voice_ticks;
    unsigned char display_dirty;
} app_context_t;

#if defined(PLATFORM_MCS51)
extern MCS51_XDATA app_context_t g_ctx;
#else
extern app_context_t g_ctx;
#endif

void app_logic_init(void);
void app_logic_loop(sched_event_t events);
void app_logic_on_key(unsigned char key_id);
void app_logic_on_sensor_tick(void);

void sensor_loop_run(sched_event_t events, void *ctx);
void logic_loop_run(sched_event_t events, void *ctx);
void key_loop_run(sched_event_t events, void *ctx);

#endif
