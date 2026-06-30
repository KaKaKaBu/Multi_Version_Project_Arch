#include "pressure_if.h"
#include "i2c_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

#define BMP180_REG_CALIB_BASE 0xAAU
#define BMP180_REG_CTRL       0xF4U
#define BMP180_REG_OUT_MSB    0xF6U
#define BMP180_CMD_READ_TEMP  0x2EU
#define BMP180_CMD_READ_PRESS 0x34U
#define BMP180_OSS            0U

typedef struct bmp180_cal {
    int16_t ac1;
    int16_t ac2;
    int16_t ac3;
    uint16_t ac4;
    uint16_t ac5;
    uint16_t ac6;
    int16_t b1;
    int16_t b2;
    int16_t mb;
    int16_t mc;
    int16_t md;
} bmp180_cal_t;

static bmp180_cal_t bmp180_cal;
static float bmp180_pressure_cache;
static float bmp180_temperature_cache;
static const bmp180_driver_config_t *bmp180_config;

static void bmp180_i2c_init(void)
{
    static uint8_t initialized;

    if (initialized != 0U) {
        return;
    }

    if (bmp180_config == 0) {
        return;
    }

    i2c_hal_config_t cfg = {
        bmp180_config->i2c.instance,
        bmp180_config->i2c.speed_hz,
        bmp180_config->i2c.scl,
        bmp180_config->i2c.sda,
        bmp180_config->i2c.remap,
        I2C_HAL_DEFAULT_TIMEOUT_US
    };

    (void)i2c_hal_init(&cfg);
    initialized = 1U;
}

static int16_t bmp180_read_int16(uint8_t reg)
{
    uint8_t buf[2];
    int16_t value;

    if ((bmp180_config == 0) ||
        (i2c_hal_read(bmp180_config->i2c.instance, bmp180_config->i2c.address, reg, buf, 2U) != HAL_OK)) {
        return 0;
    }

    value = (int16_t)(((int16_t)buf[0] << 8) | buf[1]);
    return value;
}

static uint16_t bmp180_read_uint16(uint8_t reg)
{
    return (uint16_t)bmp180_read_int16(reg);
}

static int32_t bmp180_read_ut(void)
{
    uint8_t cmd = BMP180_CMD_READ_TEMP;

    if ((bmp180_config == 0) ||
        (i2c_hal_write(bmp180_config->i2c.instance, bmp180_config->i2c.address, BMP180_REG_CTRL, &cmd, 1U) != HAL_OK)) {
        return 0;
    }

    hal_delay_us(5000U);
    return (int32_t)bmp180_read_int16(BMP180_REG_OUT_MSB);
}

static int32_t bmp180_read_up(void)
{
    uint8_t cmd = BMP180_CMD_READ_PRESS;

    if ((bmp180_config == 0) ||
        (i2c_hal_write(bmp180_config->i2c.instance, bmp180_config->i2c.address, BMP180_REG_CTRL, &cmd, 1U) != HAL_OK)) {
        return 0;
    }

    hal_delay_us(5000U);
    return (int32_t)bmp180_read_int16(BMP180_REG_OUT_MSB);
}

static void bmp180_update_result(void)
{
    int32_t ut = bmp180_read_ut();
    int32_t up = bmp180_read_up();
    int32_t x1;
    int32_t x2;
    int32_t x3;
    int32_t b3;
    int32_t b5;
    int32_t b6;
    uint32_t b4;
    int32_t b7;
    int32_t pressure_pa;
    int32_t temperature_raw;

    x1 = ((ut - (int32_t)bmp180_cal.ac6) * (int32_t)bmp180_cal.ac5) >> 15;
    x2 = (((int32_t)bmp180_cal.mc) << 11) / (x1 + (int32_t)bmp180_cal.md);
    b5 = x1 + x2;
    temperature_raw = (b5 + 8) >> 4;

    b6 = b5 - 4000;
    x1 = (((int32_t)bmp180_cal.b2) * ((b6 * b6) >> 12)) >> 11;
    x2 = (((int32_t)bmp180_cal.ac2) * b6) >> 11;
    x3 = x1 + x2;
    b3 = (((((int32_t)bmp180_cal.ac1) * 4 + x3) << BMP180_OSS) + 2) / 4;
    x1 = ((int32_t)bmp180_cal.ac3 * b6) >> 13;
    x2 = (((int32_t)bmp180_cal.b1) * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = ((uint32_t)bmp180_cal.ac4 * (uint32_t)(x3 + 32768)) >> 15;
    b7 = ((up - b3) * (50000 >> BMP180_OSS));

    if ((uint32_t)b7 < 0x80000000UL) {
        pressure_pa = (b7 * 2) / (int32_t)b4;
    } else {
        pressure_pa = (b7 / (int32_t)b4) * 2;
    }

    x1 = (pressure_pa >> 8) * (pressure_pa >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * pressure_pa) >> 16;
    pressure_pa = pressure_pa + ((x1 + x2 + 3791) >> 4) + bmp180_config->pressure_revise;

    bmp180_temperature_cache = (float)temperature_raw / 10.0f;
    bmp180_pressure_cache = (float)pressure_pa;
}

static void bmp180_init(const void *config)
{
    bmp180_config = (const bmp180_driver_config_t *)config;
    if (bmp180_config == 0) {
        return;
    }
    bmp180_i2c_init();

    bmp180_cal.ac1 = bmp180_read_int16(BMP180_REG_CALIB_BASE);
    bmp180_cal.ac2 = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 2U));
    bmp180_cal.ac3 = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 4U));
    bmp180_cal.ac4 = bmp180_read_uint16((uint8_t)(BMP180_REG_CALIB_BASE + 6U));
    bmp180_cal.ac5 = bmp180_read_uint16((uint8_t)(BMP180_REG_CALIB_BASE + 8U));
    bmp180_cal.ac6 = bmp180_read_uint16((uint8_t)(BMP180_REG_CALIB_BASE + 10U));
    bmp180_cal.b1 = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 12U));
    bmp180_cal.b2 = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 14U));
    bmp180_cal.mb = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 16U));
    bmp180_cal.mc = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 18U));
    bmp180_cal.md = bmp180_read_int16((uint8_t)(BMP180_REG_CALIB_BASE + 20U));

    bmp180_pressure_cache = 101325.0f;
    bmp180_temperature_cache = 25.0f;
}

static float bmp180_read_pressure_pa(void)
{
    bmp180_update_result();
    return bmp180_pressure_cache;
}

static float bmp180_read_temperature(void)
{
    bmp180_update_result();
    return bmp180_temperature_cache;
}

static const pressure_sensor_t bmp180_drv = {
    "bmp180",
    bmp180_init,
    bmp180_read_pressure_pa,
    bmp180_read_temperature
};

REGISTER_DRIVER(PRESSURE_SENSOR, bmp180_drv);
