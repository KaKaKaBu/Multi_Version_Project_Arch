#include "comm_if.h"
#include "usart_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static comm_rx_callback_t su03t_rx_callback;
static const usart_device_config_t *su03t_config;

static int su03t_send_byte(uint8_t data)
{
    if (su03t_config == 0) {
        return -1;
    }
    return (usart_hal_send_byte(su03t_config->instance, data) == HAL_OK) ? 1 : -1;
}

static void su03t_init(const void *config)
{
    usart_hal_config_t cfg;

    su03t_config = (const usart_device_config_t *)config;
    if (su03t_config == 0) {
        return;
    }

    cfg.instance = su03t_config->instance;
    cfg.baudrate = su03t_config->baudrate;
    cfg.tx = su03t_config->tx;
    cfg.rx = su03t_config->rx;
    cfg.remap = su03t_config->remap;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = su03t_config->tx_mode;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = su03t_config->tx_dma_channel;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(su03t_config->instance);
    su03t_rx_callback = 0;
}

static int su03t_send(const unsigned char *data, unsigned short len)
{
    uint16_t i;

    if ((data == 0) || (len == 0U)) {
        return -1;
    }

    for (i = 0U; i < len; ++i) {
        if (su03t_send_byte(data[i]) < 0) {
            return -1;
        }
    }

    return (int)len;
}

static int su03t_recv(unsigned char *buf, unsigned short max_len)
{
    uint8_t data;

    if ((buf == 0) || (max_len == 0U)) {
        return -1;
    }

    if ((su03t_config != 0) && (usart_hal_recv_byte(su03t_config->instance, &data) == 1)) {
        buf[0] = data;
        if (su03t_rx_callback != 0) {
            su03t_rx_callback(buf, 1U);
        }
        return 1;
    }

    return 0;
}

static void su03t_register_rx_callback(comm_rx_callback_t callback)
{
    su03t_rx_callback = callback;
}

static const comm_driver_t su03t_drv = {
    "su03t",
    COMM_KIND_STREAM,
    su03t_init,
    su03t_send,
    su03t_recv,
    su03t_register_rx_callback
};

REGISTER_DRIVER(COMM, su03t_drv);
