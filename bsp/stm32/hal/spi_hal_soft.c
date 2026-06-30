#include "spi_hal.h"

#if !HAL_SPI_USE_SOFT
typedef char spi_hal_soft_disabled[-1];
#else

#include "hal_common.h"

typedef struct spi_hal_runtime {
    hal_pin_t sck;
    hal_pin_t mosi;
    hal_pin_t miso;
    uint8_t cpol;
    uint8_t cpha;
    uint16_t half_period_us;
    uint8_t configured;
} spi_hal_runtime_t;

static spi_hal_runtime_t spi_hal_runtime[SPI_HAL_BUS_MAX];

static spi_hal_runtime_t *spi_hal_runtime_get(uint8_t bus_id)
{
    if (bus_id >= SPI_HAL_BUS_MAX) {
        return 0;
    }

    return &spi_hal_runtime[bus_id];
}

static void spi_hal_delay(spi_hal_runtime_t *runtime)
{
    hal_delay_us(runtime->half_period_us);
}

static void spi_hal_sck(spi_hal_runtime_t *runtime, uint8_t level)
{
    uint8_t value = level;

    if (runtime->cpol != 0U) {
        value = (level == 0U) ? 1U : 0U;
    }

    gpio_hal_write(runtime->sck.port, runtime->sck.pin, value);
}

static void spi_hal_mosi(spi_hal_runtime_t *runtime, uint8_t level)
{
    gpio_hal_write(runtime->mosi.port, runtime->mosi.pin, level);
}

static uint8_t spi_hal_miso_read(spi_hal_runtime_t *runtime)
{
    return gpio_hal_read(runtime->miso.port, runtime->miso.pin);
}

hal_status_t spi_hal_init(uint8_t bus_id, const spi_hal_config_t *cfg)
{
    spi_hal_runtime_t *runtime;
    hal_pin_t miso_in;

    if ((cfg == 0) || (bus_id >= SPI_HAL_BUS_MAX)) {
        return HAL_ERR_PARAM;
    }

    runtime = spi_hal_runtime_get(bus_id);
    if (runtime == 0) {
        return HAL_ERR_PARAM;
    }

    gpio_hal_config_pin(&cfg->sck);
    gpio_hal_config_pin(&cfg->mosi);
    miso_in = cfg->miso;
    miso_in.mode = GPIO_HAL_MODE_IN_FLOATING;
    gpio_hal_config_pin(&miso_in);

    spi_hal_sck(runtime, 0U);
    spi_hal_mosi(runtime, 0U);

    runtime->sck = cfg->sck;
    runtime->mosi = cfg->mosi;
    runtime->miso = miso_in;
    runtime->cpol = cfg->cpol;
    runtime->cpha = cfg->cpha;
    runtime->half_period_us = (cfg->half_period_us == 0U) ? 1U : cfg->half_period_us;
    runtime->configured = 1U;

    return HAL_OK;
}

uint8_t spi_hal_transfer_byte(uint8_t bus_id, uint8_t data)
{
    spi_hal_runtime_t *runtime = spi_hal_runtime_get(bus_id);
    uint8_t i;
    uint8_t sample;
    uint8_t rx = 0U;

    if ((runtime == 0) || (runtime->configured == 0U)) {
        return 0U;
    }

    for (i = 0U; i < 8U; ++i) {
        if (runtime->cpha == 0U) {
            spi_hal_mosi(runtime, (uint8_t)((data & 0x80U) != 0U));
            spi_hal_delay(runtime);
            spi_hal_sck(runtime, 1U);
            spi_hal_delay(runtime);
            sample = spi_hal_miso_read(runtime);
            spi_hal_sck(runtime, 0U);
        } else {
            spi_hal_sck(runtime, 1U);
            spi_hal_mosi(runtime, (uint8_t)((data & 0x80U) != 0U));
            spi_hal_delay(runtime);
            sample = spi_hal_miso_read(runtime);
            spi_hal_sck(runtime, 0U);
            spi_hal_delay(runtime);
        }

        rx = (uint8_t)(rx << 1U);
        if (sample != 0U) {
            rx |= 1U;
        }
        data = (uint8_t)(data << 1U);
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
        (void)spi_hal_transfer_byte(bus_id, data[i]);
    }

    return HAL_OK;
}

hal_status_t spi_hal_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    uint16_t i;
    uint8_t value;

    if ((tx == 0) || (rx == 0)) {
        return HAL_ERR_PARAM;
    }

    for (i = 0U; i < len; ++i) {
        value = spi_hal_transfer_byte(bus_id, tx[i]);
        rx[i] = value;
    }

    return HAL_OK;
}

#endif
