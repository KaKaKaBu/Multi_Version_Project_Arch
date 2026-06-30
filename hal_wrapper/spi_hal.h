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
#define SPI_HAL_PRESCALER_DEFAULT 0U
#define SPI_HAL_PRESCALER_2 2U
#define SPI_HAL_PRESCALER_4 4U
#define SPI_HAL_PRESCALER_8 8U
#define SPI_HAL_PRESCALER_16 16U
#define SPI_HAL_PRESCALER_32 32U
#define SPI_HAL_PRESCALER_64 64U
#define SPI_HAL_PRESCALER_128 128U
#define SPI_HAL_PRESCALER_256 256U

typedef struct spi_hal_config {
#if HAL_SPI_USE_HW
    hal_spi_id_t instance;
#endif
    hal_pin_t sck;
    hal_pin_t mosi;
    hal_pin_t miso;
#if HAL_SPI_USE_HW
    gpio_hal_remap_t remap;
    uint16_t prescaler;
#endif
    uint8_t cpol;
    uint8_t cpha;
#if HAL_SPI_USE_SOFT
    uint16_t half_period_us;
#endif
#if HAL_SPI_USE_HW
    uint32_t timeout_us;
#endif
#if HAL_SPI_USE_HW && HAL_SPI_ENABLE_DMA
    hal_dma_channel_id_t tx_dma_channel;
    hal_dma_channel_id_t rx_dma_channel;
#endif
} spi_hal_config_t;

hal_status_t spi_hal_init(uint8_t bus_id, const spi_hal_config_t *cfg);
hal_status_t spi_hal_write(uint8_t bus_id, const uint8_t *data, uint16_t len);
hal_status_t spi_hal_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len);
uint8_t spi_hal_transfer_byte(uint8_t bus_id, uint8_t data);

#if HAL_SPI_USE_HW && HAL_SPI_ENABLE_DMA
hal_status_t spi_hal_dma_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_us);
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
