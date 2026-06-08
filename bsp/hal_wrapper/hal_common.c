/**
 * @file hal_common.c
 * @brief BSP initialization, DWT-based microsecond timing, and poll-with-timeout helper.
 */

#include "hal_common.h"
#include "cjson_port.h"
#include "mem_pool.h"
#include "debug_uart.h"
#include "misc.h"
#include "stm32f10x.h"

#define HAL_DWT_CTRL (*(volatile uint32_t *)0xE0001000U)
#define HAL_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004U)
#define HAL_COREDEBUG_DEMCR (*(volatile uint32_t *)0xE000EDFCU)
#define HAL_COREDEBUG_TRCENA (1UL << 24U)
#define HAL_DWT_CYCCNTENA (1UL << 0U)

/**
 * @brief Returns CPU cycles per microsecond from SystemCoreClock.
 * @return Cycles per microsecond.
 */
static uint32_t hal_cycles_per_us(void)
{
    return SystemCoreClock / 1000000U;
}

/**
 * @brief Enables the DWT cycle counter for timing.
 */
static void hal_dwt_enable(void)
{
    HAL_COREDEBUG_DEMCR |= HAL_COREDEBUG_TRCENA;
    HAL_DWT_CYCCNT = 0U;
    HAL_DWT_CTRL |= HAL_DWT_CYCCNTENA;
}

/**
 * @brief Initializes NVIC, SysTick, DWT, memory pool, cJSON port, and debug UART.
 */
void bsp_init(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    hal_dwt_enable();
    (void)SysTick_Config(SystemCoreClock / 1000U);
    mem_pool_init();
    cjson_port_init();
    debug_uart_init();
}

/**
 * @brief Returns elapsed time in microseconds since reset (DWT-based).
 * @return Microseconds since boot.
 */
uint32_t hal_get_us(void)
{
    return HAL_DWT_CYCCNT / hal_cycles_per_us();
}

/**
 * @brief Blocks for the specified number of microseconds using the DWT cycle counter.
 * @param us Delay duration in microseconds.
 */
void hal_delay_us(uint32_t us)
{
    uint32_t start = HAL_DWT_CYCCNT;
    uint32_t cycles = us * hal_cycles_per_us();

    while ((HAL_DWT_CYCCNT - start) < cycles) {
    }
}

/**
 * @brief Polls a callback until it succeeds or the timeout elapses.
 * @param poll Callback invoked each iteration; must not be NULL.
 * @param ctx Opaque context passed to @p poll.
 * @param timeout_us Maximum wait time in microseconds.
 * @return HAL_OK when @p poll returns non-zero, HAL_ERR_PARAM if @p poll is NULL,
 *         HAL_ERR_TIMEOUT on expiry.
 */
hal_status_t hal_wait_flag_us(hal_poll_fn_t poll, void *ctx, uint32_t timeout_us)
{
    uint32_t start = HAL_DWT_CYCCNT;
    uint32_t timeout_cycles = timeout_us * hal_cycles_per_us();

    if (poll == 0) {
        return HAL_ERR_PARAM;
    }

    while ((HAL_DWT_CYCCNT - start) < timeout_cycles) {
        if (poll(ctx) != 0U) {
            return HAL_OK;
        }
    }

    return HAL_ERR_TIMEOUT;
}
