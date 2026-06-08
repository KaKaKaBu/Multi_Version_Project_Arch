/**
 * @file hal_common.h
 * @brief Common BSP initialization, microsecond timing, and polling helpers.
 */

#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>
#include "hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Default HAL poll/transfer timeout in microseconds. */
#define HAL_DEFAULT_TIMEOUT_US 100000U

/** @brief Poll callback; returns non-zero when the waited condition is satisfied. */
typedef uint8_t (*hal_poll_fn_t)(void *ctx);

/**
 * @brief Initializes NVIC, SysTick, DWT, memory pool, cJSON port, and debug UART.
 */
void bsp_init(void);

/**
 * @brief Blocks for the specified number of microseconds using the DWT cycle counter.
 * @param us Delay duration in microseconds.
 */
void hal_delay_us(uint32_t us);

/**
 * @brief Polls a callback until it succeeds or the timeout elapses.
 * @param poll Callback invoked each iteration; must not be NULL.
 * @param ctx Opaque context passed to @p poll.
 * @param timeout_us Maximum wait time in microseconds.
 * @return HAL_OK when @p poll returns non-zero, HAL_ERR_PARAM if @p poll is NULL,
 *         HAL_ERR_TIMEOUT on expiry.
 */
hal_status_t hal_wait_flag_us(hal_poll_fn_t poll, void *ctx, uint32_t timeout_us);

/**
 * @brief Returns elapsed time in microseconds since reset (DWT-based).
 * @return Microseconds since boot.
 */
uint32_t hal_get_us(void);

#ifdef __cplusplus
}
#endif

#endif
