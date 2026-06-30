#include "sensor_if.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

static float ds18b20_temperature_cache;
static const one_wire_sensor_config_t *ds18b20_config;

static void ds18b20_set_output(void)
{
    if (ds18b20_config != 0) {
        gpio_hal_set_mode(ds18b20_config->pin.port, ds18b20_config->pin.pin, GPIO_HAL_MODE_OUT_OD);
    }
}

static void ds18b20_set_input(void)
{
    if (ds18b20_config != 0) {
        gpio_hal_set_mode(ds18b20_config->pin.port, ds18b20_config->pin.pin, GPIO_HAL_MODE_IN_FLOATING);
    }
}

static void ds18b20_write_pin(uint8_t level)
{
    if (ds18b20_config != 0) {
        gpio_hal_write(ds18b20_config->pin.port, ds18b20_config->pin.pin, level);
    }
}

static uint8_t ds18b20_read_pin(void)
{
    if (ds18b20_config == 0) {
        return 1U;
    }
    return gpio_hal_read(ds18b20_config->pin.port, ds18b20_config->pin.pin);
}

static void ds18b20_reset(void)
{
    ds18b20_set_output();
    ds18b20_write_pin(0U);
    hal_delay_us(750U);
    ds18b20_write_pin(1U);
    hal_delay_us(15U);
}

static uint8_t ds18b20_check(void)
{
    uint16_t retry = 0U;

    ds18b20_set_input();
    while ((ds18b20_read_pin() != 0U) && (retry < 200U)) {
        ++retry;
        hal_delay_us(1U);
    }
    if (retry >= 200U) {
        return 1U;
    }

    retry = 0U;
    while ((ds18b20_read_pin() == 0U) && (retry < 240U)) {
        ++retry;
        hal_delay_us(1U);
    }
    if (retry >= 240U) {
        return 1U;
    }

    return 0U;
}

static uint8_t ds18b20_read_bit(void)
{
    uint8_t data;

    ds18b20_set_output();
    ds18b20_write_pin(0U);
    hal_delay_us(2U);
    ds18b20_write_pin(1U);
    ds18b20_set_input();
    hal_delay_us(12U);
    data = ds18b20_read_pin();
    hal_delay_us(50U);
    return data;
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t i;
    uint8_t dat = 0U;

    for (i = 0U; i < 8U; ++i) {
        dat = (uint8_t)((ds18b20_read_bit() << 7U) | (dat >> 1U));
    }
    return dat;
}

static void ds18b20_write_byte(uint8_t dat)
{
    uint8_t j;
    uint8_t testb;

    ds18b20_set_output();
    for (j = 0U; j < 8U; ++j) {
        testb = (uint8_t)(dat & 0x01U);
        dat = (uint8_t)(dat >> 1U);
        if (testb != 0U) {
            ds18b20_write_pin(0U);
            hal_delay_us(2U);
            ds18b20_write_pin(1U);
            hal_delay_us(60U);
        } else {
            ds18b20_write_pin(0U);
            hal_delay_us(60U);
            ds18b20_write_pin(1U);
            hal_delay_us(2U);
        }
    }
}

static void ds18b20_start_convert(void)
{
    ds18b20_reset();
    (void)ds18b20_check();
    ds18b20_write_byte(0xCCU);
    ds18b20_write_byte(0x44U);
}

static float ds18b20_sample_temperature(void)
{
    uint8_t temp_sign;
    uint8_t tl;
    uint8_t th;
    int16_t raw;

    ds18b20_start_convert();
    hal_delay_us(800000U);

    ds18b20_reset();
    if (ds18b20_check() != 0U) {
        return ds18b20_temperature_cache;
    }

    ds18b20_write_byte(0xCCU);
    ds18b20_write_byte(0xBEU);
    tl = ds18b20_read_byte();
    th = ds18b20_read_byte();

    if (th > 7U) {
        th = (uint8_t)(~th);
        tl = (uint8_t)(~tl);
        temp_sign = 0U;
    } else {
        temp_sign = 1U;
    }

    raw = (int16_t)(((int16_t)th << 8) | tl);
    if (temp_sign != 0U) {
        return (float)raw * 0.0625f;
    }
    return (float)(-raw) * 0.0625f;
}

static void ds18b20_init(const void *config)
{
    ds18b20_config = (const one_wire_sensor_config_t *)config;
    if (ds18b20_config == 0) {
        return;
    }
    gpio_hal_config_pin(&ds18b20_config->pin);
    ds18b20_write_pin(1U);
    ds18b20_temperature_cache = 26.0f;
    ds18b20_reset();
    (void)ds18b20_check();
}

static float ds18b20_read_temperature(void)
{
    ds18b20_temperature_cache = ds18b20_sample_temperature();
    return ds18b20_temperature_cache;
}

static float ds18b20_read_humidity(void)
{
    return -1.0f;
}

const temp_hum_sensor_t ds18b20_drv = {
    "ds18b20",
    ds18b20_init,
    ds18b20_read_temperature,
    ds18b20_read_humidity
};

REGISTER_DRIVER(TEMP_HUM_SENSOR, ds18b20_drv);
