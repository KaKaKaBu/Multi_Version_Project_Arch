#include "usart_hal.h"
#include "usart_hal_private.h"
#include "hal_common.h"
#include "irq_event.h"
#include "debug_log.h"
#include "misc.h"
#include "stm32f10x_rcc.h"

#define USART_HAL_INSTANCE_COUNT 3U
#define USART_HAL_MAX_RX_BUF_SIZE 128U

typedef struct usart_hal_flag_ctx {
    USART_TypeDef *instance;
    uint16_t flag;
} usart_hal_flag_ctx_t;

static usart_hal_runtime_t usart_hal_runtime[USART_HAL_INSTANCE_COUNT];

static void usart_hal_clock_enable(USART_TypeDef *USARTx)
{
    if (USARTx == USART1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    } else if (USARTx == USART2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    } else if (USARTx == USART3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    }
}

static uint8_t usart_hal_index(USART_TypeDef *USARTx)
{
    if (USARTx == USART1) {
        return 0U;
    }
    if (USARTx == USART2) {
        return 1U;
    }
    return 2U;
}

usart_hal_runtime_t *usart_hal_runtime_get(USART_TypeDef *USARTx)
{
    return &usart_hal_runtime[usart_hal_index(USARTx)];
}

static IRQn_Type usart_hal_irqn(USART_TypeDef *USARTx)
{
    if (USARTx == USART1) {
        return USART1_IRQn;
    }
    if (USARTx == USART2) {
        return USART2_IRQn;
    }
    return USART3_IRQn;
}

static irq_event_source_t usart_hal_rx_source(USART_TypeDef *USARTx)
{
    if (USARTx == USART1) {
        return IRQ_EVENT_SOURCE_USART1_RX;
    }
    if (USARTx == USART2) {
        return IRQ_EVENT_SOURCE_USART2_RX;
    }
    return IRQ_EVENT_SOURCE_USART3_RX;
}

static uint8_t usart_hal_poll_flag(void *ctx)
{
    const usart_hal_flag_ctx_t *flag_ctx = (const usart_hal_flag_ctx_t *)ctx;

    if (flag_ctx == 0) {
        return 0U;
    }

    return (USART_GetFlagStatus(flag_ctx->instance, flag_ctx->flag) != RESET) ? 1U : 0U;
}

static void usart_hal_reset_runtime(usart_hal_runtime_t *runtime, const usart_hal_config_t *cfg, uint16_t rx_capacity)
{
    runtime->rx_head = 0U;
    runtime->rx_tail = 0U;
    runtime->rx_capacity = rx_capacity;
    runtime->rx_overflow = 0U;
    runtime->frame_error = 0U;
    runtime->overrun_error = 0U;
    runtime->tx_timeout_us = (cfg->tx_timeout_us == 0U) ? USART_HAL_DEFAULT_TX_TIMEOUT_US : cfg->tx_timeout_us;
    runtime->tx_mode = cfg->tx_mode;
#if HAL_USART_ENABLE_DMA
    runtime->tx_dma_channel = cfg->tx_dma_channel;
    runtime->tx_dma_busy = 0U;
    runtime->tx_dma_error = 0U;
#endif
    runtime->configured = 1U;
}

hal_status_t usart_hal_init(const usart_hal_config_t *cfg)
{
    USART_InitTypeDef usart;
    usart_hal_runtime_t *runtime;
    uint16_t rx_capacity;

    if ((cfg == 0) || (cfg->instance == 0)) {
        return HAL_ERR_PARAM;
    }

    runtime = usart_hal_runtime_get(cfg->instance);
    rx_capacity = cfg->rx_buf_size;
    if ((rx_capacity == 0U) || (rx_capacity > USART_HAL_MAX_RX_BUF_SIZE)) {
        rx_capacity = USART_HAL_DEFAULT_RX_BUF_SIZE;
    }

    gpio_hal_apply_remap(cfg->remap);
    gpio_hal_config_pin(&cfg->tx);
    gpio_hal_config_pin(&cfg->rx);

    usart_hal_clock_enable(cfg->instance);
    USART_StructInit(&usart);
    usart.USART_BaudRate = cfg->baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(cfg->instance, &usart);
    USART_Cmd(cfg->instance, ENABLE);

    usart_hal_reset_runtime(runtime, cfg, rx_capacity);

#if HAL_USART_ENABLE_DMA
    if ((cfg->tx_mode == USART_HAL_TX_MODE_DMA) && (cfg->tx_dma_channel != 0)) {
        hal_status_t status = usart_hal_dma_setup(cfg->instance, cfg->tx_dma_channel);
        if (status != HAL_OK) {
            return status;
        }
    }
#endif

    return HAL_OK;
}

hal_status_t usart_hal_send_byte(USART_TypeDef *USARTx, uint8_t data)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);
    usart_hal_flag_ctx_t ctx;
    hal_status_t status;

    ctx.instance = USARTx;
    ctx.flag = USART_FLAG_TXE;
    status = hal_wait_flag_us(usart_hal_poll_flag, &ctx, runtime->tx_timeout_us);
    if (status != HAL_OK) {
        return status;
    }

    USART_SendData(USARTx, data);
    return HAL_OK;
}

static hal_status_t usart_hal_send_buffer_irq(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    hal_status_t status;

    for (i = 0; i < len; ++i) {
        status = usart_hal_send_byte(USARTx, data[i]);
        if (status != HAL_OK) {
            return status;
        }
    }

    return HAL_OK;
}

hal_status_t usart_hal_send_buffer(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len)
{
    if (data == 0) {
        return HAL_ERR_PARAM;
    }

#if HAL_USART_ENABLE_DMA
    {
        usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);

        if ((runtime->tx_mode == USART_HAL_TX_MODE_DMA) && (runtime->tx_dma_channel != 0)) {
            return usart_hal_dma_send(USARTx, data, len, runtime->tx_timeout_us);
        }
    }
#endif

    return usart_hal_send_buffer_irq(USARTx, data, len);
}

int usart_hal_recv_byte(USART_TypeDef *USARTx, uint8_t *data)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);

    if (data == 0) {
        return -1;
    }

    if (runtime->rx_head == runtime->rx_tail) {
        if (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET) {
            return 0;
        }
        *data = (uint8_t)USART_ReceiveData(USARTx);
        return 1;
    }

    *data = runtime->rx_buffer[runtime->rx_tail];
    runtime->rx_tail = (uint16_t)((runtime->rx_tail + 1U) % runtime->rx_capacity);
    return 1;
}

uint16_t usart_hal_rx_available(USART_TypeDef *USARTx)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);

    if (runtime->rx_head >= runtime->rx_tail) {
        return (uint16_t)(runtime->rx_head - runtime->rx_tail);
    }

    return (uint16_t)(runtime->rx_capacity - runtime->rx_tail + runtime->rx_head);
}

hal_status_t usart_hal_read(USART_TypeDef *USARTx, uint8_t *buf, uint16_t len)
{
    uint16_t i;
    int received;

    if (buf == 0) {
        return HAL_ERR_PARAM;
    }

    for (i = 0; i < len; ++i) {
        received = usart_hal_recv_byte(USARTx, &buf[i]);
        if (received <= 0) {
            return (i > 0U) ? HAL_OK : HAL_ERR_IO;
        }
    }

    return HAL_OK;
}

hal_status_t usart_hal_get_status(USART_TypeDef *USARTx, usart_hal_status_t *status)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);

    if (status == 0) {
        return HAL_ERR_PARAM;
    }

    status->rx_overflow = runtime->rx_overflow;
    status->frame_error = runtime->frame_error;
    status->overrun_error = runtime->overrun_error;
#if HAL_USART_ENABLE_DMA
    status->tx_dma_error = runtime->tx_dma_error;
    runtime->tx_dma_error = 0U;
#endif

    runtime->rx_overflow = 0U;
    runtime->frame_error = 0U;
    runtime->overrun_error = 0U;

    return HAL_OK;
}

void usart_hal_flush_rx(USART_TypeDef *USARTx)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);

    runtime->rx_head = 0U;
    runtime->rx_tail = 0U;
    runtime->rx_overflow = 0U;

    while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) != RESET) {
        (void)USART_ReceiveData(USARTx);
    }
}

void usart_hal_enable_rx_irq(USART_TypeDef *USARTx)
{
    NVIC_InitTypeDef nvic;

    nvic.NVIC_IRQChannel = usart_hal_irqn(USARTx);
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
    if (USARTx == USART3) {
        DEBUG_LOG_USART("[USART] USART3 RX IRQ enabled\r\n");
    }
}

void usart_hal_irq_handler(USART_TypeDef *USARTx)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);
    uint16_t next;
    uint8_t data;

    while (USART_GetITStatus(USARTx, USART_IT_RXNE) != RESET) {
        data = (uint8_t)USART_ReceiveData(USARTx);
        next = (uint16_t)((runtime->rx_head + 1U) % runtime->rx_capacity);
        if (next != runtime->rx_tail) {
            runtime->rx_buffer[runtime->rx_head] = data;
            runtime->rx_head = next;
        } else {
            runtime->rx_overflow = 1U;
        }
        irq_event_post_from_isr(usart_hal_rx_source(USARTx), data);
    }

    if (USART_GetFlagStatus(USARTx, USART_FLAG_ORE) != RESET) {
        runtime->overrun_error = 1U;
        (void)USART_ReceiveData(USARTx);
    }

    if (USART_GetFlagStatus(USARTx, USART_FLAG_FE) != RESET) {
        runtime->frame_error = 1U;
        USART_ClearFlag(USARTx, USART_FLAG_FE);
    }
}
