#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t sched_event_t;

#define SCHED_EVENT_TICK 0x00000001U

typedef enum task_state {
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SUSPENDED
} task_state_t;

struct driver_task;
typedef void (*task_entry_t)(struct driver_task *task);

typedef struct driver_task {
    const char *name;
    task_entry_t entry;
    uint8_t priority;
    task_state_t state;
    sched_event_t wait_events;
    sched_event_t pending_events;
    struct driver_task *next;
} driver_task_t;

void sched_init(void);
int sched_add_task(driver_task_t *task);
void sched_start(void);
void sched_run_once(void);
void task_block(sched_event_t events);
void task_suspend(driver_task_t *task);
void task_resume(driver_task_t *task);
void event_set(sched_event_t events);
sched_event_t task_events_get(driver_task_t *task);
void event_set_from_isr(sched_event_t events);
uint32_t sched_tick_get(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
