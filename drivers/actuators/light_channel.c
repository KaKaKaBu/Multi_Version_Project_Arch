/**
 * @file light_channel.c
 * @brief 多路照明 GPIO 输出（relay 接口），按通道名 light1/light2/light3 注册。
 */

#include "actuator_if.h"
#include "gpio_hal.h"
#include "driver_configs.h"
#include "driver_core.h"

static const gpio_output_driver_config_t *light1_config;
static const gpio_output_driver_config_t *light2_config;
static const gpio_output_driver_config_t *light3_config;

static void light_output_init(const void *config, const gpio_output_driver_config_t **storage)
{
    *storage = (const gpio_output_driver_config_t *)config;
    if (*storage == 0) {
        return;
    }
    gpio_hal_config_pin(&(*storage)->pin);
    gpio_hal_write((*storage)->pin.port, (*storage)->pin.pin, (*storage)->active_high ? 0U : 1U);
}

static void light_output_set_state(const gpio_output_driver_config_t *config, unsigned char on)
{
    uint8_t level;

    if (config == 0) {
        return;
    }
    level = (on != 0U) ? config->active_high : (uint8_t)!config->active_high;
    gpio_hal_write(config->pin.port, config->pin.pin, level);
}

static void light1_init(const void *config)
{
    light_output_init(config, &light1_config);
}

static void light1_set_state(unsigned char on)
{
    light_output_set_state(light1_config, on);
}

static const relay_driver_t light1_drv = {
    "light1",
    light1_init,
    light1_set_state
};

REGISTER_DRIVER(RELAY, light1_drv);

static void light2_init(const void *config)
{
    light_output_init(config, &light2_config);
}

static void light2_set_state(unsigned char on)
{
    light_output_set_state(light2_config, on);
}

static const relay_driver_t light2_drv = {
    "light2",
    light2_init,
    light2_set_state
};

REGISTER_DRIVER(RELAY, light2_drv);

static void light3_init(const void *config)
{
    light_output_init(config, &light3_config);
}

static void light3_set_state(unsigned char on)
{
    light_output_set_state(light3_config, on);
}

static const relay_driver_t light3_drv = {
    "light3",
    light3_init,
    light3_set_state
};

REGISTER_DRIVER(RELAY, light3_drv);
