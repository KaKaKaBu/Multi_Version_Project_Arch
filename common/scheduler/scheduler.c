/**
 * @file scheduler.c
 * @brief 协作式任务调度器实现，含 Cortex-M 临界区（PRIMASK）辅助。
 *
 * 全局数据：
 * - sched_task_list     所有任务的单向链表头（按 priority 降序插入）
 * - sched_current_task  sched_run_once 执行 entry 时指向当前任务，供 task_block 使用
 * - sched_tick_count    SysTick 中断递增，供 sched_tick_get / sched_loop 周期判断
 * - sched_isr_pending_events  ISR 中 event_set_from_isr 累积，主循环 sched_run_once 合并
 */

#include "scheduler.h"

#include <stdio.h>

#if !defined(PLATFORM_MCS51)
#include "stm32f10x.h"
#endif

/** 按优先级排序的任务链表头指针；空闲时 null。 */
static driver_task_t *sched_task_list;
/** 正在运行 entry 的任务；非运行期为 null。 */
static driver_task_t *sched_current_task;
/** SysTick_Handler 中递增；只读侧通过 sched_tick_get 访问。 */
static volatile uint32_t sched_tick_count;
/**
 * ISR 与主循环之间的“邮箱”：中断里只 OR 事件位，不直接调用 event_set，
 * 避免在 ISR 里遍历任务链表修改 state/pending_events。
 */
static volatile sched_event_t sched_isr_pending_events;

/* -------------------------------------------------------------------------- */
/* Cortex-M 临界区：通过 PRIMASK 屏蔽可配置优先级以下的 IRQ（含 SysTick）      */
/* -------------------------------------------------------------------------- */

/**
 * @brief 进入临界区：保存 PRIMASK 后 cpsid i 关中断。
 * @return 进入前的 PRIMASK，供 sched_irq_unlock 原样恢复（支持嵌套临界区语义）。
 */
static uint32_t sched_irq_lock(void)
{
#if defined(PLATFORM_MCS51)
    return 0U;
#else
    uint32_t primask;

    __asm volatile ("MRS %0, PRIMASK" : "=r" (primask) :: "memory");
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
#endif
}

/**
 * @brief 退出临界区：仅当进入前 IRQ 未被屏蔽（PRIMASK bit0==0）时才 cpsie i。
 *
 * 若进入前已在临界区（primask bit0==1），则不再开中断，避免破坏外层锁。
 */
static void sched_irq_unlock(uint32_t primask)
{
#if defined(PLATFORM_MCS51)
    (void)primask;
#else
    if ((primask & 1U) == 0U) {
        __asm volatile ("cpsie i" ::: "memory");
    }
#endif
}

/* -------------------------------------------------------------------------- */
/* 内部：就绪任务选择                                                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief 线性扫描任务链表，返回 priority 最大的 TASK_READY 任务。
 *
 * 链表虽按 priority 排序，但中间可能有 BLOCKED/SUSPENDED 节点，
 * 因此不能“只取第一个 READY”，必须遍历比较。
 *
 * @return 最高优先级就绪任务；若无 READY 任务则 null。
 */
static driver_task_t *sched_pick_ready_task(void)
{
    driver_task_t *task = sched_task_list;
    driver_task_t *best = 0;

    while (task != 0) {
        if (task->state == TASK_READY) {
            /* 首次命中或发现更高 priority 时更新 best */
            if ((best == 0) || (task->priority > best->priority)) {
                best = task;
            }
        }
        task = task->next;
    }

    return best;
}

/* -------------------------------------------------------------------------- */
/* 对外 API                                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief 初始化调度器：清空链表、当前任务、tick 与 ISR 事件邮箱。
 */
void sched_init(void)
{
    sched_task_list = 0;
    sched_current_task = 0;
    sched_tick_count = 0;
    sched_isr_pending_events = 0U;
}

/**
 * @brief 将任务插入优先级有序链表，并初始化任务字段为“就绪、无等待、无待处理事件”。
 */
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

    /*
     * 找到插入点：跳过所有 priority >= 新任务 priority 的节点，
     * 使链表中 priority 从高到低排列（同优先级时新任务排在同优先级的后面）。
     */
    while ((*cursor != 0) && ((*cursor)->priority >= task->priority)) {
        cursor = &((*cursor)->next);
    }

    task->next = *cursor;
    *cursor = task;

    return 0;
}

/**
 * @brief 永不返回的调度主循环；等价于 while(1) sched_run_once()。
 */
void sched_start(void)
{
    for (;;) {
        sched_run_once();
    }
}

/**
 * @brief 单次调度：合并 ISR 事件 → 选任务 → 运行 entry → 恢复/保持状态。
 */
void sched_run_once(void)
{
    uint32_t primask = sched_irq_lock();
    /* 原子地“取走”ISR 邮箱内容，防止合并过程中 ISR 再次写入丢失 */
    sched_event_t pending_events = sched_isr_pending_events;
    driver_task_t *task;

    sched_isr_pending_events = 0U;
    sched_irq_unlock(primask);

    /* 在主上下文统一投递，唤醒所有匹配的 BLOCKED 任务 */
    event_set(pending_events);

    task = sched_pick_ready_task();

    if (task == 0) {
        /* 无就绪任务：可能全部 BLOCKED/SUSPENDED，或尚未注册任务 */
        return;
    }

    sched_current_task = task;
    task->state = TASK_RUNNING;

    /* 协作式：entry 跑完才返回；内部应 task_block 或主动让出 */
    task->entry(task);

    /*
     * 若 entry 内调用了 task_block，state 已为 TASK_BLOCKED，不再改回 READY。
     * 若 entry 直接 return 且未 block，则视为本轮结束，重新进入就绪队列。
     */
    if (task->state == TASK_RUNNING) {
        task->state = TASK_READY;
    }

    sched_current_task = 0;
}

/**
 * @brief 当前任务自愿阻塞，直到 event_set 投递的事件与 wait_events 有交集。
 */
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

/**
 * @brief 广播事件：所有任务 pending_events |= events；
 *        阻塞中且 wait_events 与 events 有交集的任务被唤醒为 READY。
 */
void event_set(sched_event_t events)
{
    driver_task_t *task = sched_task_list;

    while (task != 0) {
        /* 累积待处理事件，任务下次 task_events_get 时可读到 */
        task->pending_events |= events;

        if ((task->state == TASK_BLOCKED) && ((task->wait_events & events) != 0U)) {
            task->wait_events = 0;
            task->state = TASK_READY;
        }
        task = task->next;
    }
}

/**
 * @brief 快照并清零 pending_events，避免同一事件被重复处理。
 */
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

/**
 * @brief ISR 安全的事件投递：只修改 sched_isr_pending_events，延迟合并。
 */
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

/**
 * @brief 板级 SysTick 中断应调用此函数（或链接到同名向量）。
 * 每 1 个 tick：全局计数 +1，并向所有任务排队 TICK 事件（在下次 sched_run_once 生效）。
 */
void SysTick_Handler(void)
{
    ++sched_tick_count;
    event_set_from_isr(SCHED_EVENT_TICK);
}
