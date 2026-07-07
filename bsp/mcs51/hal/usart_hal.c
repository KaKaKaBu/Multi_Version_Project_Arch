#include "usart_hal.h"

#ifndef MCS51_FOSC_HZ
#define MCS51_FOSC_HZ 11059200UL
#endif

#define MCS51_UART0_INSTANCE 0U
#define MCS51_UART_BAUD_ERROR_PERCENT 3UL

#if defined(__SDCC)
__sfr __at(0x87) PCON;
__sfr __at(0x89) TMOD;
__sfr __at(0x8B) TL1;
__sfr __at(0x8D) TH1;
__sfr __at(0x98) SCON;
__sfr __at(0x99) SBUF;

__sbit __at(0x8E) TR1;
__sbit __at(0x98) RI;
__sbit __at(0x99) TI;
#endif

static uint8_t usart_configured;
static uint32_t usart_tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;

static uint8_t usart_instance_valid(hal_usart_id_t instance)
{
    return ((instance == MCS51_UART0_INSTANCE) || (instance == HAL_USART_ID_1)) ? 1U : 0U;
}

static uint32_t usart_abs_diff(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static uint8_t usart_calc_timer1_reload(uint32_t baudrate, uint8_t *reload, uint8_t *smod)
{
    uint8_t mode;
    uint8_t found = 0U;
    uint8_t best_reload = 0U;
    uint8_t best_smod = 0U;
    uint32_t best_error = 0xFFFFFFFFUL;

    if ((baudrate == 0UL) || (reload == 0) || (smod == 0)) {
        return 0U;
    }

    for (mode = 0U; mode < 2U; ++mode) {
        uint32_t divisor = (mode == 0U) ? (384UL * baudrate) : (192UL * baudrate);
        uint32_t ticks;
        uint32_t actual_baud;
        uint32_t error;

        if (divisor == 0UL) {
            continue;
        }

        ticks = (MCS51_FOSC_HZ + (divisor / 2UL)) / divisor;
        if ((ticks == 0UL) || (ticks > 255UL)) {
            continue;
        }

        actual_baud = MCS51_FOSC_HZ / (((mode == 0U) ? 384UL : 192UL) * ticks);
        error = usart_abs_diff(actual_baud, baudrate);
        if ((found == 0U) || (error < best_error)) {
            found = 1U;
            best_error = error;
            best_reload = (uint8_t)(256UL - ticks);
            best_smod = mode;
        }
    }

    if (found == 0U) {
        return 0U;
    }
    if ((best_error * 100UL) > (baudrate * MCS51_UART_BAUD_ERROR_PERCENT)) {
        return 0U;
    }

    *reload = best_reload;
    *smod = best_smod;
    return 1U;
}

hal_status_t usart_hal_init(const usart_hal_config_t *cfg)
{
    uint8_t reload;
    uint8_t smod;

    if ((cfg == 0) || (usart_instance_valid(cfg->instance) == 0U)) {
        return HAL_ERR_PARAM;
    }
    if (usart_calc_timer1_reload(cfg->baudrate, &reload, &smod) == 0U) {
        return HAL_ERR_PARAM;
    }

    gpio_hal_config_pin(&cfg->tx);
    gpio_hal_config_pin(&cfg->rx);

#if defined(__SDCC)
    TMOD = (uint8_t)((TMOD & 0x0FU) | 0x20U);
    if (smod != 0U) {
        PCON |= 0x80U;
    } else {
        PCON &= (uint8_t)~0x80U;
    }
    TH1 = reload;
    TL1 = reload;
    SCON = 0x50U;
    TR1 = 1U;
    RI = 0U;
    TI = 0U;
#endif

    usart_tx_timeout_us = (cfg->tx_timeout_us == 0UL) ? USART_HAL_DEFAULT_TX_TIMEOUT_US : cfg->tx_timeout_us;
    usart_configured = 1U;
    return HAL_OK;
}

hal_status_t usart_hal_send_byte(hal_usart_id_t USARTx, uint8_t data)
{
    uint32_t timeout;

    if ((usart_configured == 0U) || (usart_instance_valid(USARTx) == 0U)) {
        return HAL_ERR_PARAM;
    }

#if defined(__SDCC)
    TI = 0U;
    SBUF = data;

    timeout = usart_tx_timeout_us;
    while ((TI == 0U) && (timeout > 0UL)) {
        --timeout;
        hal_delay_us(1U);
    }
    if (TI == 0U) {
        return HAL_ERR_TIMEOUT;
    }
    TI = 0U;
#else
    (void)data;
    (void)timeout;
#endif

    return HAL_OK;
}

hal_status_t usart_hal_send_buffer(hal_usart_id_t USARTx, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    hal_status_t status;

    if ((data == 0) && (len > 0U)) {
        return HAL_ERR_PARAM;
    }

    for (i = 0U; i < len; ++i) {
        status = usart_hal_send_byte(USARTx, data[i]);
        if (status != HAL_OK) {
            return status;
        }
    }

    return HAL_OK;
}

int usart_hal_recv_byte(hal_usart_id_t USARTx, uint8_t *data)
{
    if ((data == 0) || (usart_configured == 0U) || (usart_instance_valid(USARTx) == 0U)) {
        return -1;
    }

#if defined(__SDCC)
    if (RI == 0U) {
        return 0;
    }
    *data = SBUF;
    RI = 0U;
    return 1;
#else
    *data = 0U;
    return 0;
#endif
}

uint16_t usart_hal_rx_available(hal_usart_id_t USARTx)
{
    if ((usart_configured == 0U) || (usart_instance_valid(USARTx) == 0U)) {
        return 0U;
    }

#if defined(__SDCC)
    return (RI != 0U) ? 1U : 0U;
#else
    return 0U;
#endif
}

hal_status_t usart_hal_read(hal_usart_id_t USARTx, uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint8_t data;
    int ret;

    if ((buf == 0) && (len > 0U)) {
        return HAL_ERR_PARAM;
    }

    for (i = 0U; i < len; ++i) {
        ret = usart_hal_recv_byte(USARTx, &data);
        if (ret < 0) {
            return HAL_ERR_PARAM;
        }
        if (ret == 0) {
            return (i == 0U) ? HAL_ERR_TIMEOUT : HAL_OK;
        }
        buf[i] = data;
    }

    return HAL_OK;
}

hal_status_t usart_hal_get_status(hal_usart_id_t USARTx, usart_hal_status_t *status)
{
    if ((status == 0) || (usart_instance_valid(USARTx) == 0U)) {
        return HAL_ERR_PARAM;
    }
    status->rx_overflow = 0U;
    status->frame_error = 0U;
    status->overrun_error = 0U;
    return HAL_OK;
}

void usart_hal_flush_rx(hal_usart_id_t USARTx)
{
    uint8_t data;

    if (usart_instance_valid(USARTx) == 0U) {
        return;
    }
    while (usart_hal_recv_byte(USARTx, &data) == 1) {
    }
}

void usart_hal_enable_rx_irq(hal_usart_id_t USARTx)
{
    (void)USARTx;
}

void usart_hal_irq_handler(hal_usart_id_t USARTx)
{
    (void)USARTx;
}
