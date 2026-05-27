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

static uint32_t hal_cycles_per_us(void)
{
    return SystemCoreClock / 1000000U;
}

static void hal_dwt_enable(void)
{
    HAL_COREDEBUG_DEMCR |= HAL_COREDEBUG_TRCENA;
    HAL_DWT_CYCCNT = 0U;
    HAL_DWT_CTRL |= HAL_DWT_CYCCNTENA;
}

void bsp_init(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    hal_dwt_enable();
    (void)SysTick_Config(SystemCoreClock / 1000U);
    mem_pool_init();
    cjson_port_init();
    debug_uart_init();
}

uint32_t hal_get_us(void)
{
    return HAL_DWT_CYCCNT / hal_cycles_per_us();
}

void hal_delay_us(uint32_t us)
{
    uint32_t start = HAL_DWT_CYCCNT;
    uint32_t cycles = us * hal_cycles_per_us();

    while ((HAL_DWT_CYCCNT - start) < cycles) {
    }
}

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
