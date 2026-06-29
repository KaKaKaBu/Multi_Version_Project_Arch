#include "gpio_hal.h"

__sfr __at(0x80) P0;
__sfr __at(0x90) P1;
__sfr __at(0xA0) P2;
__sfr __at(0xB0) P3;

static volatile __sfr *gpio_hal_port(uint8_t port)
{
    switch (port) {
    case HAL_PORT_P0:
        return &P0;
    case HAL_PORT_P1:
        return &P1;
    case HAL_PORT_P2:
        return &P2;
    case HAL_PORT_P3:
        return &P3;
    default:
        return 0;
    }
}

void gpio_hal_clock_enable(uint8_t port)
{
    (void)port;
}

void gpio_hal_apply_remap(gpio_hal_remap_t remap)
{
    (void)remap;
}

void gpio_hal_config_pin(const hal_pin_t *pin)
{
    if (pin == 0) {
        return;
    }
    if ((pin->mode == GPIO_HAL_MODE_IN_FLOATING) ||
        (pin->mode == GPIO_HAL_MODE_IN_PULLUP) ||
        (pin->mode == GPIO_HAL_MODE_OUT_OD)) {
        gpio_hal_write(pin->port, pin->pin, 1U);
    }
}

void gpio_hal_set_mode(uint8_t port, uint8_t pin, gpio_hal_mode_t mode)
{
    hal_pin_t cfg;
    cfg.port = port;
    cfg.pin = pin;
    cfg.mode = mode;
    gpio_hal_config_pin(&cfg);
}

void gpio_hal_write(uint8_t port, uint8_t pin, uint8_t value)
{
    volatile __sfr *reg = gpio_hal_port(port);
    uint8_t mask;

    if ((reg == 0) || (pin > 7U)) {
        return;
    }

    mask = (uint8_t)(1U << pin);
    if (value != 0U) {
        *reg = (uint8_t)(*reg | mask);
    } else {
        *reg = (uint8_t)(*reg & (uint8_t)~mask);
    }
}

uint8_t gpio_hal_read(uint8_t port, uint8_t pin)
{
    volatile __sfr *reg = gpio_hal_port(port);
    uint8_t mask;

    if ((reg == 0) || (pin > 7U)) {
        return 0U;
    }

    mask = (uint8_t)(1U << pin);
    return ((*reg & mask) != 0U) ? 1U : 0U;
}

void gpio_hal_toggle(uint8_t port, uint8_t pin)
{
    gpio_hal_write(port, pin, gpio_hal_read(port, pin) == 0U ? 1U : 0U);
}
