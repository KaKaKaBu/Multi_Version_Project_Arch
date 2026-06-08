#include "sched_loop.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "scheduler.h"

static sched_event_t sched_loop_effective_wait(const sched_loop_t *loop)
{
    sched_event_t mask = loop->wait_events;
    if (loop->period_ms > 0U) {
        mask |= SCHED_EVENT_TICK;
    }
    if (mask == 0U) {
        mask = SCHED_EVENT_TICK;
    }
    return mask;
}

static sched_loop_t *sched_loop_from_task(driver_task_t *task)
{
    uintptr_t base = (uintptr_t)task - offsetof(sched_loop_t, task);
    return (sched_loop_t *)base;
}

static bool sched_loop_wait_condition_met(const sched_loop_t *loop, sched_event_t events)
{
    if (loop->wait_events == 0U) {
        /* 对于纯周期 loop，事件条件留给 period/tick 判断 */
        return true;
    }

    return (events & loop->wait_events) != 0U;
}

static void sched_loop_task_entry(driver_task_t *task)
{
    sched_loop_t *loop = sched_loop_from_task(task);
    sched_event_t events = task_events_get(task);
    bool should_run = true;

    if (loop->period_ms > 0U) {
        uint32_t now = sched_tick_get();

        if ((events & SCHED_EVENT_TICK) == 0U) {
            should_run = false;
        }

        if (!sched_loop_wait_condition_met(loop, events)) {
            should_run = false;
        }

        if (((uint32_t)(now - loop->last_run_tick)) < loop->period_ms) {
            should_run = false;
        } else if (should_run) {
            loop->last_run_tick = now;
        }
    } else {
        should_run = sched_loop_wait_condition_met(loop, events);
    }

    if (should_run && (loop->run != 0)) {
        loop->run(events, loop->ctx);
        if (loop->signal_events != 0U) {
            event_set(loop->signal_events);
        }
    }

    task_block(sched_loop_effective_wait(loop));
}

void sched_loop_init(void)
{
    /* 保留占位：当前实现无需全局状态 */
}

int sched_loop_register(sched_loop_t *loop)
{
    if ((loop == 0) || (loop->name == 0) || (loop->run == 0)) {
        return -1;
    }

    loop->task.name = loop->name;
    loop->task.entry = sched_loop_task_entry;
    loop->task.priority = loop->priority;
    loop->task.state = TASK_READY;
    loop->task.wait_events = 0;
    loop->task.pending_events = 0;
    loop->task.next = 0;

    if (loop->period_ms > 0U) {
        loop->last_run_tick = sched_tick_get() - loop->period_ms;
    } else {
        loop->last_run_tick = 0U;
    }

    /* 第一次运行后立即按照配置的事件重新阻塞 */
    return sched_add_task(&loop->task);
}
