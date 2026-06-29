#include "gas_if.h"
#include "usart_hal.h"
#include "board_config.h"
#include "driver_core.h"

#include <string.h>

#define CO2_RX_BUF_SIZE 64U

static uint8_t co2_rx_buf[CO2_RX_BUF_SIZE];
static uint32_t co2_rx_cnt;
static uint16_t co2_ppm;

static void co2_drain_rx(void)
{
    uint8_t byte;

    while ((co2_rx_cnt < CO2_RX_BUF_SIZE) && (usart_hal_recv_byte(BOARD_CO2_USART, &byte) == 1)) {
        co2_rx_buf[co2_rx_cnt++] = byte;
    }
}

static void co2_parse_frame(void)
{
    if ((co2_rx_cnt != 0U) &&
        (co2_rx_buf[5] == (uint8_t)(co2_rx_buf[0] + co2_rx_buf[1] + co2_rx_buf[2] + co2_rx_buf[3] + co2_rx_buf[4]))) {
        co2_ppm = (uint16_t)((uint16_t)co2_rx_buf[1] * 256U + co2_rx_buf[2]);
    }

    memset(co2_rx_buf, 0, co2_rx_cnt);
    co2_rx_cnt = 0U;
}

static void co2_init(void)
{
    usart_hal_config_t cfg;

    co2_rx_cnt = 0U;
    co2_ppm = 0U;
    memset(co2_rx_buf, 0, sizeof(co2_rx_buf));

    cfg.instance = BOARD_CO2_USART;
    cfg.baudrate = BOARD_CO2_BAUDRATE;
    cfg.tx = board_co2_tx;
    cfg.rx = board_co2_rx;
    cfg.remap = BOARD_CO2_USART_REMAP;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = USART_HAL_TX_MODE_IRQ;
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(BOARD_CO2_USART);
}

static unsigned short co2_read_ppm(void)
{
    co2_drain_rx();
    co2_parse_frame();
    return co2_ppm;
}

static const gas_sensor_t co2_drv = {
    "co2",
    co2_init,
    co2_read_ppm
};

REGISTER_DRIVER(GAS_SENSOR, co2_drv);
