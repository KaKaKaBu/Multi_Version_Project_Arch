/**
 * @file isd1820.c
 * @brief ISD1820 record/playback GPIO driver.
 */

#include "audio_recorder_if.h"
#include "driver_configs.h"
#include "driver_core.h"
#include "gpio_hal.h"

static const isd1820_driver_config_t *isd1820_config;

static uint8_t isd1820_level(unsigned char on)
{
    if (isd1820_config == 0) {
        return 0U;
    }
    return (on != 0U) ? isd1820_config->active_high : (uint8_t)(isd1820_config->active_high == 0U);
}

static void isd1820_init(const void *config)
{
    isd1820_config = (const isd1820_driver_config_t *)config;
    if (isd1820_config == 0) {
        return;
    }

    gpio_hal_config_pin(&isd1820_config->rec);
    gpio_hal_config_pin(&isd1820_config->play);
    gpio_hal_write(isd1820_config->rec.port, isd1820_config->rec.pin, isd1820_level(0U));
    gpio_hal_write(isd1820_config->play.port, isd1820_config->play.pin, isd1820_level(0U));
}

static void isd1820_set_record(unsigned char on)
{
    if (isd1820_config == 0) {
        return;
    }
    gpio_hal_write(isd1820_config->rec.port, isd1820_config->rec.pin, isd1820_level(on));
}

static void isd1820_set_play(unsigned char on)
{
    if (isd1820_config == 0) {
        return;
    }
    gpio_hal_write(isd1820_config->play.port, isd1820_config->play.pin, isd1820_level(on));
}

static const audio_recorder_driver_t isd1820_drv = {
    "isd1820",
    isd1820_init,
    isd1820_set_record,
    isd1820_set_play
};

REGISTER_DRIVER(AUDIO_RECORDER, isd1820_drv);
