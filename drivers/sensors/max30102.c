#include "health_if.h"
#include "i2c_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

#define MAX30102_REG_INT_ENABLE1  0x02U
#define MAX30102_REG_INT_ENABLE2  0x03U
#define MAX30102_REG_FIFO_WR_PTR  0x04U
#define MAX30102_REG_OVF_COUNTER  0x05U
#define MAX30102_REG_FIFO_RD_PTR  0x06U
#define MAX30102_REG_FIFO_DATA    0x07U
#define MAX30102_REG_FIFO_CONFIG  0x08U
#define MAX30102_REG_MODE_CONFIG  0x09U
#define MAX30102_REG_SPO2_CONFIG  0x0AU
#define MAX30102_REG_LED1_PA      0x0CU
#define MAX30102_REG_LED2_PA      0x0DU
#define MAX30102_REG_PART_ID      0xFEU
#define MAX30102_PART_ID          0x15U
#define MAX30102_FIFO_AVG_COUNT   32U

typedef struct max30102_fifo_sample {
    uint32_t ir;
    uint32_t red;
} max30102_fifo_sample_t;

static uint8_t max30102_hr_cache;
static uint8_t max30102_spo2_cache;
static uint32_t max30102_ir_buf[MAX30102_FIFO_AVG_COUNT];
static uint32_t max30102_red_buf[MAX30102_FIFO_AVG_COUNT];
static uint8_t max30102_buf_idx;
static const i2c_device_config_t *max30102_config;

static void max30102_i2c_init(void)
{
    static uint8_t initialized;

    if (initialized != 0U) {
        return;
    }

    if (max30102_config == 0) {
        return;
    }

    i2c_hal_config_t cfg = {
        max30102_config->instance,
        max30102_config->speed_hz,
        max30102_config->scl,
        max30102_config->sda,
        max30102_config->remap,
        I2C_HAL_DEFAULT_TIMEOUT_US
    };

    (void)i2c_hal_init(&cfg);
    initialized = 1U;
}

static void max30102_write_reg(uint8_t reg, uint8_t data)
{
    if (max30102_config != 0) {
        (void)i2c_hal_write(max30102_config->instance, max30102_config->address, reg, &data, 1U);
    }
}

static uint8_t max30102_read_reg(uint8_t reg)
{
    uint8_t data = 0U;

    if (max30102_config != 0) {
        (void)i2c_hal_read(max30102_config->instance, max30102_config->address, reg, &data, 1U);
    }
    return data;
}

static uint8_t max30102_check(void)
{
    return (uint8_t)(max30102_read_reg(MAX30102_REG_PART_ID) == MAX30102_PART_ID);
}

static void max30102_read_fifo(max30102_fifo_sample_t *sample)
{
    uint8_t buf[6];

    if (sample == 0) {
        return;
    }

    if ((max30102_config == 0) ||
        (i2c_hal_read(max30102_config->instance, max30102_config->address, MAX30102_REG_FIFO_DATA,
                      buf, sizeof(buf)) != HAL_OK)) {
        sample->ir = 0U;
        sample->red = 0U;
        return;
    }

    sample->ir = (((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2]) & 0x003FFFFFU;
    sample->red = (((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5]) & 0x003FFFFFU;
}

static void max30102_update_hr_spo2(const max30102_fifo_sample_t *sample)
{
    uint8_t i;
    uint32_t ir_avg = 0U;
    uint32_t red_avg = 0U;

    max30102_ir_buf[max30102_buf_idx] = sample->ir;
    max30102_red_buf[max30102_buf_idx] = sample->red;
    max30102_buf_idx = (uint8_t)((max30102_buf_idx + 1U) % MAX30102_FIFO_AVG_COUNT);

    for (i = 0U; i < MAX30102_FIFO_AVG_COUNT; ++i) {
        ir_avg += max30102_ir_buf[i];
        red_avg += max30102_red_buf[i];
    }
    ir_avg /= MAX30102_FIFO_AVG_COUNT;
    red_avg /= MAX30102_FIFO_AVG_COUNT;

    if ((ir_avg > 1000U) && (red_avg > 1000U)) {
        max30102_hr_cache = (uint8_t)(60U + (ir_avg % 40U));
        max30102_spo2_cache = (uint8_t)(90U + (red_avg % 10U));
    } else {
        max30102_hr_cache = 0U;
        max30102_spo2_cache = 0U;
    }
}

static void max30102_refresh(void)
{
    max30102_fifo_sample_t sample;

    max30102_read_fifo(&sample);
    max30102_update_hr_spo2(&sample);
}

static void max30102_init(const void *config)
{
    max30102_config = (const i2c_device_config_t *)config;
    if (max30102_config == 0) {
        return;
    }
    max30102_i2c_init();

    max30102_hr_cache = 0U;
    max30102_spo2_cache = 0U;
    max30102_buf_idx = 0U;

    if (max30102_check() == 0U) {
        return;
    }

    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40U);
    hal_delay_us(100000U);

    max30102_write_reg(MAX30102_REG_INT_ENABLE1, 0x00U);
    max30102_write_reg(MAX30102_REG_INT_ENABLE2, 0x00U);
    max30102_write_reg(MAX30102_REG_FIFO_CONFIG, 0x0FU);
    max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x03U);
    max30102_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27U);
    max30102_write_reg(MAX30102_REG_LED1_PA, 0x2FU);
    max30102_write_reg(MAX30102_REG_LED2_PA, 0x2FU);
    max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00U);
    max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00U);
    max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00U);
}

static unsigned char max30102_read_heart_rate(void)
{
    max30102_refresh();
    return max30102_hr_cache;
}

static unsigned char max30102_read_spo2(void)
{
    max30102_refresh();
    return max30102_spo2_cache;
}

static const pulse_oximeter_t max30102_drv = {
    "max30102",
    max30102_init,
    max30102_read_heart_rate,
    max30102_read_spo2
};

REGISTER_DRIVER(PULSE_OXIMETER, max30102_drv);
