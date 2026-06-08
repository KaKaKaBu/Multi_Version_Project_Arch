/**
 * @file usart_hal.h
 * @brief USART initialization, blocking TX/RX, IRQ ring buffer, and optional DMA TX.
 */

#ifndef USART_HAL_H
#define USART_HAL_H

#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "hal_status.h"
#include "stm32f10x_usart.h"

#if HAL_USART_ENABLE_DMA
#include "stm32f10x_dma.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Default USART IRQ RX ring buffer size in bytes. */
#define USART_HAL_DEFAULT_RX_BUF_SIZE 128U
/** @brief Default USART blocking TX wait timeout in microseconds. */
#define USART_HAL_DEFAULT_TX_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US

/** @brief USART transmit path: IRQ polling or DMA (when enabled). */
typedef enum usart_hal_tx_mode {
    USART_HAL_TX_MODE_IRQ = 0,
#if HAL_USART_ENABLE_DMA
    USART_HAL_TX_MODE_DMA
#endif
} usart_hal_tx_mode_t;

/** @brief USART instance, pins, baud rate, buffer sizes, and TX mode. */
typedef struct usart_hal_config {
    USART_TypeDef *instance;
    uint32_t baudrate;
    hal_pin_t tx;
    hal_pin_t rx;
    gpio_hal_remap_t remap;
    uint16_t rx_buf_size;
    uint32_t tx_timeout_us;
    usart_hal_tx_mode_t tx_mode;
#if HAL_USART_ENABLE_DMA
    DMA_Channel_TypeDef *tx_dma_channel;
#endif
} usart_hal_config_t;

/** @brief Latched error flags cleared on read by usart_hal_get_status(). */
typedef struct usart_hal_status {
    uint8_t rx_overflow;
    uint8_t frame_error;
    uint8_t overrun_error;
#if HAL_USART_ENABLE_DMA
    uint8_t tx_dma_error;
#endif
} usart_hal_status_t;

/**
 * @brief Initializes USART GPIO, peripheral, runtime state, and optional DMA TX.
 * @param cfg Configuration (instance and pins must be valid).
 * @return HAL_OK on success, HAL_ERR_PARAM on invalid @p cfg, or DMA setup error.
 */
hal_status_t usart_hal_init(const usart_hal_config_t *cfg);

/**
 * @brief Transmits one byte, waiting for TXE up to the configured timeout.
 * @param USARTx USART peripheral instance.
 * @param data Byte to send.
 * @return HAL_OK or HAL_ERR_TIMEOUT.
 */
hal_status_t usart_hal_send_byte(USART_TypeDef *USARTx, uint8_t data);

/**
 * @brief Transmits a buffer via IRQ polling or DMA (when configured).
 * @param USARTx USART peripheral instance.
 * @param data Buffer to transmit.
 * @param len Number of bytes.
 * @return HAL_OK, HAL_ERR_PARAM, or timeout/DMA error.
 */
hal_status_t usart_hal_send_buffer(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len);

/**
 * @brief Receives one byte from the IRQ ring buffer or hardware FIFO.
 * @param USARTx USART peripheral instance.
 * @param data Output byte (must not be NULL).
 * @return 1 if a byte was read, 0 if none available, -1 on invalid @p data.
 */
int usart_hal_recv_byte(USART_TypeDef *USARTx, uint8_t *data);

/**
 * @brief Returns the number of unread bytes in the RX ring buffer.
 * @param USARTx USART peripheral instance.
 * @return Count of buffered RX bytes.
 */
uint16_t usart_hal_rx_available(USART_TypeDef *USARTx);

/**
 * @brief Reads exactly @p len bytes into @p buf (partial read returns HAL_OK if any data).
 * @param USARTx USART peripheral instance.
 * @param buf Destination buffer.
 * @param len Number of bytes to read.
 * @return HAL_OK if at least one byte read, HAL_ERR_IO if none, HAL_ERR_PARAM if @p buf is NULL.
 */
hal_status_t usart_hal_read(USART_TypeDef *USARTx, uint8_t *buf, uint16_t len);

/**
 * @brief Reads and clears latched USART error flags for @p USARTx.
 * @param USARTx USART peripheral instance.
 * @param status Output status structure (must not be NULL).
 * @return HAL_OK or HAL_ERR_PARAM.
 */
hal_status_t usart_hal_get_status(USART_TypeDef *USARTx, usart_hal_status_t *status);

/**
 * @brief Discards the RX ring buffer and drains the hardware RX FIFO.
 * @param USARTx USART peripheral instance.
 */
void usart_hal_flush_rx(USART_TypeDef *USARTx);

/**
 * @brief Enables RXNE interrupt and NVIC for @p USARTx.
 * @param USARTx USART peripheral instance.
 */
void usart_hal_enable_rx_irq(USART_TypeDef *USARTx);

/**
 * @brief IRQ handler: fills RX ring buffer and posts irq_event on each byte.
 * @param USARTx USART peripheral instance.
 */
void usart_hal_irq_handler(USART_TypeDef *USARTx);

#if HAL_USART_ENABLE_DMA
/**
 * @brief DMA TX completion/error IRQ handler for a USART DMA channel.
 * @param channel DMA channel that triggered the interrupt.
 */
void usart_hal_dma_irq_handler(DMA_Channel_TypeDef *channel);
#endif

#ifdef __cplusplus
}
#endif

#endif
