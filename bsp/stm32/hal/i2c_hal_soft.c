#include "i2c_hal.h"
#include "hal_common.h"
#include "stm32f1_hal_map.h"

#if !HAL_I2C_USE_SOFT
typedef char i2c_hal_soft_disabled[-1];
#else

typedef struct i2c_hal_runtime {
    hal_pin_t scl;
    hal_pin_t sda;
    uint16_t half_period_us;
    uint32_t timeout_us;
    uint8_t configured;
} i2c_hal_runtime_t;

static i2c_hal_runtime_t i2c_hal_runtime[2];

static i2c_hal_runtime_t *i2c_hal_runtime_get(hal_i2c_id_t I2Cx)
{
    if (I2Cx == HAL_I2C_ID_1) {
        return &i2c_hal_runtime[0];
    }
    if (I2Cx == HAL_I2C_ID_2) {
        return &i2c_hal_runtime[1];
    }
    return 0;
}

static void i2c_soft_delay(i2c_hal_runtime_t *runtime)
{
    hal_delay_us(runtime->half_period_us);
}

static void i2c_soft_scl(i2c_hal_runtime_t *runtime, uint8_t level)
{
    gpio_hal_write(runtime->scl.port, runtime->scl.pin, level);
}

static void i2c_soft_sda_out(i2c_hal_runtime_t *runtime, uint8_t level)
{
    hal_pin_t sda = runtime->sda;

    sda.mode = GPIO_HAL_MODE_OUT_OD;
    gpio_hal_config_pin(&sda);
    gpio_hal_write(sda.port, sda.pin, level);
}

static void i2c_soft_sda_release(i2c_hal_runtime_t *runtime)
{
    hal_pin_t sda = runtime->sda;

    sda.mode = GPIO_HAL_MODE_IN_FLOATING;
    gpio_hal_config_pin(&sda);
}

static uint8_t i2c_soft_sda_read(i2c_hal_runtime_t *runtime)
{
    i2c_soft_sda_release(runtime);
    return gpio_hal_read(runtime->sda.port, runtime->sda.pin);
}

static void i2c_soft_start(i2c_hal_runtime_t *runtime)
{
    i2c_soft_sda_out(runtime, 1U);
    i2c_soft_scl(runtime, 1U);
    i2c_soft_delay(runtime);
    i2c_soft_sda_out(runtime, 0U);
    i2c_soft_delay(runtime);
    i2c_soft_scl(runtime, 0U);
    i2c_soft_delay(runtime);
}

static void i2c_soft_stop(i2c_hal_runtime_t *runtime)
{
    i2c_soft_sda_out(runtime, 0U);
    i2c_soft_delay(runtime);
    i2c_soft_scl(runtime, 1U);
    i2c_soft_delay(runtime);
    i2c_soft_sda_out(runtime, 1U);
    i2c_soft_delay(runtime);
}

static hal_status_t i2c_soft_write_bit(i2c_hal_runtime_t *runtime, uint8_t bit)
{
    i2c_soft_sda_out(runtime, bit);
    i2c_soft_delay(runtime);
    i2c_soft_scl(runtime, 1U);
    i2c_soft_delay(runtime);
    i2c_soft_scl(runtime, 0U);
    i2c_soft_delay(runtime);
    return HAL_OK;
}

static hal_status_t i2c_soft_read_bit(i2c_hal_runtime_t *runtime, uint8_t *bit)
{
    uint8_t value;

    i2c_soft_sda_release(runtime);
    i2c_soft_delay(runtime);
    i2c_soft_scl(runtime, 1U);
    i2c_soft_delay(runtime);
    value = i2c_soft_sda_read(runtime);
    i2c_soft_scl(runtime, 0U);
    i2c_soft_delay(runtime);
    *bit = value;
    return HAL_OK;
}

static hal_status_t i2c_soft_write_byte(i2c_hal_runtime_t *runtime, uint8_t data, uint8_t check_ack)
{
    uint8_t i;
    uint8_t ack;

    for (i = 0U; i < 8U; ++i) {
        if (i2c_soft_write_bit(runtime, (uint8_t)((data & 0x80U) != 0U)) != HAL_OK) {
            return HAL_ERR_IO;
        }
        data = (uint8_t)(data << 1U);
    }

    if (check_ack == 0U) {
        return HAL_OK;
    }

    if (i2c_soft_read_bit(runtime, &ack) != HAL_OK) {
        return HAL_ERR_IO;
    }

    return (ack == 0U) ? HAL_OK : HAL_ERR_NACK;
}

static hal_status_t i2c_soft_read_byte(i2c_hal_runtime_t *runtime, uint8_t *data, uint8_t send_nack)
{
    uint8_t i;
    uint8_t bit;
    uint8_t value = 0U;

    for (i = 0U; i < 8U; ++i) {
        value = (uint8_t)(value << 1U);
        if (i2c_soft_read_bit(runtime, &bit) != HAL_OK) {
            return HAL_ERR_IO;
        }
        if (bit != 0U) {
            value |= 1U;
        }
    }

    if (i2c_soft_write_bit(runtime, send_nack) != HAL_OK) {
        return HAL_ERR_IO;
    }

    *data = value;
    return HAL_OK;
}

static uint16_t i2c_soft_half_period_us(uint32_t speed_hz)
{
    uint32_t period_us;

    if (speed_hz == 0U) {
        speed_hz = 100000U;
    }

    period_us = 1000000U / speed_hz;
    if (period_us < 2U) {
        period_us = 2U;
    }

    return (uint16_t)(period_us / 2U);
}

hal_status_t i2c_hal_init(const i2c_hal_config_t *cfg)
{
    i2c_hal_runtime_t *runtime;
    hal_pin_t scl_od;
    hal_pin_t sda_od;

    if ((cfg == 0) || (cfg->instance == 0U)) {
        return HAL_ERR_PARAM;
    }

    runtime = i2c_hal_runtime_get(cfg->instance);
    if (runtime == 0) {
        return HAL_ERR_PARAM;
    }

    scl_od = cfg->scl;
    sda_od = cfg->sda;
    scl_od.mode = GPIO_HAL_MODE_OUT_OD;
    sda_od.mode = GPIO_HAL_MODE_OUT_OD;
    gpio_hal_config_pin(&scl_od);
    gpio_hal_config_pin(&sda_od);
    gpio_hal_write(scl_od.port, scl_od.pin, 1U);
    gpio_hal_write(sda_od.port, sda_od.pin, 1U);

    runtime->scl = scl_od;
    runtime->sda = sda_od;
    runtime->half_period_us = i2c_soft_half_period_us(cfg->speed_hz);
    runtime->timeout_us = (cfg->timeout_us == 0U) ? I2C_HAL_DEFAULT_TIMEOUT_US : cfg->timeout_us;
    runtime->configured = 1U;

    return HAL_OK;
}

hal_status_t i2c_hal_write(hal_i2c_id_t I2Cx, uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t len)
{
    i2c_hal_runtime_t *runtime = i2c_hal_runtime_get(I2Cx);
    uint16_t i;
    hal_status_t status;

    if ((runtime == 0) || (runtime->configured == 0U)) {
        return HAL_ERR_PARAM;
    }

    if ((data == 0) && (len > 0U)) {
        return HAL_ERR_PARAM;
    }

    i2c_soft_start(runtime);
    status = i2c_soft_write_byte(runtime, (uint8_t)(address & 0xFEU), 1U);
    if (status != HAL_OK) {
        i2c_soft_stop(runtime);
        return status;
    }

    status = i2c_soft_write_byte(runtime, reg, 1U);
    if (status != HAL_OK) {
        i2c_soft_stop(runtime);
        return status;
    }

    for (i = 0U; i < len; ++i) {
        status = i2c_soft_write_byte(runtime, data[i], 1U);
        if (status != HAL_OK) {
            i2c_soft_stop(runtime);
            return status;
        }
    }

    i2c_soft_stop(runtime);
    return HAL_OK;
}

hal_status_t i2c_hal_read(hal_i2c_id_t I2Cx, uint8_t address, uint8_t reg,
                          uint8_t *data, uint16_t len)
{
    i2c_hal_runtime_t *runtime = i2c_hal_runtime_get(I2Cx);
    uint16_t i;
    hal_status_t status;

    if ((runtime == 0) || (runtime->configured == 0U)) {
        return HAL_ERR_PARAM;
    }

    if ((data == 0) || (len == 0U)) {
        return HAL_ERR_PARAM;
    }

    i2c_soft_start(runtime);
    status = i2c_soft_write_byte(runtime, (uint8_t)(address & 0xFEU), 1U);
    if (status != HAL_OK) {
        i2c_soft_stop(runtime);
        return status;
    }

    status = i2c_soft_write_byte(runtime, reg, 1U);
    if (status != HAL_OK) {
        i2c_soft_stop(runtime);
        return status;
    }

    i2c_soft_start(runtime);
    status = i2c_soft_write_byte(runtime, (uint8_t)(address | 0x01U), 1U);
    if (status != HAL_OK) {
        i2c_soft_stop(runtime);
        return status;
    }

    for (i = 0U; i < len; ++i) {
        status = i2c_soft_read_byte(runtime, &data[i], (uint8_t)(i == (len - 1U)));
        if (status != HAL_OK) {
            i2c_soft_stop(runtime);
            return status;
        }
    }

    i2c_soft_stop(runtime);
    return HAL_OK;
}

hal_status_t i2c_hal_bus_recover(hal_i2c_id_t I2Cx, const hal_pin_t *scl, const hal_pin_t *sda)
{
    i2c_hal_runtime_t *runtime = i2c_hal_runtime_get(I2Cx);
    hal_pin_t scl_od;
    hal_pin_t sda_od;
    uint8_t i;

    if ((I2Cx == 0U) || (scl == 0) || (sda == 0)) {
        return HAL_ERR_PARAM;
    }

    scl_od = *scl;
    sda_od = *sda;

    scl_od.mode = GPIO_HAL_MODE_OUT_OD;
    sda_od.mode = GPIO_HAL_MODE_OUT_OD;
    gpio_hal_config_pin(&scl_od);
    gpio_hal_config_pin(&sda_od);
    gpio_hal_write(sda_od.port, sda_od.pin, 1U);
    gpio_hal_write(scl_od.port, scl_od.pin, 1U);

    for (i = 0U; i < 9U; ++i) {
        gpio_hal_write(scl_od.port, scl_od.pin, 0U);
        hal_delay_us(5U);
        gpio_hal_write(scl_od.port, scl_od.pin, 1U);
        hal_delay_us(5U);
    }

    gpio_hal_write(sda_od.port, sda_od.pin, 0U);
    hal_delay_us(5U);
    gpio_hal_write(scl_od.port, scl_od.pin, 1U);
    hal_delay_us(5U);
    gpio_hal_write(sda_od.port, sda_od.pin, 1U);
    hal_delay_us(5U);

    if ((runtime != 0) && (runtime->configured != 0U)) {
        i2c_hal_config_t cfg;

        cfg.instance = I2Cx;
        cfg.speed_hz = (runtime->half_period_us == 0U) ? 100000U : (1000000U / (runtime->half_period_us * 2U));
        cfg.scl = runtime->scl;
        cfg.sda = runtime->sda;
        cfg.remap = GPIO_HAL_REMAP_NONE;
        cfg.timeout_us = runtime->timeout_us;
        return i2c_hal_init(&cfg);
    }

    return HAL_OK;
}

#endif
