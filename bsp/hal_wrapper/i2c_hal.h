/**
 * @file i2c_hal.h
 * @brief I2C master HAL: init, register read/write, and bus recovery.
 */

#ifndef I2C_HAL_H
#define I2C_HAL_H

#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "hal_status.h"
#include "stm32f10x_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Default I2C transaction timeout in microseconds. */
#define I2C_HAL_DEFAULT_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US

/** @brief I2C instance, bus speed, SCL/SDA pins, remap, and operation timeout. */
typedef struct i2c_hal_config {
    I2C_TypeDef *instance;
    uint32_t speed_hz;
    hal_pin_t scl;
    hal_pin_t sda;
    gpio_hal_remap_t remap;
    uint32_t timeout_us;
} i2c_hal_config_t;

/**
 * @brief Initializes I2C GPIO, peripheral, and runtime state.
 * @param cfg Configuration (instance must be I2C1 or I2C2).
 * @return HAL_OK or HAL_ERR_PARAM.
 */
hal_status_t i2c_hal_init(const i2c_hal_config_t *cfg);

/**
 * @brief Writes @p len data bytes starting at register @p reg to 7-bit @p address.
 * @param I2Cx I2C peripheral instance.
 * @param address 7-bit slave address (shifted format as used by SPL).
 * @param reg Register address to write first.
 * @param data Payload bytes (may be NULL only when @p len is 0).
 * @param len Number of data bytes after the register byte.
 * @return HAL_OK, HAL_ERR_PARAM, HAL_ERR_NACK, HAL_ERR_TIMEOUT, or bus busy timeout.
 */
hal_status_t i2c_hal_write(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t len);

/**
 * @brief Reads @p len bytes from register @p reg on 7-bit @p address (repeated START).
 * @param I2Cx I2C peripheral instance.
 * @param address 7-bit slave address.
 * @param reg Register address to write before read phase.
 * @param data Receive buffer (must not be NULL).
 * @param len Number of bytes to read (must be > 0).
 * @return HAL_OK, HAL_ERR_PARAM, HAL_ERR_NACK, or HAL_ERR_TIMEOUT.
 */
hal_status_t i2c_hal_read(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg,
                          uint8_t *data, uint16_t len);

/**
 * @brief Recovers a stuck I2C bus via GPIO clocking and re-inits if previously configured.
 * @param I2Cx I2C peripheral instance.
 * @param scl SCL pin descriptor for bit-bang recovery.
 * @param sda SDA pin descriptor for bit-bang recovery.
 * @return HAL_OK, HAL_ERR_PARAM, or error from re-init.
 */
hal_status_t i2c_hal_bus_recover(I2C_TypeDef *I2Cx, hal_pin_t scl, hal_pin_t sda);

#ifdef __cplusplus
}
#endif

#endif
