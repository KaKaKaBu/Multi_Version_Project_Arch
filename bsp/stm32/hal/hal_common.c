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
#define HAL_DWT_PROBE_LOOPS 64U
#define HAL_DWT_STALL_GUARD_LOOPS 1000000U

static uint8_t hal_dwt_ready;

/**
 * @brief Returns CPU cycles per microsecond from SystemCoreClock.
 * @return Cycles per microsecond.
 *
 * 中文：根据当前 SystemCoreClock 计算每微秒对应的 CPU 周期数，供 DWT 计时与延时换算使用。
 */
static uint32_t hal_cycles_per_us(void)
{
    return SystemCoreClock / 1000000U;
}

static void hal_fallback_delay_us(uint32_t us)
{
    uint32_t cycles_per_us = hal_cycles_per_us();
    volatile uint32_t i;

    if (cycles_per_us == 0U) {
        cycles_per_us = 1U;
    }

    while (us > 0U) {
        for (i = 0U; i < cycles_per_us; ++i) {
            __asm volatile("nop");
        }
        --us;
    }
}

/**
 * @brief Enables the DWT cycle counter for timing.
 *
 * 中文：打开 Cortex-M3 的 DWT 计数器，后续 hal_get_us / hal_delay_us 可直接读取 CYCCNT 获得精确时间。
 */
static void hal_dwt_enable(void)
{
    volatile uint32_t i;
    uint32_t first;
    uint32_t second;

    HAL_COREDEBUG_DEMCR |= HAL_COREDEBUG_TRCENA;
    HAL_DWT_CYCCNT = 0U;
    HAL_DWT_CTRL |= HAL_DWT_CYCCNTENA;

    first = HAL_DWT_CYCCNT;
    for (i = 0U; i < HAL_DWT_PROBE_LOOPS; ++i) {
        __asm volatile("nop");
    }
    second = HAL_DWT_CYCCNT;
    hal_dwt_ready = (second != first) ? 1U : 0U;
}

/**
 * @brief Initializes NVIC, SysTick, DWT, memory pool, cJSON port, and debug UART.
 *
 * 中文：bsp_init() 是应用启动的第一步，依次完成中断优先级分组、1ms SysTick、DWT 计时、mem_pool/cJSON 钩子与调试串口，确保后续驱动与应用依赖的基础设施已经就绪。
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
 *
 * 中文：读取 DWT 周期计数器并转换为微秒，常用于轮询超时和调试时间戳。
 */
uint32_t hal_get_us(void)
{
    uint32_t cycles_per_us = hal_cycles_per_us();

    if ((hal_dwt_ready == 0U) || (cycles_per_us == 0U)) {
        return 0U;
    }

    return HAL_DWT_CYCCNT / cycles_per_us;
}

uint32_t hal_get_core_clock_hz(void)
{
    return SystemCoreClock;
}

uint32_t hal_irq_lock(void)
{
    uint32_t primask;

    __asm volatile("MRS %0, primask" : "=r"(primask));
    __disable_irq();
    return primask;
}

void hal_irq_unlock(uint32_t irq_state)
{
    if ((irq_state & 1U) == 0U) {
        __enable_irq();
    }
}

/**
 * @brief Blocks for the specified number of microseconds using the DWT cycle counter.
 * @param us Delay duration in microseconds.
 *
 * 中文：使用忙等待的方式在微秒级精度内延时，适合复位脉冲、位带协议等短延时场景。
 */
void hal_delay_us(uint32_t us)
{
    uint32_t start = HAL_DWT_CYCCNT;
    uint32_t cycles_per_us = hal_cycles_per_us();
    uint32_t cycles = us * cycles_per_us;
    uint32_t last = start;
    uint32_t stalled = 0U;

    if ((us == 0U) || (cycles_per_us == 0U)) {
        return;
    }

    if (hal_dwt_ready == 0U) {
        hal_fallback_delay_us(us);
        return;
    }

    while ((HAL_DWT_CYCCNT - start) < cycles) {
        uint32_t now = HAL_DWT_CYCCNT;
        if (now == last) {
            ++stalled;
            if (stalled >= HAL_DWT_STALL_GUARD_LOOPS) {
                hal_dwt_ready = 0U;
                hal_fallback_delay_us(us);
                break;
            }
        } else {
            last = now;
            stalled = 0U;
        }
    }
}

/**
 * @brief Polls a callback until it succeeds or the timeout elapses.
 * @param poll Callback invoked each iteration; must not be NULL.
 * @param ctx Opaque context passed to @p poll.
 * @param timeout_us Maximum wait time in microseconds.
 * @return HAL_OK when @p poll returns non-zero, HAL_ERR_PARAM if @p poll is NULL,
 *         HAL_ERR_TIMEOUT on expiry.
 *
 * 中文：通用的“等待硬件就绪”工具函数。poll 回调返回非 0 即视为成功，否则在超时时间内持续轮询；常用于 I2C/SPI/USART 等 HAL 检查忙标志。
 */
hal_status_t hal_wait_flag_us(hal_poll_fn_t poll, void *ctx, uint32_t timeout_us)
{
    uint32_t start = HAL_DWT_CYCCNT;
    uint32_t cycles_per_us = hal_cycles_per_us();
    uint32_t timeout_cycles = timeout_us * cycles_per_us;
    uint32_t elapsed_us = 0U;
    uint32_t last = start;
    uint32_t stalled = 0U;

    if (poll == 0) {
        return HAL_ERR_PARAM;
    }

    if ((hal_dwt_ready == 0U) || (cycles_per_us == 0U)) {
        while (elapsed_us < timeout_us) {
            if (poll(ctx) != 0U) {
                return HAL_OK;
            }
            hal_fallback_delay_us(1U);
            ++elapsed_us;
        }
        return HAL_ERR_TIMEOUT;
    }

    while ((HAL_DWT_CYCCNT - start) < timeout_cycles) {
        uint32_t now = HAL_DWT_CYCCNT;
        if (poll(ctx) != 0U) {
            return HAL_OK;
        }
        if (now == last) {
            ++stalled;
            if (stalled >= HAL_DWT_STALL_GUARD_LOOPS) {
                hal_dwt_ready = 0U;
                break;
            }
        } else {
            last = now;
            stalled = 0U;
        }
    }

    return HAL_ERR_TIMEOUT;
}
