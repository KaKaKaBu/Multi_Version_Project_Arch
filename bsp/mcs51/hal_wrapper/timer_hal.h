#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include <stdint.h>
#include "hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

void timer_hal_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif
