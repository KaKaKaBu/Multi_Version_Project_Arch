/**
 * @file irq_event.h
 * @brief 硬件中断源 → 调度器事件的桥接层（ISR 安全投递）。
 *
 * 问题：ISR 中不宜直接操作任务链表（event_set 会改 TASK_BLOCKED 状态）。
 * 做法：ISR 仅 irq_event_post_from_isr → event_set_from_isr 排队；
 *       主循环 sched_run_once 合并后再 event_set 唤醒协作任务。
 *
 * 用法：
 *   irq_event_init();
 *   irq_event_bind(IRQ_EVENT_SOURCE_USART1_RX, MY_USART1_RX_EVENT);
 *   在 USART1_IRQHandler 末尾： irq_event_post_from_isr(IRQ_EVENT_SOURCE_USART1_RX, 0);
 *   协作任务：task_block(MY_USART1_RX_EVENT); 被唤醒后 comm_port_recv() 等。
 */

#ifndef IRQ_EVENT_H
#define IRQ_EVENT_H

#include <stdint.h>
#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 板级已知的 IRQ 槽位；与具体 ISR 向量名在文档/板级代码中对应。 */
typedef enum irq_event_source {
    IRQ_EVENT_SOURCE_USART1_RX = 0,  ///< USART1 接收非空/空闲。
    IRQ_EVENT_SOURCE_USART2_RX,      ///< USART2 接收。
    IRQ_EVENT_SOURCE_USART3_RX,      ///< USART3 接收。
    IRQ_EVENT_SOURCE_TIM1_UPDATE,    ///< TIM1 更新（溢出）。
    IRQ_EVENT_SOURCE_TIM2_UPDATE,
    IRQ_EVENT_SOURCE_TIM3_UPDATE,
    IRQ_EVENT_SOURCE_TIM4_UPDATE,
    IRQ_EVENT_SOURCE_KEY_EXTI,       ///< EXTI 线路（键盘等）产生的唤醒。
    IRQ_EVENT_SOURCE_COUNT           ///< 哨兵：非法 source 上界，不可 bind。
} irq_event_source_t;

void irq_event_init(void);

/**
 * @brief 建立 source → sched_event_t 的一对一映射。
 * @return 0 成功；-1 表示 source 越界。
 */
int irq_event_bind(irq_event_source_t source, sched_event_t event);

/**
 * @brief ISR 内调用：累加计数、保存 value、若已 bind 则 event_set_from_isr。
 * @param value 应用自定义载荷（如 DMA 剩余长度），任务侧用 irq_event_get_last_value 读取。
 */
void irq_event_post_from_isr(irq_event_source_t source, uint32_t value);

uint32_t irq_event_get_count(irq_event_source_t source);
uint32_t irq_event_get_last_value(irq_event_source_t source);
void irq_event_clear(irq_event_source_t source);

#ifdef __cplusplus
}
#endif

#endif
