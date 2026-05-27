#ifndef IRQ_EVENT_H
#define IRQ_EVENT_H

#include <stdint.h>
#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum irq_event_source {
    IRQ_EVENT_SOURCE_USART1_RX = 0,
    IRQ_EVENT_SOURCE_USART2_RX,
    IRQ_EVENT_SOURCE_USART3_RX,
    IRQ_EVENT_SOURCE_TIM1_UPDATE,
    IRQ_EVENT_SOURCE_TIM2_UPDATE,
    IRQ_EVENT_SOURCE_TIM3_UPDATE,
    IRQ_EVENT_SOURCE_TIM4_UPDATE,
    IRQ_EVENT_SOURCE_COUNT
} irq_event_source_t;

void irq_event_init(void);
int irq_event_bind(irq_event_source_t source, sched_event_t event);
void irq_event_post_from_isr(irq_event_source_t source, uint32_t value);
uint32_t irq_event_get_count(irq_event_source_t source);
uint32_t irq_event_get_last_value(irq_event_source_t source);
void irq_event_clear(irq_event_source_t source);

#ifdef __cplusplus
}
#endif

#endif
