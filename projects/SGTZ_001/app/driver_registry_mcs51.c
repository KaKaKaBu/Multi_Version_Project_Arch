#include "driver_core.h"
#include "actuator_if.h"
#include "board_config.h"
#include "display_if.h"
#include "distance_if.h"
#include "driver_configs.h"
#include "input_if.h"
#include "misc_if.h"
#include "comm_if.h"
#include "weight_if.h"

extern const display_driver_t oled_drv;
extern const weight_sensor_t hx711_drv;
#if VERSION_FEATURE_HEIGHT
extern const distance_sensor_t hcsr04_drv;
#endif
#if VERSION_FEATURE_FAN_CONTROL
extern const relay_driver_t relay_drv;
#endif
#if VERSION_FEATURE_WEIGHT_ALARM || VERSION_FEATURE_VOICE
extern const misc_driver_t buzzer_drv;
#endif
#if VERSION_FEATURE_WIFI
extern const comm_driver_t esp8266_drv;
#endif
extern const input_driver_t key_drv;

static const driver_registry_entry_t sgtz_mcs51_registry[] = {
    { DRIVER_TYPE_DISPLAY, &oled_drv },
    { DRIVER_TYPE_WEIGHT_SENSOR, &hx711_drv },
#if VERSION_FEATURE_HEIGHT
    { DRIVER_TYPE_DISTANCE_SENSOR, &hcsr04_drv },
#endif
#if VERSION_FEATURE_FAN_CONTROL
    { DRIVER_TYPE_RELAY, &relay_drv },
#endif
#if VERSION_FEATURE_WEIGHT_ALARM || VERSION_FEATURE_VOICE
    { DRIVER_TYPE_MISC, &buzzer_drv },
#endif
#if VERSION_FEATURE_WIFI
    { DRIVER_TYPE_COMM, &esp8266_drv },
#endif
    { DRIVER_TYPE_INPUT, &key_drv }
};

static const i2c_device_config_t sgtz_mcs51_oled_config = {
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

static const hx711_driver_config_t sgtz_mcs51_hx711_config = {
    .sck = {
        board_hx711_sck_pin.port,
        board_hx711_sck_pin.pin,
        board_hx711_sck_pin.mode
    },
    .data = {
        board_hx711_data_pin.port,
        board_hx711_data_pin.pin,
        board_hx711_data_pin.mode
    },
    .scale = BOARD_HX711_SCALE,
    .offset = BOARD_HX711_OFFSET
};

#if VERSION_FEATURE_HEIGHT
static const hcsr04_driver_config_t sgtz_mcs51_hcsr04_config = {
    .trig = {
        board_hcsr04_trig_pin.port,
        board_hcsr04_trig_pin.pin,
        board_hcsr04_trig_pin.mode
    },
    .echo = {
        board_hcsr04_echo_pin.port,
        board_hcsr04_echo_pin.pin,
        board_hcsr04_echo_pin.mode
    }
};
#endif

#if VERSION_FEATURE_FAN_CONTROL
static const gpio_output_driver_config_t sgtz_mcs51_relay_config = {
    .pin = {
        board_fan_relay_pin.port,
        board_fan_relay_pin.pin,
        board_fan_relay_pin.mode
    },
    .active_high = 1U
};
#endif

#if VERSION_FEATURE_WEIGHT_ALARM || VERSION_FEATURE_VOICE
static const gpio_output_driver_config_t sgtz_mcs51_buzzer_config = {
    .pin = {
        board_buzzer_pin.port,
        board_buzzer_pin.pin,
        board_buzzer_pin.mode
    },
    .active_high = BOARD_BUZZER_TRIGGER_LEVEL
};
#endif

#if VERSION_FEATURE_WIFI
static const esp8266_driver_config_t sgtz_mcs51_esp8266_config = {
    .usart = {
        .instance = BOARD_ESP8266_USART,
        .baudrate = BOARD_ESP8266_BAUDRATE,
        .tx = {
            board_esp8266_tx.port,
            board_esp8266_tx.pin,
            board_esp8266_tx.mode
        },
        .rx = {
            board_esp8266_rx.port,
            board_esp8266_rx.pin,
            board_esp8266_rx.mode
        },
        .remap = BOARD_ESP8266_USART_REMAP,
        .tx_mode = BOARD_ESP8266_USART_TX_MODE,
        .tx_dma_channel = 0U,
        .instance_id = BOARD_ESP8266_USART_ID
    },
    .ch_pd = {
        board_esp8266_ch_pd_pin.port,
        board_esp8266_ch_pd_pin.pin,
        board_esp8266_ch_pd_pin.mode
    },
    .rst = {
        board_esp8266_rst_pin.port,
        board_esp8266_rst_pin.pin,
        board_esp8266_rst_pin.mode
    },
    .debug_trace_enable = BOARD_ESP8266_DEBUG_TRACE_ENABLE
};
#endif

static const hal_pin_t sgtz_mcs51_key_pins[] = {
    { board_key1_pin.port, board_key1_pin.pin, board_key1_pin.mode },
    { board_key2_pin.port, board_key2_pin.pin, board_key2_pin.mode },
    { board_key3_pin.port, board_key3_pin.pin, board_key3_pin.mode },
    { board_key4_pin.port, board_key4_pin.pin, board_key4_pin.mode }
};

static const gpio_input_driver_config_t sgtz_mcs51_key_config = {
    .pins = sgtz_mcs51_key_pins,
    .pin_refs = 0,
    .count = BOARD_KEY_COUNT,
    .active_low = 1U
};

static const board_device_entry_t sgtz_mcs51_board_devices[] = {
    { DRIVER_TYPE_DISPLAY, "oled", &sgtz_mcs51_oled_config },
    { DRIVER_TYPE_WEIGHT_SENSOR, "hx711", &sgtz_mcs51_hx711_config },
#if VERSION_FEATURE_HEIGHT
    { DRIVER_TYPE_DISTANCE_SENSOR, "hcsr04", &sgtz_mcs51_hcsr04_config },
#endif
#if VERSION_FEATURE_FAN_CONTROL
    { DRIVER_TYPE_RELAY, "relay", &sgtz_mcs51_relay_config },
#endif
#if VERSION_FEATURE_WEIGHT_ALARM || VERSION_FEATURE_VOICE
    { DRIVER_TYPE_MISC, "buzzer", &sgtz_mcs51_buzzer_config },
#endif
#if VERSION_FEATURE_WIFI
    { DRIVER_TYPE_COMM, "esp8266", &sgtz_mcs51_esp8266_config },
#endif
    { DRIVER_TYPE_INPUT, "key", &sgtz_mcs51_key_config }
};

const driver_registry_entry_t *driver_registry_begin(void)
{
    return sgtz_mcs51_registry;
}

const driver_registry_entry_t *driver_registry_end(void)
{
    return &sgtz_mcs51_registry[sizeof(sgtz_mcs51_registry) / sizeof(sgtz_mcs51_registry[0])];
}

const board_device_entry_t *board_device_registry_begin(void)
{
    return sgtz_mcs51_board_devices;
}

const board_device_entry_t *board_device_registry_end(void)
{
    return &sgtz_mcs51_board_devices[sizeof(sgtz_mcs51_board_devices) / sizeof(sgtz_mcs51_board_devices[0])];
}
