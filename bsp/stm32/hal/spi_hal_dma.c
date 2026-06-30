#if HAL_SPI_ENABLE_DMA
#include "spi_hal.h"
#include "spi_hal_private.h"
#include "hal_common.h"
#include "misc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_rcc.h"
#else
typedef char spi_hal_dma_disabled[-1];
#endif

#if HAL_SPI_ENABLE_DMA

typedef struct spi_hal_dma_ctx {
    SPI_TypeDef *spi;
    DMA_Channel_TypeDef *tx_channel;
    DMA_Channel_TypeDef *rx_channel;
    const uint8_t *tx;
    uint8_t *rx;
    uint16_t len;
} spi_hal_dma_ctx_t;

static spi_hal_dma_ctx_t spi_hal_dma_ctx[SPI_HAL_BUS_MAX];

static IRQn_Type spi_hal_dma_irqn(DMA_Channel_TypeDef *channel)
{
    if (channel == DMA1_Channel1) {
        return DMA1_Channel1_IRQn;
    }
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

static uint32_t spi_hal_dma_tc_flag(DMA_Channel_TypeDef *channel)
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

static uint8_t spi_hal_dma_poll_not_busy(void *ctx)
{
    spi_hal_runtime_t *runtime = (spi_hal_runtime_t *)ctx;

    if (runtime == 0) {
        return 0U;
    }

    return (runtime->dma_busy == 0U) ? 1U : 0U;
}

static hal_status_t spi_hal_dma_setup_channel(DMA_Channel_TypeDef *channel, uint32_t peripheral_addr,
                                              uint32_t memory_addr, uint16_t direction, uint16_t len)
{
    DMA_InitTypeDef dma;

    DMA_DeInit(channel);
    DMA_StructInit(&dma);
    dma.DMA_PeripheralBaseAddr = peripheral_addr;
    dma.DMA_MemoryBaseAddr = memory_addr;
    dma.DMA_DIR = direction;
    dma.DMA_BufferSize = len;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(channel, &dma);

    return HAL_OK;
}

static hal_status_t spi_hal_dma_enable_irq(DMA_Channel_TypeDef *channel)
{
    NVIC_InitTypeDef nvic;

    nvic.NVIC_IRQChannel = spi_hal_dma_irqn(channel);
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    DMA_ITConfig(channel, DMA_IT_TC, ENABLE);

    return HAL_OK;
}

hal_status_t spi_hal_dma_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_us)
{
    spi_hal_runtime_t *runtime = spi_hal_runtime_get_by_bus(bus_id);
    spi_hal_dma_ctx_t *ctx;
    hal_status_t status;
    uint32_t dr_addr;

    if ((runtime == 0) || (runtime->configured == 0U) || (tx == 0) || (rx == 0) || (len == 0U)) {
        return HAL_ERR_PARAM;
    }

    if ((runtime->tx_dma_channel == 0) || (runtime->rx_dma_channel == 0)) {
        return HAL_ERR_PARAM;
    }

    status = hal_wait_flag_us(spi_hal_dma_poll_not_busy, runtime, timeout_us);
    if (status != HAL_OK) {
        return HAL_ERR_BUSY;
    }

    ctx = &spi_hal_dma_ctx[bus_id];
    ctx->spi = runtime->instance;
    ctx->tx_channel = runtime->tx_dma_channel;
    ctx->rx_channel = runtime->rx_dma_channel;
    ctx->tx = tx;
    ctx->rx = rx;
    ctx->len = len;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    dr_addr = (uint32_t)&runtime->instance->DR;

    (void)spi_hal_dma_setup_channel(runtime->tx_dma_channel, dr_addr, (uint32_t)tx, DMA_DIR_PeripheralDST, len);
    (void)spi_hal_dma_setup_channel(runtime->rx_dma_channel, dr_addr, (uint32_t)rx, DMA_DIR_PeripheralSRC, len);
    (void)spi_hal_dma_enable_irq(runtime->rx_dma_channel);

    runtime->dma_busy = 1U;
    SPI_I2S_DMACmd(runtime->instance, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

    DMA_ClearITPendingBit(spi_hal_dma_tc_flag(runtime->rx_dma_channel));
    DMA_Cmd(runtime->tx_dma_channel, ENABLE);
    DMA_Cmd(runtime->rx_dma_channel, ENABLE);

    status = hal_wait_flag_us(spi_hal_dma_poll_not_busy, runtime, timeout_us);
    if (status != HAL_OK) {
        DMA_Cmd(runtime->tx_dma_channel, DISABLE);
        DMA_Cmd(runtime->rx_dma_channel, DISABLE);
        SPI_I2S_DMACmd(runtime->instance, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);
        runtime->dma_busy = 0U;
        return HAL_ERR_TIMEOUT;
    }

    return HAL_OK;
}

void spi_hal_dma_irq_handler(DMA_Channel_TypeDef *channel)
{
    spi_hal_runtime_t *runtime;
    uint8_t i;

    if (DMA_GetITStatus(spi_hal_dma_tc_flag(channel)) == RESET) {
        return;
    }

    DMA_ClearITPendingBit(spi_hal_dma_tc_flag(channel));

    for (i = 0U; i < SPI_HAL_BUS_MAX; ++i) {
        runtime = spi_hal_runtime_get_by_bus(i);
        if ((runtime == 0) || (runtime->rx_dma_channel != channel)) {
            continue;
        }

        DMA_Cmd(runtime->tx_dma_channel, DISABLE);
        DMA_Cmd(runtime->rx_dma_channel, DISABLE);
        SPI_I2S_DMACmd(runtime->instance, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);
        runtime->dma_busy = 0U;
        return;
    }
}

#endif
