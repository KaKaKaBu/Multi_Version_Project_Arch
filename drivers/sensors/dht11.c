#include "sensor_if.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

#define DHT11_VALID_TEMP_MIN  0U
#define DHT11_VALID_TEMP_MAX  50U
#define DHT11_VALID_HUMI_MIN  20U
#define DHT11_VALID_HUMI_MAX  90U
#define DHT11_READ_RETRY      3U

static float dht11_temperature_cache;
static float dht11_humidity_cache;
static const one_wire_sensor_config_t *dht11_config;

static void dht11_set_output(void)
{
    if (dht11_config != 0) {
        gpio_hal_set_mode(dht11_config->pin.port, dht11_config->pin.pin, GPIO_HAL_MODE_OUT_OD);
    }
}

static void dht11_set_input(void)
{
    if (dht11_config != 0) {
        gpio_hal_set_mode(dht11_config->pin.port, dht11_config->pin.pin, GPIO_HAL_MODE_IN_PULLUP);
    }
}

static void dht11_write(uint8_t level)
{
    if (dht11_config != 0) {
        gpio_hal_write(dht11_config->pin.port, dht11_config->pin.pin, level);
    }
}

static uint8_t dht11_read_pin(void)
{
    if (dht11_config == 0) {
        return 1U;
    }
    return gpio_hal_read(dht11_config->pin.port, dht11_config->pin.pin);
}

static uint8_t dht11_read_byte(void)
{
    uint8_t i;
    uint8_t byte_data = 0U;
    uint32_t timeout;

    for (i = 0U; i < 8U; ++i) {
        timeout = 1000U;
        while ((dht11_read_pin() == 0U) && (timeout > 0U)) {
            --timeout;
        }
        if (timeout == 0U) {
            return 0xFFU;
        }

        hal_delay_us(40U);
        byte_data = (uint8_t)(byte_data << 1U);

        if (dht11_read_pin() != 0U) {
            byte_data |= 0x01U;
            timeout = 1000U;
            while ((dht11_read_pin() != 0U) && (timeout > 0U)) {
                --timeout;
            }
            if (timeout == 0U) {
                return 0xFFU;
            }
        }
    }

    return byte_data;
}

static uint8_t dht11_refresh(void)
{
    uint8_t retry;
    uint8_t humi_high;
    uint8_t humi_low;
    uint8_t temp_high;
    uint8_t temp_low;
    uint8_t check_sum;
    uint32_t timeout;

    for (retry = 0U; retry < DHT11_READ_RETRY; ++retry) {
        dht11_set_output();
        dht11_write(0U);
        hal_delay_us(19000U);
        dht11_write(1U);
        hal_delay_us(30U);

        dht11_set_input();
        timeout = 1000U;
        while ((dht11_read_pin() == 0U) && (timeout > 0U)) {
            --timeout;
        }
        if (timeout == 0U) {
            continue;
        }

        timeout = 1000U;
        while ((dht11_read_pin() != 0U) && (timeout > 0U)) {
            --timeout;
        }
        if (timeout == 0U) {
            continue;
        }

        humi_high = dht11_read_byte();
        humi_low = dht11_read_byte();
        temp_high = dht11_read_byte();
        temp_low = dht11_read_byte();
        check_sum = dht11_read_byte();

        (void)humi_low;
        (void)temp_low;

        if ((humi_high == 0xFFU) || (temp_high == 0xFFU) || (check_sum == 0xFFU)) {
            continue;
        }
        if ((uint8_t)(humi_high + humi_low + temp_high + temp_low) != check_sum) {
            continue;
        }
        if ((humi_high < DHT11_VALID_HUMI_MIN) || (humi_high > DHT11_VALID_HUMI_MAX)) {
            continue;
        }
        if ((temp_high > DHT11_VALID_TEMP_MAX)) {
            continue;
        }
        dht11_temperature_cache = (float)temp_high;
        dht11_humidity_cache = (float)humi_high;
        return 1U;
    }

    return 0U;
}

static void dht11_init(const void *config)
{
    dht11_config = (const one_wire_sensor_config_t *)config;
    if (dht11_config == 0) {
        return;
    }
    gpio_hal_config_pin(&dht11_config->pin);
    dht11_write(1U);
    dht11_temperature_cache = 25.0f;
    dht11_humidity_cache = 50.0f;
}

static float dht11_read_temperature(void)
{
    (void)dht11_refresh();
    return dht11_temperature_cache;
}

static float dht11_read_humidity(void)
{
    (void)dht11_refresh();
    return dht11_humidity_cache;
}

static const temp_hum_sensor_t dht11_drv = {
    "dht11",
    dht11_init,
    dht11_read_temperature,
    dht11_read_humidity
};

REGISTER_DRIVER(TEMP_HUM_SENSOR, dht11_drv);
