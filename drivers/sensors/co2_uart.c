#include "gas_if.h"
#include "usart_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

#include <string.h>

#define CO2_RX_BUF_SIZE 64U

static uint8_t co2_rx_buf[CO2_RX_BUF_SIZE];
static uint32_t co2_rx_cnt;
static uint16_t co2_ppm;
static const usart_device_config_t *co2_config;

static void co2_drain_rx(void)
{
    uint8_t byte;

    if (co2_config == 0) {
        return;
    }

    while ((co2_rx_cnt < CO2_RX_BUF_SIZE) && (usart_hal_recv_byte(co2_config->instance, &byte) == 1)) {
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

static void co2_init(const void *config)
{
    usart_hal_config_t cfg;

    co2_rx_cnt = 0U;
    co2_ppm = 0U;
    memset(co2_rx_buf, 0, sizeof(co2_rx_buf));

    co2_config = (const usart_device_config_t *)config;
    if (co2_config == 0) {
        return;
    }

    cfg.instance = co2_config->instance;
    cfg.baudrate = co2_config->baudrate;
    cfg.tx = co2_config->tx;
    cfg.rx = co2_config->rx;
    cfg.remap = co2_config->remap;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = co2_config->tx_mode;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = co2_config->tx_dma_channel;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(co2_config->instance);
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
