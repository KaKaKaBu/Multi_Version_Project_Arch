#ifndef SOFT_UART_H
#define SOFT_UART_H

#include "gpio_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct soft_uart_config {
    hal_pin_t tx_pin;
    uint32_t baudrate;
} soft_uart_config_t;

void soft_uart_init(const soft_uart_config_t *cfg);
void soft_uart_write_byte(uint8_t byte);
void soft_uart_write(const uint8_t *data, unsigned short len);
void soft_uart_write_str(const char *str);

#ifdef __cplusplus
}
#endif

#endif
