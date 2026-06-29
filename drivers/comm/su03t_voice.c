#include "comm_if.h"
#include "usart_hal.h"
#include "board_config.h"
#include "driver_core.h"

static comm_rx_callback_t su03t_rx_callback;

static int su03t_send_byte(uint8_t data)
{
    return (usart_hal_send_byte(BOARD_SU03T_USART, data) == HAL_OK) ? 1 : -1;
}

static void su03t_send_cmd(int len, const int *payload)
{
    int i;

    (void)su03t_send_byte(0xAAU);
    (void)su03t_send_byte(0x55U);

    for (i = 0; i < len; ++i) {
        (void)su03t_send_byte((uint8_t)payload[i]);
    }

    (void)su03t_send_byte(0x55U);
    (void)su03t_send_byte(0xAAU);
}

static void su03t_send_cmd1(int dat1, int dat2)
{
    int payload[2];

    payload[0] = dat1;
    payload[1] = dat2;
    su03t_send_cmd(2, payload);
}

static void su03t_send_cmd2(int dat1, int dat2, int dat3)
{
    int payload[3];

    payload[0] = dat1;
    payload[1] = dat2;
    payload[2] = dat3;
    su03t_send_cmd(3, payload);
}

static void su03t_init(void)
{
    usart_hal_config_t cfg;

    cfg.instance = BOARD_SU03T_USART;
    cfg.baudrate = BOARD_SU03T_BAUDRATE;
    cfg.tx = board_su03t_tx;
    cfg.rx = board_su03t_rx;
    cfg.remap = BOARD_SU03T_USART_REMAP;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = BOARD_SU03T_USART_TX_MODE;
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(BOARD_SU03T_USART);
    su03t_rx_callback = 0;
}

static int su03t_send(const unsigned char *data, unsigned short len)
{
    uint16_t i;

    if ((data == 0) || (len == 0U)) {
        return -1;
    }

    if ((len == 2U) && (data[0] <= 0x7FU) && (data[1] <= 0x7FU)) {
        su03t_send_cmd1((int)data[0], (int)data[1]);
        return (int)len;
    }

    if ((len == 3U) && (data[0] <= 0x7FU) && (data[1] <= 0x7FU) && (data[2] <= 0x7FU)) {
        su03t_send_cmd2((int)data[0], (int)data[1], (int)data[2]);
        return (int)len;
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

    if (usart_hal_recv_byte(BOARD_SU03T_USART, &data) == 1) {
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
