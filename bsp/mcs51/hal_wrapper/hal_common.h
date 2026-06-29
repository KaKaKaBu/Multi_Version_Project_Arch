#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>
#include "hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_DEFAULT_TIMEOUT_US 100000U

typedef uint8_t (*hal_poll_fn_t)(void *ctx);

void bsp_init(void);
void hal_delay_us(uint32_t us);
hal_status_t hal_wait_flag_us(hal_poll_fn_t poll, void *ctx, uint32_t timeout_us);
uint32_t hal_get_us(void);
uint32_t hal_irq_lock(void);
void hal_irq_unlock(uint32_t irq_state);

#ifdef __cplusplus
}
#endif

#endif
