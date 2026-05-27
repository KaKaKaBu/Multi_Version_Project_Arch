#ifndef APP_FSM_H
#define APP_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum app_fsm_result {
    APP_FSM_RESULT_OK = 0,
    APP_FSM_RESULT_IGNORED
} app_fsm_result_t;

typedef struct app_fsm_transition {
    unsigned short from;
    unsigned short event;
    unsigned short to;
    void (*action)(void *context);
} app_fsm_transition_t;

typedef struct app_fsm {
    unsigned short state;
    const app_fsm_transition_t *transitions;
    unsigned short transition_count;
    void *context;
} app_fsm_t;

void app_fsm_init(app_fsm_t *fsm, unsigned short initial_state, const app_fsm_transition_t *transitions, unsigned short transition_count, void *context);
app_fsm_result_t app_fsm_dispatch(app_fsm_t *fsm, unsigned short event);

#ifdef __cplusplus
}
#endif

#endif
