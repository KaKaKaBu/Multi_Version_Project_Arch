#include "usart_hal.h"

hal_status_t usart_hal_init(const usart_hal_config_t *cfg)
{
    (void)cfg;
    return HAL_OK;
}

hal_status_t usart_hal_send_byte(usart_hal_instance_t USARTx, uint8_t data)
{
    (void)USARTx;
    (void)data;
    return HAL_OK;
}

hal_status_t usart_hal_send_buffer(usart_hal_instance_t USARTx, const uint8_t *data, uint16_t len)
{
    (void)USARTx;
    return (data == 0 && len > 0U) ? HAL_ERR_PARAM : HAL_OK;
}

int usart_hal_recv_byte(usart_hal_instance_t USARTx, uint8_t *data)
{
    (void)USARTx;
    if (data == 0) {
        return -1;
    }
    return 0;
}

uint16_t usart_hal_rx_available(usart_hal_instance_t USARTx)
{
    (void)USARTx;
    return 0U;
}

hal_status_t usart_hal_read(usart_hal_instance_t USARTx, uint8_t *buf, uint16_t len)
{
    (void)USARTx;
    (void)len;
    return (buf == 0) ? HAL_ERR_PARAM : HAL_ERR_IO;
}

hal_status_t usart_hal_get_status(usart_hal_instance_t USARTx, usart_hal_status_t *status)
{
    (void)USARTx;
    if (status == 0) {
        return HAL_ERR_PARAM;
    }
    status->rx_overflow = 0U;
    status->frame_error = 0U;
    status->overrun_error = 0U;
    return HAL_OK;
}

void usart_hal_flush_rx(usart_hal_instance_t USARTx)
{
    (void)USARTx;
}

void usart_hal_enable_rx_irq(usart_hal_instance_t USARTx)
{
    (void)USARTx;
}

void usart_hal_irq_handler(usart_hal_instance_t USARTx)
{
    (void)USARTx;
}
