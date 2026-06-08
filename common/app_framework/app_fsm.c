/**
 * @file app_fsm.c
 * @brief 表驱动应用有限状态机实现。
 */

#include "app_fsm.h"

void app_fsm_init(app_fsm_t *fsm, unsigned short initial_state,
                  const app_fsm_transition_t *transitions,
                  unsigned short transition_count, void *context)
{
    if (fsm == 0) {
        return;
    }

    fsm->state = initial_state;
    fsm->transitions = transitions;
    fsm->transition_count = transition_count;
    fsm->context = context;
}

app_fsm_result_t app_fsm_dispatch(app_fsm_t *fsm, unsigned short event)
{
    unsigned short i;

    if ((fsm == 0) || (fsm->transitions == 0)) {
        return APP_FSM_RESULT_IGNORED;
    }

    for (i = 0; i < fsm->transition_count; ++i) {
        const app_fsm_transition_t *transition = &fsm->transitions[i];

        if ((transition->from == fsm->state) && (transition->event == event)) {
            /* 先改状态再 action，便于 action 内嵌套 dispatch 时看到新 state */
            fsm->state = transition->to;
            if (transition->action != 0) {
                transition->action(fsm->context);
            }
            return APP_FSM_RESULT_OK;
        }
    }

    return APP_FSM_RESULT_IGNORED;
}
