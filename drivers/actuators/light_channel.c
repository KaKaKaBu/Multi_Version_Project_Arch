/**
 * @file light_channel.c
 * @brief 多路照明 GPIO 输出（relay 接口），按通道名 light1/light2/light3 注册。
 */

#include "actuator_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

#if BOARD_LIGHT_CHANNEL_COUNT >= 1U
static void light1_init(void)
{
    gpio_hal_config_pin(&board_light1_pin);
    gpio_hal_write(board_light1_pin.port, board_light1_pin.pin, 0U);
}

static void light1_set_state(unsigned char on)
{
    gpio_hal_write(board_light1_pin.port, board_light1_pin.pin, on);
}

static const relay_driver_t light1_drv = {
    "light1",
    light1_init,
    light1_set_state
};

REGISTER_DRIVER(RELAY, light1_drv);
#endif

#if BOARD_LIGHT_CHANNEL_COUNT >= 2U
static void light2_init(void)
{
    gpio_hal_config_pin(&board_light2_pin);
    gpio_hal_write(board_light2_pin.port, board_light2_pin.pin, 0U);
}

static void light2_set_state(unsigned char on)
{
    gpio_hal_write(board_light2_pin.port, board_light2_pin.pin, on);
}

static const relay_driver_t light2_drv = {
    "light2",
    light2_init,
    light2_set_state
};

REGISTER_DRIVER(RELAY, light2_drv);
#endif

#if BOARD_LIGHT_CHANNEL_COUNT >= 3U
static void light3_init(void)
{
    gpio_hal_config_pin(&board_light3_pin);
    gpio_hal_write(board_light3_pin.port, board_light3_pin.pin, 0U);
}

static void light3_set_state(unsigned char on)
{
    gpio_hal_write(board_light3_pin.port, board_light3_pin.pin, on);
}

static const relay_driver_t light3_drv = {
    "light3",
    light3_init,
    light3_set_state
};

REGISTER_DRIVER(RELAY, light3_drv);
#endif
