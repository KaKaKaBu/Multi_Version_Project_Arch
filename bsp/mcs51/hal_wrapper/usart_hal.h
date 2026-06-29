#ifndef USART_HAL_H
#define USART_HAL_H

#include <stdint.h>
#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "hal_status.h"

typedef uint8_t usart_hal_instance_t;

#define USART_HAL_DEFAULT_RX_BUF_SIZE 64U
#define USART_HAL_DEFAULT_TX_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US

typedef enum usart_hal_tx_mode {
    USART_HAL_TX_MODE_IRQ = 0
} usart_hal_tx_mode_t;

typedef struct usart_hal_config {
    usart_hal_instance_t instance;
    uint32_t baudrate;
    hal_pin_t tx;
    hal_pin_t rx;
    gpio_hal_remap_t remap;
    uint16_t rx_buf_size;
    uint32_t tx_timeout_us;
    usart_hal_tx_mode_t tx_mode;
} usart_hal_config_t;

typedef struct usart_hal_status {
    uint8_t rx_overflow;
    uint8_t frame_error;
    uint8_t overrun_error;
} usart_hal_status_t;

hal_status_t usart_hal_init(const usart_hal_config_t *cfg);
hal_status_t usart_hal_send_byte(usart_hal_instance_t USARTx, uint8_t data);
hal_status_t usart_hal_send_buffer(usart_hal_instance_t USARTx, const uint8_t *data, uint16_t len);
int usart_hal_recv_byte(usart_hal_instance_t USARTx, uint8_t *data);
uint16_t usart_hal_rx_available(usart_hal_instance_t USARTx);
hal_status_t usart_hal_read(usart_hal_instance_t USARTx, uint8_t *buf, uint16_t len);
hal_status_t usart_hal_get_status(usart_hal_instance_t USARTx, usart_hal_status_t *status);
void usart_hal_flush_rx(usart_hal_instance_t USARTx);
void usart_hal_enable_rx_irq(usart_hal_instance_t USARTx);
void usart_hal_irq_handler(usart_hal_instance_t USARTx);

#endif
