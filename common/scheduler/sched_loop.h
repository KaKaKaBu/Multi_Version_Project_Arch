/**
 * @file sched_loop.h
 * @brief 框架级 Loop 封装：把“周期性轮询”和“纯事件驱动”统一成协作调度任务。
 *
 * sched_loop_t 内嵌 driver_task_t，注册后由调度器周期性或按事件调用 run 回调，
 * 回调结束后自动 task_block，形成典型的“运行 → 阻塞等待 → 再运行”协作模型。
 *
 * ## 两种工作模式（由 period_ms 区分）
 *
 * | period_ms | 行为 |
 * |-----------|------|
 * | > 0       | **周期模式**：除 wait_events 外还等待 SCHED_EVENT_TICK；仅当
 *               (sched_tick_get - last_run_tick) >= period_ms 时才调用 run，
 *               用于传感器轮询、状态机等固定间隔逻辑。 |
 * | == 0      | **事件模式**：仅在 wait_events 对应事件到达时唤醒并执行 run，
 *               不在 tick 上主动跑逻辑（仍会在每次唤醒时进入 entry，但可能不调用 run）。 |
 *
 * ## signal_events
 *
 * run 执行完毕后，若 signal_events 非 0，会 event_set(signal_events)，用于向
 * 其他任务广播“本 Loop 已完成某阶段”等协作信号。
 *
 * ## 静态定义示例
 *
 * @code
 * static sched_loop_t g_sensor_loop = SCHED_LOOP_DEF(
 *     "sensor", sensor_loop_fn, 10, 50, 0, SENSOR_DONE_EVENT);
 * // 50ms 周期，无额外 wait，完成后投递 SENSOR_DONE_EVENT
 * sched_loop_register(&g_sensor_loop);
 * @endcode
 */

#ifndef SCHED_LOOP_H
#define SCHED_LOOP_H

#include "scheduler.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loop 被调度器唤醒后执行的用户回调。
 *
 * @param[in] events 本次唤醒时通过 task_events_get 得到的待处理事件（可能含 TICK 等）。
 * @param[in] ctx    用户上下文，来自 sched_loop_t::ctx，可在注册前赋值。
 */
typedef void (*sched_loop_fn_t)(sched_event_t events, void *ctx);

/**
 * @brief 一个可注册的协作 Loop 实例。
 *
 * 内存布局上 task 必须是结构体最后一个成员（或与 task 偏移固定），
 * 以便 sched_loop_task_entry 通过 offsetof 从 driver_task_t* 反推 sched_loop_t*。
 */
typedef struct sched_loop {
    const char *name;              ///< 逻辑名称，注册时复制到内嵌 task.name。
    sched_loop_fn_t run;           ///< 用户轮询/事件处理函数；不可为 null。
    void *ctx;                     ///< 传给 run 的 opaque 指针（设备句柄、配置表等）。
    uint32_t period_ms;            ///< 周期模式：两次 run 之间最少间隔的 tick 数（通常 1 tick = 1ms）。
    sched_event_t wait_events;     ///< 阻塞时除 TICK 外还要等待的事件（事件模式时仅此掩码生效）。
    sched_event_t signal_events;   ///< run 成功返回后向系统广播的事件位（0 表示不广播）。
    uint8_t priority;              ///< 内嵌 driver_task 的调度优先级。
    driver_task_t task;            ///< 内嵌 TCB；须通过 sched_loop_register 加入调度器。
    uint32_t last_run_tick;        ///< 周期模式：上次真正调用 run 时的 sched_tick_get 快照。
} sched_loop_t;

/**
 * @brief 静态初始化 sched_loop_t 的字段（不含 task、last_run_tick，由 register 填充）。
 *
 * @param _name          任务名字符串。
 * @param _fn            run 回调。
 * @param _pri           调度优先级（越大越先运行）。
 * @param _period_ms     周期 tick 间隔；0 表示纯事件驱动。
 * @param _wait_events   阻塞等待掩码。
 * @param _signal_events run 完成后 event_set 的掩码。
 */
#define SCHED_LOOP_DEF(_name, _fn, _pri, _period_ms, _wait_events, _signal_events) \
    { \
        .name = (_name), \
        .run = (_fn), \
        .ctx = 0, \
        .period_ms = (_period_ms), \
        .wait_events = (_wait_events), \
        .signal_events = (_signal_events), \
        .priority = (_pri) \
    }

/**
 * @brief 框架初始化占位；当前无全局状态，保留供未来扩展（如 Loop 链表统计）。
 */
void sched_loop_init(void);

/**
 * @brief 根据 sched_loop_t 填好内嵌 driver_task，并 sched_add_task 注册到调度器。
 *
 * @param[in] loop 已用 SCHED_LOOP_DEF 或手动初始化 name、run、priority 等的实例。
 * @return 0 成功；-1 表示 loop、run 或 name 无效。
 */
int sched_loop_register(sched_loop_t *loop);

#ifdef __cplusplus
}
#endif

#endif
