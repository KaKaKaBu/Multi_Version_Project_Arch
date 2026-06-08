/**
 * @file usart_hal.c
 * @brief USART HAL: init, blocking TX/RX, IRQ ring buffer, and optional DMA TX.
 */

#include "usart_hal.h"
#include "usart_hal_private.h"
#include "hal_common.h"
#include "irq_event.h"
#include "debug_log.h"
#include "misc.h"
#include "stm32f10x_rcc.h"

#define USART_HAL_INSTANCE_COUNT 3U
#define USART_HAL_MAX_RX_BUF_SIZE 128U

/** @brief Context for polling a USART flag via hal_wait_flag_us(). */
typedef struct usart_hal_flag_ctx {
    USART_TypeDef *instance;
    uint16_t flag;
} usart_hal_flag_ctx_t;

/** @brief Per-instance runtime state for USART1, USART2, and USART3. */
static usart_hal_runtime_t usart_hal_runtime[USART_HAL_INSTANCE_COUNT];

/**
 * @brief Enables RCC clock for USART1 (APB2) or USART2/3 (APB1).
 * @param USARTx USART peripheral instance.
 */
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

/**
 * @brief Maps USART instance to runtime array index (0=USART1, 1=USART2, 2=USART3).
 * @param USARTx USART peripheral instance.
 * @return Index into usart_hal_runtime[].
 */
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

/**
 * @brief Returns runtime state for USART1, USART2, or USART3.
 * @param USARTx USART peripheral instance.
 * @return Pointer to the instance runtime block.
 */
usart_hal_runtime_t *usart_hal_runtime_get(USART_TypeDef *USARTx)
{
    return &usart_hal_runtime[usart_hal_index(USARTx)];
}

/**
 * @brief Returns the NVIC IRQ number for the USART instance.
 * @param USARTx USART peripheral instance.
 * @return IRQn_Type for USART1, USART2, or USART3.
 */
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

/**
 * @brief Maps USART instance to irq_event RX source identifier.
 * @param USARTx USART peripheral instance.
 * @return irq_event_source_t for the instance RX line.
 */
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

/**
 * @brief Poll callback: returns 1 when the USART flag in ctx is set.
 * @param ctx Pointer to usart_hal_flag_ctx_t.
 * @return 1 if flag set, 0 otherwise.
 */
static uint8_t usart_hal_poll_flag(void *ctx)
{
    const usart_hal_flag_ctx_t *flag_ctx = (const usart_hal_flag_ctx_t *)ctx;

    if (flag_ctx == 0) {
        return 0U;
    }

    return (USART_GetFlagStatus(flag_ctx->instance, flag_ctx->flag) != RESET) ? 1U : 0U;
}

/**
 * @brief Resets RX ring buffer and copies TX settings from config.
 * @param runtime Runtime block to initialize.
 * @param cfg Source configuration.
 * @param rx_capacity Effective RX buffer capacity.
 */
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

/**
 * @brief Initializes USART GPIO, peripheral, runtime state, and optional DMA TX.
 * @param cfg Configuration (instance and pins must be valid).
 * @return HAL_OK on success, HAL_ERR_PARAM on invalid @p cfg, or DMA setup error.
 */
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

/**
 * @brief Transmits one byte, waiting for TXE up to the configured timeout.
 * @param USARTx USART peripheral instance.
 * @param data Byte to send.
 * @return HAL_OK or HAL_ERR_TIMEOUT.
 */
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

/**
 * @brief Transmits @p len bytes one byte at a time via IRQ polling.
 * @param USARTx USART peripheral instance.
 * @param data Buffer to transmit.
 * @param len Number of bytes.
 * @return HAL_OK or first TX error encountered.
 */
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

/**
 * @brief Transmits a buffer via IRQ polling or DMA (when configured).
 * @param USARTx USART peripheral instance.
 * @param data Buffer to transmit.
 * @param len Number of bytes.
 * @return HAL_OK, HAL_ERR_PARAM, or timeout/DMA error.
 */
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

/**
 * @brief Receives one byte from the IRQ ring buffer or hardware FIFO.
 * @param USARTx USART peripheral instance.
 * @param data Output byte (must not be NULL).
 * @return 1 if a byte was read, 0 if none available, -1 on invalid @p data.
 */
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

/**
 * @brief Returns the number of unread bytes in the RX ring buffer.
 * @param USARTx USART peripheral instance.
 * @return Count of buffered RX bytes.
 */
uint16_t usart_hal_rx_available(USART_TypeDef *USARTx)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);

    if (runtime->rx_head >= runtime->rx_tail) {
        return (uint16_t)(runtime->rx_head - runtime->rx_tail);
    }

    return (uint16_t)(runtime->rx_capacity - runtime->rx_tail + runtime->rx_head);
}

/**
 * @brief Reads exactly @p len bytes into @p buf (partial read returns HAL_OK if any data).
 * @param USARTx USART peripheral instance.
 * @param buf Destination buffer.
 * @param len Number of bytes to read.
 * @return HAL_OK if at least one byte read, HAL_ERR_IO if none, HAL_ERR_PARAM if @p buf is NULL.
 */
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

/**
 * @brief Reads and clears latched USART error flags for @p USARTx.
 * @param USARTx USART peripheral instance.
 * @param status Output status structure (must not be NULL).
 * @return HAL_OK or HAL_ERR_PARAM.
 */
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

/**
 * @brief Discards the RX ring buffer and drains the hardware RX FIFO.
 * @param USARTx USART peripheral instance.
 */
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

/**
 * @brief Enables RXNE interrupt and NVIC for @p USARTx.
 * @param USARTx USART peripheral instance.
 */
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

/**
 * @brief IRQ handler: fills RX ring buffer and posts irq_event on each byte.
 * @param USARTx USART peripheral instance.
 */
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
