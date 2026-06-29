#include "comm_if.h"
#include "usart_hal.h"
#include "board_config.h"
#include "driver_core.h"

static comm_rx_callback_t jdy31_rx_callback;

static void jdy31_init(void)
{
    usart_hal_config_t cfg;

    cfg.instance = BOARD_JDY31_USART;
    cfg.baudrate = BOARD_JDY31_BAUDRATE;
    cfg.tx = board_jdy31_tx;
    cfg.rx = board_jdy31_rx;
    cfg.remap = BOARD_JDY31_USART_REMAP;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = BOARD_JDY31_USART_TX_MODE;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = BOARD_JDY31_USART_TX_DMA;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(BOARD_JDY31_USART);
    jdy31_rx_callback = 0;
}

static int jdy31_send(const unsigned char *data, unsigned short len)
{
    if (usart_hal_send_buffer(BOARD_JDY31_USART, data, len) != HAL_OK) {
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

    if (usart_hal_recv_byte(BOARD_JDY31_USART, &data) == 1) {
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
