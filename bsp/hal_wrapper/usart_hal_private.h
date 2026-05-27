#ifndef USART_HAL_PRIVATE_H
#define USART_HAL_PRIVATE_H

#include "usart_hal.h"

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

usart_hal_runtime_t *usart_hal_runtime_get(USART_TypeDef *USARTx);

#if HAL_USART_ENABLE_DMA
hal_status_t usart_hal_dma_setup(USART_TypeDef *USARTx, DMA_Channel_TypeDef *channel);
hal_status_t usart_hal_dma_send(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len, uint32_t timeout_us);
#endif

#endif
