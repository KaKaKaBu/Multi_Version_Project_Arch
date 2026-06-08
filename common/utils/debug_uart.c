/**
 * @file debug_uart.c
 * @brief 调试串口：board_debug_uart_tx + BOARD_DEBUG_UART_BAUDRATE 初始化 soft_uart。
 */

#include "debug_uart.h"
#include "soft_uart.h"
#include "board_config.h"

#if HAL_DEBUG_UART_ENABLE

#ifndef BOARD_DEBUG_UART_BAUDRATE
/** @brief 未定义 BOARD_DEBUG_UART_BAUDRATE 时的默认调试串口波特率。 */
#define BOARD_DEBUG_UART_BAUDRATE 57600U
#endif

static int debug_uart_ready; ///< debug_uart_init 成功后为非零。

/**
 * @brief 使用 board_config 中的 TX 引脚与波特率初始化调试串口。
 */
void debug_uart_init(void)
{
    soft_uart_config_t cfg;

    cfg.tx_pin = board_debug_uart_tx;
    cfg.baudrate = BOARD_DEBUG_UART_BAUDRATE;
    soft_uart_init(&cfg);
    debug_uart_ready = 1;
}

/**
 * @brief 向调试串口写入原始字节。
 * @param[in] buf 数据缓冲区。
 * @param[in] len 待写字节数。
 * @return 实际接受的字节数（通常为 len）；未就绪或参数无效时仍返回 len。
 */
int debug_uart_write(const char *buf, int len)
{
    if ((debug_uart_ready == 0) || (buf == 0) || (len <= 0)) {
        return len;
    }

    soft_uart_write((const uint8_t *)buf, (unsigned short)len);
    return len;
}

/**
 * @brief 向调试串口写入二进制跟踪缓冲区。
 * @param[in] data 字节缓冲区。
 * @param[in] len 字节数。
 */
void debug_uart_trace_bytes(const uint8_t *data, unsigned short len)
{
    if ((debug_uart_ready == 0) || (data == 0) || (len == 0U)) {
        return;
    }

    soft_uart_write(data, len);
}

/**
 * @brief 判断指定 GPIO 是否用作调试串口 TX 线。
 * @param[in] pin 待检查的引脚描述符。
 * @return 与 board 调试 TX 匹配返回 1，否则返回 0。
 */
int debug_uart_uses_pin(const hal_pin_t *pin)
{
    if (pin == 0) {
        return 0;
    }

    return (pin->port == board_debug_uart_tx.port) &&
           (pin->pin == board_debug_uart_tx.pin);
}

#else

/**
 * @brief HAL_DEBUG_UART_ENABLE 关闭时的空 init。
 */
void debug_uart_init(void)
{
}

/**
 * @brief 调试串口禁用时不实际发送数据。
 * @param[in] buf 忽略。
 * @param[in] len 原样作为已接受字节数返回。
 * @return len。
 */
int debug_uart_write(const char *buf, int len)
{
    (void)buf;
    return len;
}

#endif
