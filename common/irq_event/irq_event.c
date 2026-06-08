/**
 * @file irq_event.c
 * @brief IRQ 源到调度器事件的映射表与 ISR 安全投递实现。
 */

#include "irq_event.h"
#include "debug_log.h"

/** 各 IRQ 槽位绑定的调度事件掩码；0 表示未绑定，post 时不投递。 */
static sched_event_t irq_event_map[IRQ_EVENT_SOURCE_COUNT];
/** 自 init/clear 以来该源 post_from_isr 调用次数（调试用/统计）。 */
static volatile uint32_t irq_event_count[IRQ_EVENT_SOURCE_COUNT];
/** 最近一次 post 传入的 value 参数。 */
static volatile uint32_t irq_event_last_value[IRQ_EVENT_SOURCE_COUNT];

void irq_event_init(void)
{
    uint32_t i;

    for (i = 0; i < (uint32_t)IRQ_EVENT_SOURCE_COUNT; ++i) {
        irq_event_map[i] = 0U;
        irq_event_count[i] = 0U;
        irq_event_last_value[i] = 0U;
    }
}

int irq_event_bind(irq_event_source_t source, sched_event_t event)
{
    if ((uint32_t)source >= (uint32_t)IRQ_EVENT_SOURCE_COUNT) {
        return -1;
    }

    irq_event_map[source] = event;
    DEBUG_LOG_IRQ_EVENT("[IRQ_EVENT] bind source=%u event=0x%08lX\r\n",
                        (unsigned int)source,
                        (unsigned long)event);
    return 0;
}

void irq_event_post_from_isr(irq_event_source_t source, uint32_t value)
{
    sched_event_t event;

    if ((uint32_t)source >= (uint32_t)IRQ_EVENT_SOURCE_COUNT) {
        return;
    }

    irq_event_last_value[source] = value;
    ++irq_event_count[source];

    event = irq_event_map[source];
    if (event != 0U) {
        /* 延迟到 sched_run_once：避免 ISR 内遍历任务链表 */
        event_set_from_isr(event);
    }
}

uint32_t irq_event_get_count(irq_event_source_t source)
{
    if ((uint32_t)source >= (uint32_t)IRQ_EVENT_SOURCE_COUNT) {
        return 0U;
    }

    return irq_event_count[source];
}

uint32_t irq_event_get_last_value(irq_event_source_t source)
{
    if ((uint32_t)source >= (uint32_t)IRQ_EVENT_SOURCE_COUNT) {
        return 0U;
    }

    return irq_event_last_value[source];
}

void irq_event_clear(irq_event_source_t source)
{
    if ((uint32_t)source >= (uint32_t)IRQ_EVENT_SOURCE_COUNT) {
        return;
    }

    irq_event_count[source] = 0U;
}
