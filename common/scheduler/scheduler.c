#include "scheduler.h"

#include <stdio.h>

static driver_task_t *sched_task_list;
static driver_task_t *sched_current_task;
static volatile uint32_t sched_tick_count;
static volatile sched_event_t sched_isr_pending_events;

static uint32_t sched_irq_lock(void)
{
    uint32_t primask;

    __asm volatile ("MRS %0, PRIMASK" : "=r" (primask) :: "memory");
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
}

static void sched_irq_unlock(uint32_t primask)
{
    if ((primask & 1U) == 0U) {
        __asm volatile ("cpsie i" ::: "memory");
    }
}

static driver_task_t *sched_pick_ready_task(void)
{
    driver_task_t *task = sched_task_list;
    driver_task_t *best = 0;

    while (task != 0) {
        if (task->state == TASK_READY) {
            if ((best == 0) || (task->priority > best->priority)) {
                best = task;
            }
        }
        task = task->next;
    }

    return best;
}

void sched_init(void)
{
    sched_task_list = 0;
    sched_current_task = 0;
    sched_tick_count = 0;
    sched_isr_pending_events = 0U;
}

int sched_add_task(driver_task_t *task)
{
    driver_task_t **cursor = &sched_task_list;

    if ((task == 0) || (task->entry == 0)) {
        return -1;
    }

    task->state = TASK_READY;
    task->wait_events = 0;
    task->pending_events = 0;
    task->next = 0;

    while ((*cursor != 0) && ((*cursor)->priority >= task->priority)) {
        cursor = &((*cursor)->next);
    }

    task->next = *cursor;
    *cursor = task;

    return 0;
}

void sched_start(void)
{
    for (;;) {
        sched_run_once();
    }
}

void sched_run_once(void)
{
    uint32_t primask = sched_irq_lock();
    sched_event_t pending_events = sched_isr_pending_events;
    driver_task_t *task;

    sched_isr_pending_events = 0U;
    sched_irq_unlock(primask);

    event_set(pending_events);

    task = sched_pick_ready_task();

    if (task == 0) {
        return;
    }

    sched_current_task = task;
    task->state = TASK_RUNNING;
    task->entry(task);

    if (task->state == TASK_RUNNING) {
        task->state = TASK_READY;
    }

    sched_current_task = 0;
}

void task_block(sched_event_t events)
{
    if (sched_current_task != 0) {
        sched_current_task->wait_events = events;
        sched_current_task->state = TASK_BLOCKED;
    }
}

void task_suspend(driver_task_t *task)
{
    if (task != 0) {
        task->state = TASK_SUSPENDED;
    }
}

void task_resume(driver_task_t *task)
{
    if ((task != 0) && (task->state == TASK_SUSPENDED)) {
        task->state = TASK_READY;
    }
}

void event_set(sched_event_t events)
{
    driver_task_t *task = sched_task_list;

    while (task != 0) {
        task->pending_events |= events;
        if ((task->state == TASK_BLOCKED) && ((task->wait_events & events) != 0U)) {
            task->wait_events = 0;
            task->state = TASK_READY;
        }
        task = task->next;
    }
}

sched_event_t task_events_get(driver_task_t *task)
{
    sched_event_t events;

    if (task == 0) {
        return 0U;
    }

    events = task->pending_events;
    task->pending_events = 0U;
    return events;
}

void event_set_from_isr(sched_event_t events)
{
    uint32_t primask = sched_irq_lock();

    sched_isr_pending_events |= events;
    sched_irq_unlock(primask);
}

uint32_t sched_tick_get(void)
{
    return sched_tick_count;
}

void SysTick_Handler(void)
{
    ++sched_tick_count;
    event_set_from_isr(SCHED_EVENT_TICK);
}
