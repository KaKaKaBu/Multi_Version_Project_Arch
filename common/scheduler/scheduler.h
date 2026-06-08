/**
 * @file scheduler.h
 * @brief 协作式（非抢占）任务调度器：按优先级选取就绪任务，支持事件阻塞与唤醒。
 *
 * ## 设计概要
 *
 * - **协作式调度**：任务在 entry() 中主动让出 CPU（通过 task_block / 函数返回），
 *   调度器不会在任务执行中途打断它。适合裸机、无 RTOS 的驱动与应用分层。
 * - **单链表 + 优先级**：所有任务挂在 sched_task_list 上，按 priority 降序插入；
 *   每次 sched_run_once() 遍历链表，选出 state==TASK_READY 且 priority 最高的任务运行。
 * - **事件驱动同步**：任务可 task_block(wait_mask)，由 event_set() 或
 *   event_set_from_isr() 投递事件后变为 TASK_READY。pending_events 累积未读事件位。
 *
 * ## 典型用法
 *
 * 1. sched_init()
 * 2. 为每个 driver_task_t 填写 name、entry、priority，sched_add_task()
 * 3. 配置 SysTick，在中断里由 SysTick_Handler 递增节拍并投递 SCHED_EVENT_TICK
 * 4. 主函数调用 sched_start()（死循环内 sched_run_once），或自行循环 sched_run_once()
 *
 * ## 扩展事件
 *
 * 除 SCHED_EVENT_TICK 外，可在工程内自定义 sched_event_t 位（如传感器就绪、按键等），
 * 通过 event_set / event_set_from_isr 与 task_block 组合使用。
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 调度器事件位掩码；每一位表示一类可等待/投递的同步事件。 */
typedef uint32_t sched_event_t;

/** @brief SysTick 周期性节拍事件（通常 1ms 一次，具体由板级 SysTick 配置决定）。 */
#define SCHED_EVENT_TICK 0x00000001U

/**
 * @brief 协作任务在调度器眼中的生命周期状态。
 *
 * 状态转换（简化）：
 *   TASK_READY --sched_run_once 选中--> TASK_RUNNING
 *   TASK_RUNNING --entry 返回且未 block--> TASK_READY
 *   TASK_RUNNING --task_block--> TASK_BLOCKED
 *   TASK_BLOCKED --event_set 匹配 wait_events--> TASK_READY
 *   任意 --task_suspend--> TASK_SUSPENDED
 *   TASK_SUSPENDED --task_resume--> TASK_READY
 */
typedef enum task_state {
    TASK_RUNNING = 0,   ///< 正在被 sched_run_once 执行 entry 回调。
    TASK_READY,         ///< 就绪，可被 sched_pick_ready_task 选中。
    TASK_BLOCKED,       ///< 已调用 task_block，等待 wait_events 中某位被置位。
    TASK_SUSPENDED      ///< 挂起，不参与调度直至 task_resume。
} task_state_t;

/** @brief 前向声明；完整结构体定义见 driver_task_t。 */
struct driver_task;

/**
 * @brief 任务入口函数类型。
 *
 * 每次被调度时调用一次；函数末尾应 task_block() 或自然返回。
 * 若返回时仍为 TASK_RUNNING，调度器会自动改回 TASK_READY（便于“跑完即让出”的写法）。
 *
 * @param[in] task 当前任务控制块，通常可用来取内嵌父结构体（见 sched_loop 的 offsetof 用法）。
 */
typedef void (*task_entry_t)(struct driver_task *task);

/**
 * @brief 调度器任务控制块（TCB），作为单向链表节点挂在 sched_task_list 上。
 *
 * 用户侧常见两种用法：
 * 1. 静态定义 driver_task_t，填写字段后 sched_add_task；
 * 2. 将 driver_task_t 内嵌在更大结构体中（如 sched_loop_t），通过 container_of 取回上下文。
 */
typedef struct driver_task {
    const char *name;              ///< 任务名，仅用于调试/日志，可为调试字符串字面量。
    task_entry_t entry;            ///< 调度时调用的入口；sched_add_task 要求非 null。
    uint8_t priority;              ///< 优先级，数值越大越优先（0~255，由应用约定）。
    task_state_t state;            ///< 当前生命周期状态，由调度器与 API 维护。
    sched_event_t wait_events;     ///< TASK_BLOCKED 时：等待这些位中任意一位出现在 pending 或本次投递中。
    sched_event_t pending_events;  ///< 已投递但尚未被 task_events_get 消费的事件累积（按位 OR）。
    struct driver_task *next;      ///< 链表后继；链表按 priority 从高到低排列，便于遍历选最高就绪任务。
} driver_task_t;

/** @brief 清空调度器全局状态（任务链表、当前任务指针、tick、ISR 事件队列）。 */
void sched_init(void);

/**
 * @brief 将任务按优先级插入调度链表，并初始化为 TASK_READY。
 *
 * 插入规则：遍历链表，找到第一个 priority < 新任务 priority 的位置插入，
 * 保证从头到尾 priority 非递增（即高优先级靠前）。同优先级时后注册的任务排在后面。
 *
 * @param[in] task 任务描述符；entry 必须非 null。
 * @return 0 成功；-1 表示 task 或 entry 为 null。
 */
int sched_add_task(driver_task_t *task);

/**
 * @brief 启动调度主循环；内部为 for(;;) { sched_run_once(); }，正常不会返回。
 *
 * 调用前须已完成 sched_init、注册任务，并配置好 SysTick 等节拍源。
 */
void sched_start(void);

/**
 * @brief 执行**一次**调度迭代（适合在主循环或裸机 super-loop 中手动调用）。
 *
 * 步骤概要：
 * 1. 临界区内取出并清空 sched_isr_pending_events，再 event_set 合并到各任务；
 * 2. 选取优先级最高的 TASK_READY 任务；
 * 3. 置 TASK_RUNNING，调用 entry；若返回后仍为 RUNNING 则改回 READY；
 * 4. 清空 sched_current_task。
 */
void sched_run_once(void);

/**
 * @brief 阻塞**当前正在运行**的任务（须在 entry 回调上下文中调用）。
 *
 * 设置 sched_current_task 的 wait_events 与 state=TASK_BLOCKED。
 * 下次 event_set 若 (wait_events & events)!=0，该任务变为 TASK_READY。
 * 若 sched_current_task 为 null（非任务上下文），本调用无效果。
 *
 * @param[in] events 等待的事件位掩码（任意一位匹配即可唤醒）。
 */
void task_block(sched_event_t events);

/**
 * @brief 挂起指定任务，之后 sched_pick_ready_task 不会选中它。
 * @param[in] task 目标任务；null 时忽略。
 */
void task_suspend(driver_task_t *task);

/**
 * @brief 将 TASK_SUSPENDED 的任务恢复为 TASK_READY。
 * @param[in] task 目标任务；null 或状态不是 SUSPENDED 时忽略。
 */
void task_resume(driver_task_t *task);

/**
 * @brief 向**所有**已注册任务投递事件（按位 OR 进 pending_events）。
 *
 * 对 TASK_BLOCKED 任务：若 (wait_events & events) 非 0，则清空 wait_events 并置 TASK_READY。
 * 可在任务上下文或 sched_run_once 合并 ISR 事件时调用；与中断并发时由调用方保证一致性
 * （ISR 侧应使用 event_set_from_isr，由 sched_run_once 统一合并）。
 *
 * @param[in] events 要投递的事件位掩码。
 */
void event_set(sched_event_t events);

/**
 * @brief 读取并**清空**指定任务的 pending_events（消费模型，读后即丢）。
 *
 * 任务在 entry 开头常用此函数获取“本次为何被唤醒”的事件集合。
 *
 * @param[in] task 目标任务；null 返回 0。
 * @return 调用前的 pending_events 快照。
 */
sched_event_t task_events_get(driver_task_t *task);

/**
 * @brief 在中断服务程序中投递事件（仅写入 ISR 侧队列，不直接改任务链表）。
 *
 * 使用 sched_irq_lock 保护对 sched_isr_pending_events 的 OR 操作；
 * 真正的 event_set 在下次 sched_run_once 开头于主循环/任务上下文执行，避免 ISR 与任务状态竞态。
 *
 * @param[in] events 要排队的事件位掩码。
 */
void event_set_from_isr(sched_event_t events);

/**
 * @brief 获取自调度器启动以来 SysTick_Handler 递增的全局节拍计数。
 *
 * 注意：与 wall-clock 的换算依赖 SysTick 重装值（板级配置），sched_loop 的 period_ms
 * 在本实现中与 tick 数值按 1:1 比较，通常约定 1 tick = 1ms。
 *
 * @return sched_tick_count 的当前值（volatile 读）。
 */
uint32_t sched_tick_get(void);

/**
 * @brief Cortex-M SysTick 中断服务例程（弱符号可由板级覆盖）。
 *
 * 每节拍：sched_tick_count++，并通过 event_set_from_isr 投递 SCHED_EVENT_TICK。
 */
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
