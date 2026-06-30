#include "comm_if.h"
#include "usart_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static comm_rx_callback_t jdy31_rx_callback;
static const usart_device_config_t *jdy31_config;

static void jdy31_init(const void *config)
{
    usart_hal_config_t cfg;

    jdy31_config = (const usart_device_config_t *)config;
    if (jdy31_config == 0) {
        return;
    }

    cfg.instance = jdy31_config->instance;
    cfg.baudrate = jdy31_config->baudrate;
    cfg.tx = jdy31_config->tx;
    cfg.rx = jdy31_config->rx;
    cfg.remap = jdy31_config->remap;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = jdy31_config->tx_mode;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = jdy31_config->tx_dma_channel;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(jdy31_config->instance);
    jdy31_rx_callback = 0;
}

static int jdy31_send(const unsigned char *data, unsigned short len)
{
    if ((jdy31_config == 0) || (usart_hal_send_buffer(jdy31_config->instance, data, len) != HAL_OK)) {
        return -1;
    }

    return (int)len;
}

static int jdy31_recv(unsigned char *buf, unsigned short max_len)
{
    uint8_t data;

    if ((buf == 0) || (max_len == 0U)) {
        return -1;
    }

    if ((jdy31_config != 0) && (usart_hal_recv_byte(jdy31_config->instance, &data) == 1)) {
        buf[0] = data;
        if (jdy31_rx_callback != 0) {
            jdy31_rx_callback(buf, 1U);
        }
        return 1;
    }

    return 0;
}

static void jdy31_register_rx_callback(comm_rx_callback_t callback)
{
    jdy31_rx_callback = callback;
}

static const comm_driver_t jdy31_drv = {
    "jdy31",
    COMM_KIND_STREAM,
    jdy31_init,
    jdy31_send,
    jdy31_recv,
    jdy31_register_rx_callback
};

REGISTER_DRIVER(COMM, jdy31_drv);
