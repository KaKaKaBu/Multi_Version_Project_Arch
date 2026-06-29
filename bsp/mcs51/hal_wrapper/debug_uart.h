#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "gpio_hal.h"

static inline void debug_uart_init(void) {}
static inline int debug_uart_write(const char *data, int len)
{
    (void)data;
    return len;
}
static inline unsigned char debug_uart_uses_pin(const hal_pin_t *pin)
{
    (void)pin;
    return 0U;
}

#endif
