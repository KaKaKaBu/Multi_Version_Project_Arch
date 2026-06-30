#include "imu_if.h"
#include "i2c_hal.h"
#include "driver_configs.h"
#include "driver_core.h"
#include <math.h>

#define MPU6050_REG_SMPLRT_DIV    0x19U
#define MPU6050_REG_CONFIG        0x1AU
#define MPU6050_REG_GYRO_CONFIG   0x1BU
#define MPU6050_REG_ACCEL_CONFIG  0x1CU
#define MPU6050_REG_ACCEL_XOUT_H  0x3BU
#define MPU6050_REG_USER_CTRL     0x6AU
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_PWR_MGMT_2    0x6CU
#define MPU6050_REG_BANK_SEL      0x6DU
#define MPU6050_REG_MEM_START     0x6EU
#define MPU6050_REG_MEM_R_W       0x6FU
#define MPU6050_REG_FIFO_COUNTH   0x72U
#define MPU6050_REG_FIFO_R_W      0x74U

#define MPU6050_DMP_PACKET_SIZE   16U

static imu_sample_t mpu6050_sample_cache;
static const mpu6050_driver_config_t *mpu6050_ext_config;
static const i2c_device_config_t *mpu6050_config;
static uint8_t mpu6050_dmp_ready_flag;

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

static void mpu6050_write_mem(uint16_t address, const uint8_t *data, uint8_t len)
{
    uint8_t bank = (uint8_t)(address >> 8);
    uint8_t offset = (uint8_t)(address & 0xFFU);

    if ((mpu6050_config == 0) || (data == 0) || (len == 0U)) {
        return;
    }

    mpu6050_write_reg(MPU6050_REG_BANK_SEL, bank);
    mpu6050_write_reg(MPU6050_REG_MEM_START, offset);
    (void)i2c_hal_write(mpu6050_config->instance, mpu6050_config->address, MPU6050_REG_MEM_R_W, data, len);
}

static uint16_t mpu6050_fifo_count(void)
{
    uint8_t buf[2] = { 0U, 0U };

    if (mpu6050_config == 0) {
        return 0U;
    }

    if (i2c_hal_read(mpu6050_config->instance, mpu6050_config->address, MPU6050_REG_FIFO_COUNTH, buf, sizeof(buf)) != HAL_OK) {
        return 0U;
    }

    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

static void mpu6050_load_dmp_firmware(const mpu6050_dmp_firmware_t *firmware)
{
    uint16_t offset = 0U;

    if ((firmware == 0) || (firmware->data == 0) || (firmware->size == 0U)) {
        return;
    }

    while (offset < firmware->size) {
        uint8_t chunk = (uint8_t)(((uint16_t)(firmware->size - offset) > 16U) ? 16U : (uint16_t)(firmware->size - offset));
        mpu6050_write_mem(offset, &firmware->data[offset], chunk);
        offset = (uint16_t)(offset + chunk);
    }

    mpu6050_write_reg(MPU6050_REG_USER_CTRL, 0xC0U);
    mpu6050_write_reg(MPU6050_REG_USER_CTRL, 0x40U);
    mpu6050_dmp_ready_flag = 1U;
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
    mpu6050_dmp_ready_flag = 0U;
    mpu6050_ext_config = (const mpu6050_driver_config_t *)config;
    if (mpu6050_ext_config == 0) {
        return;
    }
    mpu6050_config = &mpu6050_ext_config->i2c;
    mpu6050_i2c_init();

    mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x01U);
    mpu6050_write_reg(MPU6050_REG_PWR_MGMT_2, 0x00U);
    mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x09U);
    mpu6050_write_reg(MPU6050_REG_CONFIG, 0x06U);
    mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, mpu6050_ext_config->gyro_config);
    mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, mpu6050_ext_config->accel_config);

    mpu6050_sample_cache.ax = 0;
    mpu6050_sample_cache.ay = 0;
    mpu6050_sample_cache.az = 0;
    mpu6050_sample_cache.gx = 0;
    mpu6050_sample_cache.gy = 0;
    mpu6050_sample_cache.gz = 0;

    if (mpu6050_ext_config->enable_dmp != 0U) {
        mpu6050_load_dmp_firmware(mpu6050_ext_config->dmp_firmware);
    }
}

static void mpu6050_read_sample(imu_sample_t *sample)
{
    if (sample == 0) {
        return;
    }

    mpu6050_refresh_sample();
    *sample = mpu6050_sample_cache;
}

static unsigned char mpu6050_dmp_ready(void)
{
    return mpu6050_dmp_ready_flag;
}

static unsigned char mpu6050_read_quaternion(imu_quaternion_t *quat)
{
    uint8_t packet[MPU6050_DMP_PACKET_SIZE];

    if ((quat == 0) || (mpu6050_dmp_ready_flag == 0U) || (mpu6050_config == 0)) {
        return 0U;
    }

    if (mpu6050_fifo_count() < MPU6050_DMP_PACKET_SIZE) {
        return 0U;
    }

    if (i2c_hal_read(mpu6050_config->instance, mpu6050_config->address, MPU6050_REG_FIFO_R_W,
                     packet, sizeof(packet)) != HAL_OK) {
        return 0U;
    }

    quat->w = (long)(((uint32_t)packet[0] << 24) | ((uint32_t)packet[1] << 16) |
                     ((uint32_t)packet[2] << 8) | packet[3]);
    quat->x = (long)(((uint32_t)packet[4] << 24) | ((uint32_t)packet[5] << 16) |
                     ((uint32_t)packet[6] << 8) | packet[7]);
    quat->y = (long)(((uint32_t)packet[8] << 24) | ((uint32_t)packet[9] << 16) |
                     ((uint32_t)packet[10] << 8) | packet[11]);
    quat->z = (long)(((uint32_t)packet[12] << 24) | ((uint32_t)packet[13] << 16) |
                     ((uint32_t)packet[14] << 8) | packet[15]);

    return 1U;
}

static unsigned char mpu6050_read_euler(imu_euler_t *euler)
{
    imu_quaternion_t quat;
    float q30 = 1073741824.0f;
    float qw;
    float qx;
    float qy;
    float qz;

    if (euler == 0) {
        return 0U;
    }

    if (mpu6050_read_quaternion(&quat) == 0U) {
        return 0U;
    }

    qw = (float)quat.w / q30;
    qx = (float)quat.x / q30;
    qy = (float)quat.y / q30;
    qz = (float)quat.z / q30;

    euler->yaw = (float)(57.2957795f * atan2f(2.0f * qx * qy - 2.0f * qw * qz,
                                             2.0f * qw * qw + 2.0f * qx * qx - 1.0f));
    euler->pitch = (float)(57.2957795f * asinf(2.0f * qx * qz + 2.0f * qw * qy));
    euler->roll = (float)(57.2957795f * atan2f(2.0f * qy * qz - 2.0f * qw * qx,
                                              2.0f * qw * qw + 2.0f * qz * qz - 1.0f));
    return 1U;
}

static const imu_sensor_t mpu6050_drv = {
    "mpu6050",
    mpu6050_init,
    mpu6050_read_sample,
    mpu6050_dmp_ready,
    mpu6050_read_quaternion,
    mpu6050_read_euler
};

REGISTER_DRIVER(IMU_SENSOR, mpu6050_drv);
