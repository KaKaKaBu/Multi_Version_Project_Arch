/**
 * @file usart_hal_private.h
 * @brief Internal USART runtime state and DMA helpers (included by usart_hal.c only).
 */

#ifndef USART_HAL_PRIVATE_H
#define USART_HAL_PRIVATE_H

#include "usart_hal.h"

/** @brief Per-instance RX ring buffer, error flags, and TX configuration. */
typedef struct usart_hal_runtime {
    volatile uint8_t rx_buffer[128U];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile uint16_t rx_capacity;
    volatile uint8_t rx_overflow;
    volatile uint8_t frame_error;
    volatile uint8_t overrun_error;
    uint32_t tx_timeout_us;
    usart_hal_tx_mode_t tx_mode;
#if HAL_USART_ENABLE_DMA
    DMA_Channel_TypeDef *tx_dma_channel;
    volatile uint8_t tx_dma_busy;
    volatile uint8_t tx_dma_error;
#endif
    uint8_t configured;
} usart_hal_runtime_t;

/**
 * @brief Returns runtime state for USART1, USART2, or USART3.
 * @param USARTx USART peripheral instance.
 * @return Pointer to the instance runtime block.
 */
usart_hal_runtime_t *usart_hal_runtime_get(USART_TypeDef *USARTx);

#if HAL_USART_ENABLE_DMA
/**
 * @brief Configures DMA TX for the given USART instance.
 * @param USARTx USART peripheral instance.
 * @param channel DMA channel bound to USART TX.
 * @return HAL_OK or an error status from DMA setup.
 */
hal_status_t usart_hal_dma_setup(USART_TypeDef *USARTx, DMA_Channel_TypeDef *channel);

/**
 * @brief Transmits @p len bytes via DMA with timeout.
 * @param USARTx USART peripheral instance.
 * @param data Buffer to transmit.
 * @param len Number of bytes.
 * @param timeout_us Maximum wait time in microseconds.
 * @return HAL_OK or timeout/DMA error.
 */
hal_status_t usart_hal_dma_send(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len, uint32_t timeout_us);
#endif

#endif
