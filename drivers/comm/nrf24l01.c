#include "comm_if.h"
#include "spi_hal.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

#include <string.h>

#if HAL_SPI_USE_HW

#define NRF24L01_R_REGISTER 0x00U
#define NRF24L01_W_REGISTER 0x20U
#define NRF24L01_R_RX_PAYLOAD 0x61U
#define NRF24L01_W_TX_PAYLOAD 0xA0U
#define NRF24L01_FLUSH_TX 0xE1U
#define NRF24L01_FLUSH_RX 0xE2U
#define NRF24L01_NOP 0xFFU

#define NRF24L01_CONFIG 0x00U
#define NRF24L01_EN_AA 0x01U
#define NRF24L01_EN_RXADDR 0x02U
#define NRF24L01_SETUP_AW 0x03U
#define NRF24L01_SETUP_RETR 0x04U
#define NRF24L01_RF_CH 0x05U
#define NRF24L01_RF_SETUP 0x06U
#define NRF24L01_STATUS 0x07U
#define NRF24L01_RX_ADDR_P0 0x0AU
#define NRF24L01_TX_ADDR 0x10U
#define NRF24L01_RX_PW_P0 0x11U

#define NRF24L01_ADDR_WIDTH 5U
#define NRF24L01_TX_PACKET_WIDTH 32U
#define NRF24L01_RX_PACKET_WIDTH 32U
#define NRF24L01_SEND_TIMEOUT 10000U

static uint8_t nrf24_tx_address[NRF24L01_ADDR_WIDTH] = { 0x11U, 0x22U, 0x33U, 0x44U, 0x55U };
static uint8_t nrf24_rx_address[NRF24L01_ADDR_WIDTH] = { 0x11U, 0x22U, 0x33U, 0x44U, 0x55U };
static uint8_t nrf24_tx_packet[NRF24L01_TX_PACKET_WIDTH];
static uint8_t nrf24_rx_packet[NRF24L01_RX_PACKET_WIDTH];
static comm_rx_callback_t nrf24_rx_callback;
static const spi_device_config_t *nrf24_config;

static void nrf24_ce_write(uint8_t value)
{
    gpio_hal_write(nrf24_config->aux.port, nrf24_config->aux.pin, value);
}

static void nrf24_csn_write(uint8_t value)
{
    gpio_hal_write(nrf24_config->cs.port, nrf24_config->cs.pin, value);
}

static uint8_t nrf24_spi_swap(uint8_t byte)
{
    return spi_hal_transfer_byte(nrf24_config->bus_id, byte);
}

static uint8_t nrf24_read_reg(uint8_t reg)
{
    uint8_t data;

    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_R_REGISTER | reg);
    data = nrf24_spi_swap(NRF24L01_NOP);
    nrf24_csn_write(1U);
    return data;
}

static void nrf24_read_regs(uint8_t reg, uint8_t *data_array, uint8_t count)
{
    uint8_t i;

    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_R_REGISTER | reg);
    for (i = 0U; i < count; ++i) {
        data_array[i] = nrf24_spi_swap(NRF24L01_NOP);
    }
    nrf24_csn_write(1U);
}

static void nrf24_write_reg(uint8_t reg, uint8_t data)
{
    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_W_REGISTER | reg);
    (void)nrf24_spi_swap(data);
    nrf24_csn_write(1U);
}

static void nrf24_write_regs(uint8_t reg, const uint8_t *data_array, uint8_t count)
{
    uint8_t i;

    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_W_REGISTER | reg);
    for (i = 0U; i < count; ++i) {
        (void)nrf24_spi_swap(data_array[i]);
    }
    nrf24_csn_write(1U);
}

static void nrf24_read_rx_payload(uint8_t *data_array, uint8_t count)
{
    uint8_t i;

    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_R_RX_PAYLOAD);
    for (i = 0U; i < count; ++i) {
        data_array[i] = nrf24_spi_swap(NRF24L01_NOP);
    }
    nrf24_csn_write(1U);
}

static void nrf24_write_tx_payload(const uint8_t *data_array, uint8_t count)
{
    uint8_t i;

    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_W_TX_PAYLOAD);
    for (i = 0U; i < count; ++i) {
        (void)nrf24_spi_swap(data_array[i]);
    }
    nrf24_csn_write(1U);
}

static void nrf24_flush_tx(void)
{
    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_FLUSH_TX);
    nrf24_csn_write(1U);
}

static void nrf24_flush_rx(void)
{
    nrf24_csn_write(0U);
    (void)nrf24_spi_swap(NRF24L01_FLUSH_RX);
    nrf24_csn_write(1U);
}

static uint8_t nrf24_read_status(void)
{
    uint8_t status;

    nrf24_csn_write(0U);
    status = nrf24_spi_swap(NRF24L01_NOP);
    nrf24_csn_write(1U);
    return status;
}

static void nrf24_rx_mode(void)
{
    uint8_t config;

    nrf24_ce_write(0U);
    config = nrf24_read_reg(NRF24L01_CONFIG);
    if (config == 0xFFU) {
        return;
    }
    config |= 0x03U;
    nrf24_write_reg(NRF24L01_CONFIG, config);
    nrf24_ce_write(1U);
}

static void nrf24_tx_mode(void)
{
    uint8_t config;

    nrf24_ce_write(0U);
    config = nrf24_read_reg(NRF24L01_CONFIG);
    if (config == 0xFFU) {
        return;
    }
    config |= 0x02U;
    config &= (uint8_t)~0x01U;
    nrf24_write_reg(NRF24L01_CONFIG, config);
    nrf24_ce_write(1U);
}

static void nrf24_chip_init(void)
{
    nrf24_write_reg(NRF24L01_CONFIG, 0x08U);
    nrf24_write_reg(NRF24L01_EN_AA, 0x3FU);
    nrf24_write_reg(NRF24L01_EN_RXADDR, 0x01U);
    nrf24_write_reg(NRF24L01_SETUP_AW, 0x03U);
    nrf24_write_reg(NRF24L01_SETUP_RETR, 0x03U);
    nrf24_write_reg(NRF24L01_RF_CH, 0x02U);
    nrf24_write_reg(NRF24L01_RF_SETUP, 0x0EU);
    nrf24_write_reg(NRF24L01_RX_PW_P0, NRF24L01_RX_PACKET_WIDTH);
    nrf24_write_regs(NRF24L01_RX_ADDR_P0, nrf24_rx_address, NRF24L01_ADDR_WIDTH);
    nrf24_flush_tx();
    nrf24_flush_rx();
    nrf24_write_reg(NRF24L01_STATUS, 0x70U);
    nrf24_rx_mode();
}

static uint8_t nrf24_do_send(void)
{
    uint8_t status;
    uint8_t send_flag = 0U;
    uint32_t timeout = NRF24L01_SEND_TIMEOUT;

    nrf24_write_regs(NRF24L01_TX_ADDR, nrf24_tx_address, NRF24L01_ADDR_WIDTH);
    nrf24_write_regs(NRF24L01_RX_ADDR_P0, nrf24_tx_address, NRF24L01_ADDR_WIDTH);
    nrf24_write_tx_payload(nrf24_tx_packet, NRF24L01_TX_PACKET_WIDTH);
    nrf24_tx_mode();

    while (1) {
        status = nrf24_read_status();
        if (--timeout == 0U) {
            send_flag = 4U;
            nrf24_chip_init();
            break;
        }
        if ((status & 0x30U) == 0x30U) {
            send_flag = 3U;
            nrf24_chip_init();
            break;
        }
        if ((status & 0x10U) == 0x10U) {
            send_flag = 2U;
            nrf24_chip_init();
            break;
        }
        if ((status & 0x20U) == 0x20U) {
            send_flag = 1U;
            break;
        }
    }

    nrf24_write_reg(NRF24L01_STATUS, 0x30U);
    nrf24_flush_tx();
    nrf24_write_regs(NRF24L01_RX_ADDR_P0, nrf24_rx_address, NRF24L01_ADDR_WIDTH);
    nrf24_rx_mode();
    return send_flag;
}

static uint8_t nrf24_do_receive(void)
{
    uint8_t status;
    uint8_t config;
    uint8_t receive_flag;

    status = nrf24_read_status();
    config = nrf24_read_reg(NRF24L01_CONFIG);

    if ((config & 0x02U) == 0x00U) {
        receive_flag = 3U;
        nrf24_chip_init();
    } else if ((status & 0x30U) == 0x30U) {
        receive_flag = 2U;
        nrf24_chip_init();
    } else if ((status & 0x40U) == 0x40U) {
        receive_flag = 1U;
        nrf24_read_rx_payload(nrf24_rx_packet, NRF24L01_RX_PACKET_WIDTH);
        nrf24_write_reg(NRF24L01_STATUS, 0x40U);
        nrf24_flush_rx();
    } else {
        receive_flag = 0U;
    }

    return receive_flag;
}

static void nrf24_init(const void *config)
{
    spi_hal_config_t cfg;

    nrf24_config = (const spi_device_config_t *)config;
    if (nrf24_config == 0) {
        return;
    }

    gpio_hal_config_pin(&nrf24_config->aux);
    gpio_hal_config_pin(&nrf24_config->cs);
    nrf24_ce_write(0U);
    nrf24_csn_write(1U);

    cfg.instance = nrf24_config->instance;
    cfg.sck = nrf24_config->sck;
    cfg.mosi = nrf24_config->mosi;
    cfg.miso = nrf24_config->miso;
    cfg.remap = nrf24_config->remap;
    cfg.prescaler = nrf24_config->prescaler;
    cfg.cpol = nrf24_config->cpol;
    cfg.cpha = nrf24_config->cpha;
    cfg.timeout_us = SPI_HAL_DEFAULT_TIMEOUT_US;
#if HAL_SPI_ENABLE_DMA
    cfg.tx_dma_channel = nrf24_config->tx_dma_channel;
    cfg.rx_dma_channel = nrf24_config->rx_dma_channel;
#endif
    (void)spi_hal_init(nrf24_config->bus_id, &cfg);
    nrf24_rx_callback = 0;
    nrf24_chip_init();
}

static int nrf24_send(const unsigned char *data, unsigned short len)
{
    uint16_t copy_len = len;

    if ((nrf24_config == 0) || (data == 0) || (len == 0U)) {
        return -1;
    }

    if (copy_len > NRF24L01_TX_PACKET_WIDTH) {
        copy_len = NRF24L01_TX_PACKET_WIDTH;
    }

    memset(nrf24_tx_packet, 0, sizeof(nrf24_tx_packet));
    memcpy(nrf24_tx_packet, data, copy_len);

    if (nrf24_do_send() != 1U) {
        return -1;
    }

    return (int)copy_len;
}

static int nrf24_recv(unsigned char *data, unsigned short max_len)
{
    uint16_t copy_len;

    if ((nrf24_config == 0) || (data == 0) || (max_len == 0U)) {
        return -1;
    }

    if (nrf24_do_receive() != 1U) {
        return 0;
    }

    copy_len = max_len;
    if (copy_len > NRF24L01_RX_PACKET_WIDTH) {
        copy_len = NRF24L01_RX_PACKET_WIDTH;
    }

    memcpy(data, nrf24_rx_packet, copy_len);
    if (nrf24_rx_callback != 0) {
        nrf24_rx_callback(data, copy_len);
    }
    return (int)copy_len;
}

static void nrf24_register_rx_callback(comm_rx_callback_t callback)
{
    nrf24_rx_callback = callback;
}

static const comm_driver_t nrf24_drv = {
    "nrf24",
    COMM_KIND_PACKET,
    nrf24_init,
    nrf24_send,
    nrf24_recv,
    nrf24_register_rx_callback
};

REGISTER_DRIVER(COMM, nrf24_drv);

#endif
