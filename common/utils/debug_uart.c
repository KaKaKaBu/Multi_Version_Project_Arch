#include "debug_uart.h"
#include "soft_uart.h"
#include "board_config.h"

#if HAL_DEBUG_UART_ENABLE

#ifndef BOARD_DEBUG_UART_BAUDRATE
#define BOARD_DEBUG_UART_BAUDRATE 57600U
#endif

static int debug_uart_ready;

void debug_uart_init(void)
{
    soft_uart_config_t cfg;

    cfg.tx_pin = board_debug_uart_tx;
    cfg.baudrate = BOARD_DEBUG_UART_BAUDRATE;
    soft_uart_init(&cfg);
    debug_uart_ready = 1;
}

int debug_uart_write(const char *buf, int len)
{
    if ((debug_uart_ready == 0) || (buf == 0) || (len <= 0)) {
        return len;
    }

    soft_uart_write((const uint8_t *)buf, (unsigned short)len);
    return len;
}

void debug_uart_trace_bytes(const uint8_t *data, unsigned short len)
{
    if ((debug_uart_ready == 0) || (data == 0) || (len == 0U)) {
        return;
    }

    soft_uart_write(data, len);
}

int debug_uart_uses_pin(const hal_pin_t *pin)
{
    if (pin == 0) {
        return 0;
    }

    return (pin->port == board_debug_uart_tx.port) &&
           (pin->pin == board_debug_uart_tx.pin);
}

#else

void debug_uart_init(void)
{
}

int debug_uart_write(const char *buf, int len)
{
    (void)buf;
    return len;
}

#endif
