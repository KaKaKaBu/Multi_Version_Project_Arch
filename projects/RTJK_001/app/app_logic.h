/* RTJK_001 应用业务入口和调度回调声明。 */
#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_logic_init(void);
void app_logic_loop(sched_event_t events);
void app_logic_on_key(unsigned char key_id);
void app_logic_on_sensor_poll(void);

#ifdef __cplusplus
}
#endif

#endif
