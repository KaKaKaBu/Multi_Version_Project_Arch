#include "driver_core.h"
#include "actuator_if.h"
#include "board_config.h"
#include "display_if.h"
#include "driver_configs.h"
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

static const i2c_device_config_t wxcs_mcs51_oled_config = {
    .instance = BOARD_OLED_I2C,
    .speed_hz = BOARD_OLED_I2C_SPEED,
    .scl = {
        board_oled_i2c_scl.port,
        board_oled_i2c_scl.pin,
        board_oled_i2c_scl.mode
    },
    .sda = {
        board_oled_i2c_sda.port,
        board_oled_i2c_sda.pin,
        board_oled_i2c_sda.mode
    },
    .remap = BOARD_OLED_I2C_REMAP,
    .address = BOARD_OLED_I2C_ADDR
};

static const mq_adc0832_driver_config_t wxcs_mcs51_mq135_config = {
    .adc = {
        .cs = {
            board_adc0832_cs_pin.port,
            board_adc0832_cs_pin.pin,
            board_adc0832_cs_pin.mode
        },
        .clk = {
            board_adc0832_clk_pin.port,
            board_adc0832_clk_pin.pin,
            board_adc0832_clk_pin.mode
        },
        .dio = {
            board_adc0832_dio_pin.port,
            board_adc0832_dio_pin.pin,
            board_adc0832_dio_pin.mode
        }
    },
    .channel = BOARD_MQ135_ADC0832_CHANNEL,
    .max_ppm = BOARD_MQ135_ADC0832_MAX_PPM
};

static const mq_adc0832_driver_config_t wxcs_mcs51_mq7_config = {
    .adc = {
        .cs = {
            board_adc0832_cs_pin.port,
            board_adc0832_cs_pin.pin,
            board_adc0832_cs_pin.mode
        },
        .clk = {
            board_adc0832_clk_pin.port,
            board_adc0832_clk_pin.pin,
            board_adc0832_clk_pin.mode
        },
        .dio = {
            board_adc0832_dio_pin.port,
            board_adc0832_dio_pin.pin,
            board_adc0832_dio_pin.mode
        }
    },
    .channel = BOARD_MQ7_ADC0832_CHANNEL,
    .max_ppm = BOARD_MQ7_ADC0832_MAX_PPM
};

static const one_wire_sensor_config_t wxcs_mcs51_ds18b20_config = {
    .pin = {
        board_ds18b20_pin.port,
        board_ds18b20_pin.pin,
        board_ds18b20_pin.mode
    }
};

static const gpio_output_driver_config_t wxcs_mcs51_relay_config = {
    .pin = {
        board_relay_pin.port,
        board_relay_pin.pin,
        board_relay_pin.mode
    },
    .active_high = 1U
};

static const gpio_output_driver_config_t wxcs_mcs51_buzzer_config = {
    .pin = {
        board_buzzer_pin.port,
        board_buzzer_pin.pin,
        board_buzzer_pin.mode
    },
    .active_high = 1U
};

static const gpio_output_driver_config_t wxcs_mcs51_led_config = {
    .pin = {
        board_led_pin.port,
        board_led_pin.pin,
        board_led_pin.mode
    },
    .active_high = 0U
};

static const hal_pin_t wxcs_mcs51_key_pins[] = {
    { board_key1_pin.port, board_key1_pin.pin, board_key1_pin.mode },
    { board_key2_pin.port, board_key2_pin.pin, board_key2_pin.mode },
    { board_key3_pin.port, board_key3_pin.pin, board_key3_pin.mode },
    { board_key4_pin.port, board_key4_pin.pin, board_key4_pin.mode }
};

static const gpio_input_driver_config_t wxcs_mcs51_key_config = {
    .pins = wxcs_mcs51_key_pins,
    .pin_refs = 0,
    .count = BOARD_KEY_COUNT,
    .active_low = 1U
};

static const board_device_entry_t wxcs_mcs51_board_devices[] = {
    { DRIVER_TYPE_DISPLAY, "oled", &wxcs_mcs51_oled_config },
    { DRIVER_TYPE_GAS_SENSOR, "mq135", &wxcs_mcs51_mq135_config },
    { DRIVER_TYPE_GAS_SENSOR, "mq7_co", &wxcs_mcs51_mq7_config },
    { DRIVER_TYPE_TEMP_HUM_SENSOR, "ds18b20", &wxcs_mcs51_ds18b20_config },
    { DRIVER_TYPE_RELAY, "relay", &wxcs_mcs51_relay_config },
    { DRIVER_TYPE_MISC, "buzzer", &wxcs_mcs51_buzzer_config },
    { DRIVER_TYPE_MISC, "led", &wxcs_mcs51_led_config },
    { DRIVER_TYPE_INPUT, "key", &wxcs_mcs51_key_config }
};

const driver_registry_entry_t *driver_registry_begin(void)
{
    return wxcs_mcs51_registry;
}

const driver_registry_entry_t *driver_registry_end(void)
{
    return &wxcs_mcs51_registry[sizeof(wxcs_mcs51_registry) / sizeof(wxcs_mcs51_registry[0])];
}

const board_device_entry_t *board_device_registry_begin(void)
{
    return wxcs_mcs51_board_devices;
}

const board_device_entry_t *board_device_registry_end(void)
{
    return &wxcs_mcs51_board_devices[sizeof(wxcs_mcs51_board_devices) / sizeof(wxcs_mcs51_board_devices[0])];
}
