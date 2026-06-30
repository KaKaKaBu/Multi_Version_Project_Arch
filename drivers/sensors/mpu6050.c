#include "imu_if.h"
#include "i2c_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

#define MPU6050_REG_SMPLRT_DIV    0x19U
#define MPU6050_REG_CONFIG        0x1AU
#define MPU6050_REG_GYRO_CONFIG   0x1BU
#define MPU6050_REG_ACCEL_CONFIG  0x1CU
#define MPU6050_REG_ACCEL_XOUT_H  0x3BU
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_PWR_MGMT_2    0x6CU

static imu_sample_t mpu6050_sample_cache;
static const i2c_device_config_t *mpu6050_config;

static void mpu6050_i2c_init(void)
{
    static uint8_t initialized;

    if (initialized != 0U) {
        return;
    }

    if (mpu6050_config == 0) {
        return;
    }

    i2c_hal_config_t cfg = {
        mpu6050_config->instance,
        mpu6050_config->speed_hz,
        mpu6050_config->scl,
        mpu6050_config->sda,
        mpu6050_config->remap,
        I2C_HAL_DEFAULT_TIMEOUT_US
    };

    (void)i2c_hal_init(&cfg);
    initialized = 1U;
}

static void mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    if (mpu6050_config != 0) {
        (void)i2c_hal_write(mpu6050_config->instance, mpu6050_config->address, reg, &data, 1U);
    }
}

static void mpu6050_refresh_sample(void)
{
    uint8_t buf[14];
    int16_t value;

    if ((mpu6050_config == 0) ||
        (i2c_hal_read(mpu6050_config->instance, mpu6050_config->address, MPU6050_REG_ACCEL_XOUT_H,
                      buf, sizeof(buf)) != HAL_OK)) {
        return;
    }

    value = (int16_t)(((int16_t)buf[0] << 8) | buf[1]);
    mpu6050_sample_cache.ax = value;
    value = (int16_t)(((int16_t)buf[2] << 8) | buf[3]);
    mpu6050_sample_cache.ay = value;
    value = (int16_t)(((int16_t)buf[4] << 8) | buf[5]);
    mpu6050_sample_cache.az = value;
    value = (int16_t)(((int16_t)buf[8] << 8) | buf[9]);
    mpu6050_sample_cache.gx = value;
    value = (int16_t)(((int16_t)buf[10] << 8) | buf[11]);
    mpu6050_sample_cache.gy = value;
    value = (int16_t)(((int16_t)buf[12] << 8) | buf[13]);
    mpu6050_sample_cache.gz = value;
}

static void mpu6050_init(const void *config)
{
    mpu6050_config = (const i2c_device_config_t *)config;
    if (mpu6050_config == 0) {
        return;
    }
    mpu6050_i2c_init();

    mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x01U);
    mpu6050_write_reg(MPU6050_REG_PWR_MGMT_2, 0x00U);
    mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x09U);
    mpu6050_write_reg(MPU6050_REG_CONFIG, 0x06U);
    mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, 0x18U);
    mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, 0x18U);

    mpu6050_sample_cache.ax = 0;
    mpu6050_sample_cache.ay = 0;
    mpu6050_sample_cache.az = 0;
    mpu6050_sample_cache.gx = 0;
    mpu6050_sample_cache.gy = 0;
    mpu6050_sample_cache.gz = 0;
}

static void mpu6050_read_sample(imu_sample_t *sample)
{
    if (sample == 0) {
        return;
    }

    mpu6050_refresh_sample();
    *sample = mpu6050_sample_cache;
}

static const imu_sensor_t mpu6050_drv = {
    "mpu6050",
    mpu6050_init,
    mpu6050_read_sample
};

REGISTER_DRIVER(IMU_SENSOR, mpu6050_drv);
