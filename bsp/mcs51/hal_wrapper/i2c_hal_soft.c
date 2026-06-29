#include "i2c_hal.h"
#include "mcs51_memory.h"

typedef struct i2c_hal_runtime {
    hal_pin_t scl;
    hal_pin_t sda;
    uint16_t half_period_us;
    uint8_t configured;
} i2c_hal_runtime_t;

static MCS51_XDATA i2c_hal_runtime_t i2c_runtime;

static void i2c_delay(void)
{
    hal_delay_us(i2c_runtime.half_period_us);
}

static void i2c_scl(uint8_t level)
{
    gpio_hal_write(i2c_runtime.scl.port, i2c_runtime.scl.pin, level);
}

static void i2c_sda(uint8_t level)
{
    gpio_hal_write(i2c_runtime.sda.port, i2c_runtime.sda.pin, level);
}

static uint8_t i2c_sda_read(void)
{
    gpio_hal_write(i2c_runtime.sda.port, i2c_runtime.sda.pin, 1U);
    return gpio_hal_read(i2c_runtime.sda.port, i2c_runtime.sda.pin);
}

static void i2c_start(void)
{
    i2c_sda(1U);
    i2c_scl(1U);
    i2c_delay();
    i2c_sda(0U);
    i2c_delay();
    i2c_scl(0U);
}

static void i2c_stop(void)
{
    i2c_sda(0U);
    i2c_delay();
    i2c_scl(1U);
    i2c_delay();
    i2c_sda(1U);
}

static void i2c_write_bit(uint8_t bit)
{
    i2c_sda(bit);
    i2c_delay();
    i2c_scl(1U);
    i2c_delay();
    i2c_scl(0U);
}

static uint8_t i2c_read_bit(void)
{
    uint8_t bit;
    i2c_sda(1U);
    i2c_delay();
    i2c_scl(1U);
    i2c_delay();
    bit = i2c_sda_read();
    i2c_scl(0U);
    return bit;
}

static hal_status_t i2c_write_byte(uint8_t data, uint8_t check_ack)
{
    uint8_t i;
    uint8_t ack;

    for (i = 0U; i < 8U; ++i) {
        i2c_write_bit((data & 0x80U) != 0U);
        data = (uint8_t)(data << 1U);
    }

    if (check_ack == 0U) {
        return HAL_OK;
    }

    ack = i2c_read_bit();
    return (ack == 0U) ? HAL_OK : HAL_ERR_NACK;
}

static uint8_t i2c_read_byte(uint8_t send_nack)
{
    uint8_t i;
    uint8_t value = 0U;

    for (i = 0U; i < 8U; ++i) {
        value = (uint8_t)(value << 1U);
        if (i2c_read_bit() != 0U) {
            value |= 1U;
        }
    }

    i2c_write_bit(send_nack);
    return value;
}

hal_status_t i2c_hal_init(const i2c_hal_config_t *cfg)
{
    uint32_t period_us;

    if (cfg == 0) {
        return HAL_ERR_PARAM;
    }

    i2c_runtime.scl = cfg->scl;
    i2c_runtime.sda = cfg->sda;
    period_us = 1000000UL / ((cfg->speed_hz == 0U) ? 100000UL : cfg->speed_hz);
    if (period_us < 10U) {
        period_us = 10U;
    }
    i2c_runtime.half_period_us = (uint16_t)(period_us / 2U);
    i2c_runtime.configured = 1U;
    gpio_hal_config_pin(&i2c_runtime.scl);
    gpio_hal_config_pin(&i2c_runtime.sda);
    i2c_scl(1U);
    i2c_sda(1U);
    return HAL_OK;
}

hal_status_t i2c_hal_write(i2c_hal_instance_t I2Cx, uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t len)
{
    uint16_t i;
    hal_status_t status;

    (void)I2Cx;
    if ((i2c_runtime.configured == 0U) || ((data == 0) && (len > 0U))) {
        return HAL_ERR_PARAM;
    }

    i2c_start();
    status = i2c_write_byte((uint8_t)(address & 0xFEU), 1U);
    if (status == HAL_OK) {
        status = i2c_write_byte(reg, 1U);
    }
    for (i = 0U; (status == HAL_OK) && (i < len); ++i) {
        status = i2c_write_byte(data[i], 1U);
    }
    i2c_stop();
    return status;
}

hal_status_t i2c_hal_read(i2c_hal_instance_t I2Cx, uint8_t address, uint8_t reg,
                          uint8_t *data, uint16_t len)
{
    uint16_t i;
    hal_status_t status;

    (void)I2Cx;
    if ((i2c_runtime.configured == 0U) || (data == 0) || (len == 0U)) {
        return HAL_ERR_PARAM;
    }

    i2c_start();
    status = i2c_write_byte((uint8_t)(address & 0xFEU), 1U);
    if (status == HAL_OK) {
        status = i2c_write_byte(reg, 1U);
    }
    if (status == HAL_OK) {
        i2c_start();
        status = i2c_write_byte((uint8_t)(address | 0x01U), 1U);
    }
    for (i = 0U; (status == HAL_OK) && (i < len); ++i) {
        data[i] = i2c_read_byte((i + 1U) == len);
    }
    i2c_stop();
    return status;
}

hal_status_t i2c_hal_bus_recover(i2c_hal_instance_t I2Cx, const hal_pin_t *scl, const hal_pin_t *sda)
{
    uint8_t i;

    (void)I2Cx;
    if ((scl == 0) || (sda == 0)) {
        return HAL_ERR_PARAM;
    }
    gpio_hal_config_pin(scl);
    gpio_hal_config_pin(sda);
    for (i = 0U; i < 9U; ++i) {
        gpio_hal_write(scl->port, scl->pin, 0U);
        hal_delay_us(5U);
        gpio_hal_write(scl->port, scl->pin, 1U);
        hal_delay_us(5U);
    }
    return HAL_OK;
}
