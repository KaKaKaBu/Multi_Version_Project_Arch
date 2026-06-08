/**
 * @file app_fsm.h
 * @brief 表驱动有限状态机：应用层界面、模式与工作流导航。
 *
 * 与调度器 FSM 无关：纯同步、在任务 entry 或按键回调中调用 app_fsm_dispatch。
 * 转移表存放在 Flash（const 数组），每行描述 (from, event) → to + 可选 action。
 *
 * 示例：
 * @code
 * static const app_fsm_transition_t g_trans[] = {
 *   { STATE_IDLE, EVT_BTN, STATE_RUN, on_start },
 * };
 * app_fsm_t fsm;
 * app_fsm_init(&fsm, STATE_IDLE, g_trans, ARRAY_SIZE(g_trans), &ctx);
 * app_fsm_dispatch(&fsm, EVT_BTN);
 * @endcode
 */

#ifndef APP_FSM_H
#define APP_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 事件投递结果：是否命中转移表。 */
typedef enum app_fsm_result {
    APP_FSM_RESULT_OK = 0,       ///< 找到 (state, event) 匹配行并已执行 action。
    APP_FSM_RESULT_IGNORED       ///< 无匹配行，state 不变。
} app_fsm_result_t;

/** @brief 转移表中的一行（Moore/Mealy 混合：可在转移时执行 action）。 */
typedef struct app_fsm_transition {
    unsigned short from;           ///< 源状态 ID（应用自定义枚举数值）。
    unsigned short event;          ///< 触发事件 ID。
    unsigned short to;             ///< 目标状态 ID。
    void (*action)(void *context); ///< 进入 to 之前调用；可为 null。
} app_fsm_transition_t;

/** @brief FSM 运行时上下文。 */
typedef struct app_fsm {
    unsigned short state;                    ///< 当前状态。
    const app_fsm_transition_t *transitions; ///< 转移表（生命周期须覆盖 fsm 使用期）。
    unsigned short transition_count;         ///< 表行数。
    void *context;                           ///< 传给 action 的用户指针。
} app_fsm_t;

void app_fsm_init(app_fsm_t *fsm, unsigned short initial_state,
                  const app_fsm_transition_t *transitions,
                  unsigned short transition_count, void *context);

/**
 * @brief 线性扫描转移表，应用**首个** (from==state && event) 匹配行。
 *
 * 若有多个相同 (from, event) 行，仅第一条生效；表设计时应保证唯一性。
 */
app_fsm_result_t app_fsm_dispatch(app_fsm_t *fsm, unsigned short event);

#ifdef __cplusplus
}
#endif

#endif
