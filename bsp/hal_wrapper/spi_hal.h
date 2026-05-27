#ifndef SPI_HAL_H
#define SPI_HAL_H

#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "hal_status.h"

#if HAL_SPI_USE_SOFT || HAL_SPI_USE_HW

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_HAL_BUS_MAX 2U
#define SPI_HAL_DEFAULT_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US

#if HAL_SPI_USE_SOFT

typedef struct spi_hal_config {
    hal_pin_t sck;
    hal_pin_t mosi;
    hal_pin_t miso;
    uint8_t cpol;
    uint8_t cpha;
    uint16_t half_period_us;
} spi_hal_config_t;

#elif HAL_SPI_USE_HW

#include "stm32f10x_spi.h"
#if HAL_SPI_ENABLE_DMA
#include "stm32f10x_dma.h"
#endif

typedef struct spi_hal_config {
    SPI_TypeDef *instance;
    hal_pin_t sck;
    hal_pin_t mosi;
    hal_pin_t miso;
    gpio_hal_remap_t remap;
    uint16_t prescaler;
    uint8_t cpol;
    uint8_t cpha;
    uint32_t timeout_us;
#if HAL_SPI_ENABLE_DMA
    DMA_Channel_TypeDef *tx_dma_channel;
    DMA_Channel_TypeDef *rx_dma_channel;
#endif
} spi_hal_config_t;

#endif

hal_status_t spi_hal_init(uint8_t bus_id, const spi_hal_config_t *cfg);
hal_status_t spi_hal_write(uint8_t bus_id, const uint8_t *data, uint16_t len);
hal_status_t spi_hal_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len);
uint8_t spi_hal_transfer_byte(uint8_t bus_id, uint8_t data);

#if HAL_SPI_USE_HW && HAL_SPI_ENABLE_DMA
hal_status_t spi_hal_dma_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_us);
void spi_hal_dma_irq_handler(DMA_Channel_TypeDef *channel);
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
