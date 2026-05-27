#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_logic_init(void);
void app_logic_loop(sched_event_t events);
void app_logic_on_key(unsigned char key_id);
void app_logic_on_second_tick(void);

#ifdef __cplusplus
}
#endif

#endif
