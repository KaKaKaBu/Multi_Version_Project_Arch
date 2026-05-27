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

#define USART_HAL_DEFAULT_RX_BUF_SIZE 128U
#define USART_HAL_DEFAULT_TX_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US

typedef enum usart_hal_tx_mode {
    USART_HAL_TX_MODE_IRQ = 0,
#if HAL_USART_ENABLE_DMA
    USART_HAL_TX_MODE_DMA
#endif
} usart_hal_tx_mode_t;

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

typedef struct usart_hal_status {
    uint8_t rx_overflow;
    uint8_t frame_error;
    uint8_t overrun_error;
#if HAL_USART_ENABLE_DMA
    uint8_t tx_dma_error;
#endif
} usart_hal_status_t;

hal_status_t usart_hal_init(const usart_hal_config_t *cfg);
hal_status_t usart_hal_send_byte(USART_TypeDef *USARTx, uint8_t data);
hal_status_t usart_hal_send_buffer(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len);
int usart_hal_recv_byte(USART_TypeDef *USARTx, uint8_t *data);
uint16_t usart_hal_rx_available(USART_TypeDef *USARTx);
hal_status_t usart_hal_read(USART_TypeDef *USARTx, uint8_t *buf, uint16_t len);
hal_status_t usart_hal_get_status(USART_TypeDef *USARTx, usart_hal_status_t *status);
void usart_hal_flush_rx(USART_TypeDef *USARTx);
void usart_hal_enable_rx_irq(USART_TypeDef *USARTx);
void usart_hal_irq_handler(USART_TypeDef *USARTx);

#if HAL_USART_ENABLE_DMA
void usart_hal_dma_irq_handler(DMA_Channel_TypeDef *channel);
#endif

#ifdef __cplusplus
}
#endif

#endif
