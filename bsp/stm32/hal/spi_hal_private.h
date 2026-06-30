#ifndef SPI_HAL_PRIVATE_H
#define SPI_HAL_PRIVATE_H

#include "hal_features.h"

#if HAL_SPI_USE_HW

#include "stm32f10x_spi.h"
#include "stm32f10x_dma.h"

typedef struct spi_hal_runtime {
    SPI_TypeDef *instance;
    uint32_t timeout_us;
    uint8_t configured;
#if HAL_SPI_ENABLE_DMA
    uint8_t dma_busy;
    DMA_Channel_TypeDef *tx_dma_channel;
    DMA_Channel_TypeDef *rx_dma_channel;
#endif
} spi_hal_runtime_t;

spi_hal_runtime_t *spi_hal_runtime_get_by_bus(uint8_t bus_id);
spi_hal_runtime_t *spi_hal_runtime_get_by_instance(SPI_TypeDef *SPIx);

void spi_hal_dma_irq_handler(DMA_Channel_TypeDef *channel);

#endif

#endif
