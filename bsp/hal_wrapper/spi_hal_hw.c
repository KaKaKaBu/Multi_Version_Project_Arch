#include "spi_hal.h"

#if !HAL_SPI_USE_HW
typedef char spi_hal_hw_disabled[-1];
#else

#include "spi_hal_private.h"
#include "hal_common.h"
#include "stm32f10x_rcc.h"

static spi_hal_runtime_t spi_hal_runtime[SPI_HAL_BUS_MAX];

spi_hal_runtime_t *spi_hal_runtime_get_by_bus(uint8_t bus_id)
{
    if (bus_id >= SPI_HAL_BUS_MAX) {
        return 0;
    }

    return &spi_hal_runtime[bus_id];
}

spi_hal_runtime_t *spi_hal_runtime_get_by_instance(SPI_TypeDef *SPIx)
{
    uint8_t i;

    for (i = 0U; i < SPI_HAL_BUS_MAX; ++i) {
        if ((spi_hal_runtime[i].configured != 0U) && (spi_hal_runtime[i].instance == SPIx)) {
            return &spi_hal_runtime[i];
        }
    }

    return 0;
}

static void spi_hal_clock_enable(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    } else if (SPIx == SPI2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    }
}

static uint16_t spi_hal_prescaler_value(uint16_t prescaler)
{
    if (prescaler == 0U) {
        return SPI_BaudRatePrescaler_8;
    }

    return prescaler;
}

static void spi_hal_apply_cpol_cpha(SPI_InitTypeDef *spi, uint8_t cpol, uint8_t cpha)
{
    spi->SPI_CPOL = (cpol != 0U) ? SPI_CPOL_High : SPI_CPOL_Low;
    spi->SPI_CPHA = (cpha != 0U) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
}

typedef struct spi_hal_flag_ctx {
    SPI_TypeDef *instance;
    uint16_t flag;
} spi_hal_flag_ctx_t;

static uint8_t spi_hal_poll_flag(void *ctx)
{
    const spi_hal_flag_ctx_t *flag_ctx = (const spi_hal_flag_ctx_t *)ctx;

    if (flag_ctx == 0) {
        return 0U;
    }

    return (SPI_I2S_GetFlagStatus(flag_ctx->instance, flag_ctx->flag) != RESET) ? 1U : 0U;
}

static hal_status_t spi_hal_wait_flag(SPI_TypeDef *SPIx, uint16_t flag, uint32_t timeout_us)
{
    spi_hal_flag_ctx_t ctx;

    ctx.instance = SPIx;
    ctx.flag = flag;
    return hal_wait_flag_us(spi_hal_poll_flag, &ctx, timeout_us);
}

static hal_status_t spi_hal_exchange_byte(spi_hal_runtime_t *runtime, uint8_t tx, uint8_t *rx)
{
    hal_status_t status;

    status = spi_hal_wait_flag(runtime->instance, SPI_I2S_FLAG_TXE, runtime->timeout_us);
    if (status != HAL_OK) {
        return status;
    }

    SPI_I2S_SendData(runtime->instance, tx);

    status = spi_hal_wait_flag(runtime->instance, SPI_I2S_FLAG_RXNE, runtime->timeout_us);
    if (status != HAL_OK) {
        return status;
    }

    if (rx != 0) {
        *rx = (uint8_t)SPI_I2S_ReceiveData(runtime->instance);
    } else {
        (void)SPI_I2S_ReceiveData(runtime->instance);
    }

    return HAL_OK;
}

hal_status_t spi_hal_init(uint8_t bus_id, const spi_hal_config_t *cfg)
{
    SPI_InitTypeDef spi;
    spi_hal_runtime_t *runtime;

    if ((cfg == 0) || (cfg->instance == 0) || (bus_id >= SPI_HAL_BUS_MAX)) {
        return HAL_ERR_PARAM;
    }

    runtime = spi_hal_runtime_get_by_bus(bus_id);
    if (runtime == 0) {
        return HAL_ERR_PARAM;
    }

    gpio_hal_apply_remap(cfg->remap);
    gpio_hal_config_pin(&cfg->sck);
    gpio_hal_config_pin(&cfg->mosi);
    gpio_hal_config_pin(&cfg->miso);

    spi_hal_clock_enable(cfg->instance);
    SPI_I2S_DeInit(cfg->instance);
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = spi_hal_prescaler_value(cfg->prescaler);
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    spi_hal_apply_cpol_cpha(&spi, cfg->cpol, cfg->cpha);
    SPI_Init(cfg->instance, &spi);
    SPI_Cmd(cfg->instance, ENABLE);

    runtime->instance = cfg->instance;
    runtime->timeout_us = (cfg->timeout_us == 0U) ? SPI_HAL_DEFAULT_TIMEOUT_US : cfg->timeout_us;
    runtime->configured = 1U;
#if HAL_SPI_ENABLE_DMA
    runtime->dma_busy = 0U;
    runtime->tx_dma_channel = cfg->tx_dma_channel;
    runtime->rx_dma_channel = cfg->rx_dma_channel;
#endif

    return HAL_OK;
}

uint8_t spi_hal_transfer_byte(uint8_t bus_id, uint8_t data)
{
    spi_hal_runtime_t *runtime = spi_hal_runtime_get_by_bus(bus_id);
    uint8_t rx = 0U;

    if ((runtime == 0) || (runtime->configured == 0U)) {
        return 0U;
    }

    if (spi_hal_exchange_byte(runtime, data, &rx) != HAL_OK) {
        return 0U;
    }

    return rx;
}

hal_status_t spi_hal_write(uint8_t bus_id, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (data == 0) {
        return HAL_ERR_PARAM;
    }

    for (i = 0U; i < len; ++i) {
        if (spi_hal_transfer_byte(bus_id, data[i]) == 0U) {
            return HAL_ERR_TIMEOUT;
        }
    }

    return HAL_OK;
}

hal_status_t spi_hal_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    spi_hal_runtime_t *runtime = spi_hal_runtime_get_by_bus(bus_id);
    uint16_t i;
    hal_status_t status;

    if ((tx == 0) || (rx == 0) || (runtime == 0) || (runtime->configured == 0U)) {
        return HAL_ERR_PARAM;
    }

    for (i = 0U; i < len; ++i) {
        status = spi_hal_exchange_byte(runtime, tx[i], &rx[i]);
        if (status != HAL_OK) {
            return status;
        }
    }

    return HAL_OK;
}

#endif
