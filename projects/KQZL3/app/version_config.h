#ifndef VERSION_CONFIG_H
#define VERSION_CONFIG_H

#include "scheduler.h"

/* Version info - KQZL3_VERSION is defined via CMake */
#ifndef KQZL3_VERSION
#define KQZL3_VERSION 14
#endif

#define VERSION_NAME "KQZL3"

/* Application events */
#define APP_EVENT_TICK          SCHED_EVENT_TICK
#define APP_EVENT_KEY           0x01
#define APP_EVENT_SENSOR_READY  0x02
#define APP_EVENT_COMM_RX       0x04
#define APP_EVENT_ALARM_TRIGGER 0x08

#endif
