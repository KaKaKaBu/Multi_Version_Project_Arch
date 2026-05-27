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

#define I2C_HAL_DEFAULT_TIMEOUT_US HAL_DEFAULT_TIMEOUT_US

typedef struct i2c_hal_config {
    I2C_TypeDef *instance;
    uint32_t speed_hz;
    hal_pin_t scl;
    hal_pin_t sda;
    gpio_hal_remap_t remap;
    uint32_t timeout_us;
} i2c_hal_config_t;

hal_status_t i2c_hal_init(const i2c_hal_config_t *cfg);
hal_status_t i2c_hal_write(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t len);
hal_status_t i2c_hal_read(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg,
                          uint8_t *data, uint16_t len);
hal_status_t i2c_hal_bus_recover(I2C_TypeDef *I2Cx, hal_pin_t scl, hal_pin_t sda);

#ifdef __cplusplus
}
#endif

#endif
