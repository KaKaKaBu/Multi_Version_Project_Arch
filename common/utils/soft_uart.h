/**
 * @file soft_uart.h
 * @brief 单线软件 UART 发送（8N1，LSB 先发，DWT 周期延时）。
 *
 * 仅 TX、无 RX；发送字节时关中断以保证位时间。需 CPU 使能 DWT CYCCNT（调试时常默认开启）。
 */

#ifndef SOFT_UART_H
#define SOFT_UART_H

#include "gpio_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 软件 UART 发送配置。 */
typedef struct soft_uart_config {
    hal_pin_t tx_pin;       ///< 用作 TX 的 GPIO 引脚（空闲为高电平）。
    uint32_t baudrate;      ///< 比特率（位/秒）。
} soft_uart_config_t;

/**
 * @brief 在指定 TX 引脚与波特率上初始化软件 UART。
 * @param[in] cfg 配置；为 null 或 baudrate 为 0 时忽略。
 */
void soft_uart_init(const soft_uart_config_t *cfg);

/**
 * @brief 发送一个 8N1 字节（LSB 先发），帧期间屏蔽中断。
 * @param[in] byte 待发送数据字节。
 */
void soft_uart_write_byte(uint8_t byte);

/**
 * @brief 发送字节缓冲区。
 * @param[in] data 字节缓冲区；为 null 时忽略。
 * @param[in] len 待发送字节数。
 */
void soft_uart_write(const uint8_t *data, unsigned short len);

/**
 * @brief 发送以 null 结尾的 ASCII 字符串。
 * @param[in] str 待发送字符串；为 null 时忽略。
 */
void soft_uart_write_str(const char *str);

#ifdef __cplusplus
}
#endif

#endif
