#include "comm_if.h"
#include "usart_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

/*
 * Generic serial passthrough TTS driver.
 *
 * The voice module expects already-encoded GB2312 bytes. This driver only
 * initializes the configured UART and writes bytes transparently.
 */

static comm_rx_callback_t tts_uart_rx_callback;
static const usart_device_config_t *tts_uart_config;

static int tts_uart_send_byte(uint8_t data)
{
    if (tts_uart_config == 0) {
        return -1;
    }
    return (usart_hal_send_byte(tts_uart_config->instance, data) == HAL_OK) ? 1 : -1;
}

static void tts_uart_init(const void *config)
{
    usart_hal_config_t cfg;

    tts_uart_config = (const usart_device_config_t *)config;
    if (tts_uart_config == 0) {
        return;
    }

    cfg.instance = tts_uart_config->instance;
    cfg.baudrate = tts_uart_config->baudrate;
    cfg.tx = tts_uart_config->tx;
    cfg.rx = tts_uart_config->rx;
    cfg.remap = tts_uart_config->remap;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = tts_uart_config->tx_mode;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = tts_uart_config->tx_dma_channel;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(tts_uart_config->instance);
    tts_uart_rx_callback = 0;
}

static int tts_uart_send(const unsigned char *data, unsigned short len)
{
    uint16_t i;

    if ((data == 0) || (len == 0U)) {
        return -1;
    }

    for (i = 0U; i < len; ++i) {
        if (tts_uart_send_byte(data[i]) < 0) {
            return -1;
        }
    }

    return (int)len;
}

static int tts_uart_recv(unsigned char *buf, unsigned short max_len)
{
    uint8_t data;

    if ((buf == 0) || (max_len == 0U)) {
        return -1;
    }

    if ((tts_uart_config != 0) && (usart_hal_recv_byte(tts_uart_config->instance, &data) == 1)) {
        buf[0] = data;
        if (tts_uart_rx_callback != 0) {
            tts_uart_rx_callback(buf, 1U);
        }
        return 1;
    }

    return 0;
}

static void tts_uart_register_rx_callback(comm_rx_callback_t callback)
{
    tts_uart_rx_callback = callback;
}

static const comm_driver_t tts_uart_drv = {
    "tts_uart",
    COMM_KIND_STREAM,
    tts_uart_init,
    tts_uart_send,
    tts_uart_recv,
    tts_uart_register_rx_callback
};

REGISTER_DRIVER(COMM, tts_uart_drv);
