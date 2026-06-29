#include "driver_core.h"
#include "actuator_if.h"
#include "display_if.h"
#include "gas_if.h"
#include "input_if.h"
#include "misc_if.h"
#include "sensor_if.h"

extern const display_driver_t oled_drv;
extern const gas_sensor_t mq135_drv;
extern const gas_sensor_t mq7_co_drv;
extern const temp_hum_sensor_t ds18b20_drv;
extern const relay_driver_t relay_drv;
extern const misc_driver_t buzzer_drv;
extern const misc_driver_t led_drv;
extern const input_driver_t key_drv;

static const driver_registry_entry_t wxcs_mcs51_registry[] = {
    { DRIVER_TYPE_DISPLAY, &oled_drv },
    { DRIVER_TYPE_GAS_SENSOR, &mq135_drv },
    { DRIVER_TYPE_GAS_SENSOR, &mq7_co_drv },
    { DRIVER_TYPE_TEMP_HUM_SENSOR, &ds18b20_drv },
    { DRIVER_TYPE_RELAY, &relay_drv },
    { DRIVER_TYPE_MISC, &buzzer_drv },
    { DRIVER_TYPE_MISC, &led_drv },
    { DRIVER_TYPE_INPUT, &key_drv }
};

const driver_registry_entry_t *driver_registry_begin(void)
{
    return wxcs_mcs51_registry;
}

const driver_registry_entry_t *driver_registry_end(void)
{
    return &wxcs_mcs51_registry[sizeof(wxcs_mcs51_registry) / sizeof(wxcs_mcs51_registry[0])];
}
