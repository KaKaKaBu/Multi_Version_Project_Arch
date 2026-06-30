#if HAL_USART_ENABLE_DMA
#include "usart_hal_private.h"
#include "hal_common.h"
#include "misc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_rcc.h"
#else
typedef char usart_hal_dma_disabled[-1];
#endif

#if HAL_USART_ENABLE_DMA

#define USART_HAL_DMA_TX_BUF_SIZE 256U

typedef struct usart_hal_dma_ctx {
    USART_TypeDef *usart;
    DMA_Channel_TypeDef *channel;
    uint8_t buffer[USART_HAL_DMA_TX_BUF_SIZE];
} usart_hal_dma_ctx_t;

extern usart_hal_runtime_t *usart_hal_runtime_get(USART_TypeDef *USARTx);

static usart_hal_dma_ctx_t usart_hal_dma_ctx[3];

static usart_hal_dma_ctx_t *usart_hal_dma_ctx_get(USART_TypeDef *USARTx)
{
    if (USARTx == USART1) {
        return &usart_hal_dma_ctx[0];
    }
    if (USARTx == USART2) {
        return &usart_hal_dma_ctx[1];
    }
    return &usart_hal_dma_ctx[2];
}

static uint32_t usart_hal_dma_peripheral_addr(USART_TypeDef *USARTx)
{
    return (uint32_t)&USARTx->DR;
}

static IRQn_Type usart_hal_dma_irqn(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel2) {
        return DMA1_Channel2_IRQn;
    }
    if (channel == DMA1_Channel3) {
        return DMA1_Channel3_IRQn;
    }
    if (channel == DMA1_Channel4) {
        return DMA1_Channel4_IRQn;
    }
    if (channel == DMA1_Channel5) {
        return DMA1_Channel5_IRQn;
    }
    if (channel == DMA1_Channel6) {
        return DMA1_Channel6_IRQn;
    }
    return DMA1_Channel7_IRQn;
}

static uint32_t usart_hal_dma_tc_flag(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel1) {
        return DMA1_IT_TC1;
    }
    if (channel == DMA1_Channel2) {
        return DMA1_IT_TC2;
    }
    if (channel == DMA1_Channel3) {
        return DMA1_IT_TC3;
    }
    if (channel == DMA1_Channel4) {
        return DMA1_IT_TC4;
    }
    if (channel == DMA1_Channel5) {
        return DMA1_IT_TC5;
    }
    if (channel == DMA1_Channel6) {
        return DMA1_IT_TC6;
    }
    return DMA1_IT_TC7;
}

static uint8_t usart_hal_dma_poll_not_busy(void *ctx)
{
    usart_hal_runtime_t *runtime = (usart_hal_runtime_t *)ctx;

    if (runtime == 0) {
        return 0U;
    }

    return (runtime->tx_dma_busy == 0U) ? 1U : 0U;
}

hal_status_t usart_hal_dma_setup(USART_TypeDef *USARTx, DMA_Channel_TypeDef *channel)
{
    DMA_InitTypeDef dma;
    usart_hal_dma_ctx_t *ctx = usart_hal_dma_ctx_get(USARTx);
    NVIC_InitTypeDef nvic;

    if ((USARTx == 0) || (channel == 0)) {
        return HAL_ERR_PARAM;
    }

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    ctx->usart = USARTx;
    ctx->channel = channel;

    DMA_DeInit(channel);
    DMA_StructInit(&dma);
    dma.DMA_PeripheralBaseAddr = usart_hal_dma_peripheral_addr(USARTx);
    dma.DMA_MemoryBaseAddr = (uint32_t)ctx->buffer;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = 0U;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_Medium;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(channel, &dma);

    nvic.NVIC_IRQChannel = usart_hal_dma_irqn(channel);
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 2U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    DMA_ITConfig(channel, DMA_IT_TC, ENABLE);
    USART_DMACmd(USARTx, USART_DMAReq_Tx, ENABLE);

    return HAL_OK;
}

hal_status_t usart_hal_dma_send(USART_TypeDef *USARTx, const uint8_t *data, uint16_t len, uint32_t timeout_us)
{
    usart_hal_runtime_t *runtime = usart_hal_runtime_get(USARTx);
    usart_hal_dma_ctx_t *ctx = usart_hal_dma_ctx_get(USARTx);
    uint16_t i;
    hal_status_t status;

    if ((data == 0) || (len == 0U)) {
        return HAL_ERR_PARAM;
    }

    if (len > USART_HAL_DMA_TX_BUF_SIZE) {
        return HAL_ERR_PARAM;
    }

    status = hal_wait_flag_us(usart_hal_dma_poll_not_busy, runtime, timeout_us);
    if (status != HAL_OK) {
        return HAL_ERR_BUSY;
    }

    for (i = 0U; i < len; ++i) {
        ctx->buffer[i] = data[i];
    }

    runtime->tx_dma_busy = 1U;
    DMA_Cmd(ctx->channel, DISABLE);
    ctx->channel->CMAR = (uint32_t)ctx->buffer;
    ctx->channel->CNDTR = len;
    DMA_ClearITPendingBit(usart_hal_dma_tc_flag(ctx->channel));
    DMA_Cmd(ctx->channel, ENABLE);

    return HAL_OK;
}

void usart_hal_dma_irq_handler(DMA_Channel_TypeDef *channel)
{
    usart_hal_runtime_t *runtime;
    uint8_t i;

    if (DMA_GetITStatus(usart_hal_dma_tc_flag(channel)) == RESET) {
        return;
    }

    DMA_ClearITPendingBit(usart_hal_dma_tc_flag(channel));

    for (i = 0U; i < 3U; ++i) {
        if (usart_hal_dma_ctx[i].channel == channel) {
            runtime = usart_hal_runtime_get(usart_hal_dma_ctx[i].usart);
            DMA_Cmd(channel, DISABLE);
            runtime->tx_dma_busy = 0U;
            return;
        }
    }
}

#endif
