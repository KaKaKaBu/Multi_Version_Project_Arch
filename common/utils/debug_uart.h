/**
 * @file debug_uart.h
 * @brief 板级调试 UART 门面（HAL_DEBUG_UART_ENABLE 时走 soft_uart 位bang TX）。
 *
 * 与 printf 重定向配合输出日志；禁用时 write 为空操作但仍返回 len，避免调用方判错。
 */

#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "hal_features.h"
#include "gpio_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 使用 board_config 中的 TX 引脚与波特率初始化调试 UART。
 */
void debug_uart_init(void);

/**
 * @brief 向调试 UART 写入原始字节。
 * @param[in] buf 数据缓冲区。
 * @param[in] len 待写入字节数。
 * @return 已接受的字节数（len）；禁用或未就绪时仍返回 len。
 */
int debug_uart_write(const char *buf, int len);

#if HAL_DEBUG_UART_ENABLE
/**
 * @brief 向调试 UART 写入二进制跟踪缓冲区。
 * @param[in] data 字节缓冲区。
 * @param[in] len 字节长度。
 */
void debug_uart_trace_bytes(const uint8_t *data, unsigned short len);

/**
 * @brief 报告某 GPIO 引脚是否用作调试 UART TX 线。
 * @param[in] pin 待检查的引脚描述符。
 * @return 引脚与板级调试 TX 匹配时返回 1，否则返回 0。
 */
int debug_uart_uses_pin(const hal_pin_t *pin);
#else
/**
 * @brief 调试 UART 禁用时的空操作跟踪。
 * @param[in] data 字节缓冲区（未使用）。
 * @param[in] len 字节长度（未使用）。
 */
static inline void debug_uart_trace_bytes(const uint8_t *data, unsigned short len)
{
    (void)data;
    (void)len;
}

/**
 * @brief 调试 UART 禁用时始终返回 0。
 * @param[in] pin 引脚描述符（未使用）。
 * @return 恒为 0。
 */
static inline int debug_uart_uses_pin(const hal_pin_t *pin)
{
    (void)pin;
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
