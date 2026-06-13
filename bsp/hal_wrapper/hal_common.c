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
 *
 * 中文：根据当前 SystemCoreClock 计算每微秒对应的 CPU 周期数，供 DWT 计时与延时换算使用。
 */
static uint32_t hal_cycles_per_us(void)
{
    return SystemCoreClock / 1000000U;
}

/**
 * @brief Enables the DWT cycle counter for timing.
 *
 * 中文：打开 Cortex-M3 的 DWT 计数器，后续 hal_get_us / hal_delay_us 可直接读取 CYCCNT 获得精确时间。
 */
static void hal_dwt_enable(void)
{
    HAL_COREDEBUG_DEMCR |= HAL_COREDEBUG_TRCENA;
    HAL_DWT_CYCCNT = 0U;
    HAL_DWT_CTRL |= HAL_DWT_CYCCNTENA;
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
    return HAL_DWT_CYCCNT / hal_cycles_per_us();
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
 *
 * 中文：通用的“等待硬件就绪”工具函数。poll 回调返回非 0 即视为成功，否则在超时时间内持续轮询；常用于 I2C/SPI/USART 等 HAL 检查忙标志。
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
