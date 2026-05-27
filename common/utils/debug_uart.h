#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "hal_features.h"
#include "gpio_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void debug_uart_init(void);
int debug_uart_write(const char *buf, int len);

#if HAL_DEBUG_UART_ENABLE
void debug_uart_trace_bytes(const uint8_t *data, unsigned short len);
int debug_uart_uses_pin(const hal_pin_t *pin);
#else
static inline void debug_uart_trace_bytes(const uint8_t *data, unsigned short len)
{
    (void)data;
    (void)len;
}

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
