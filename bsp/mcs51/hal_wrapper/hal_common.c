#include "hal_common.h"

#ifndef MCS51_FOSC_HZ
#define MCS51_FOSC_HZ 11059200UL
#endif

static volatile uint32_t hal_time_us;

void bsp_init(void)
{
    hal_time_us = 0U;
}

void hal_delay_us(uint32_t us)
{
    uint32_t i;
    volatile uint8_t spin;

    while (us > 0U) {
        for (i = 0U; i < (MCS51_FOSC_HZ / 12000000UL + 1UL); ++i) {
            spin = 0U;
            (void)spin;
        }
        --us;
        ++hal_time_us;
    }
}

uint32_t hal_get_us(void)
{
    return hal_time_us;
}

uint32_t hal_irq_lock(void)
{
    return 0U;
}

void hal_irq_unlock(uint32_t irq_state)
{
    (void)irq_state;
}

hal_status_t hal_wait_flag_us(hal_poll_fn_t poll, void *ctx, uint32_t timeout_us)
{
    uint32_t start;

    if (poll == 0) {
        return HAL_ERR_PARAM;
    }

    start = hal_get_us();
    while ((hal_get_us() - start) < timeout_us) {
        if (poll(ctx) != 0U) {
            return HAL_OK;
        }
        hal_delay_us(1U);
    }

    return HAL_ERR_TIMEOUT;
}
