#include "devmgr.h"
#include "comm_port.h"
#include "board_config.h"
#include "hal_common.h"
#include "input_if.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"
#include <string.h>

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif

#define APP_KEY_DEBOUNCE_MS 50U
#define APP_SENSOR_POLL_MS 500U

void app_comm_rx_callback(const unsigned char *data, unsigned short len);
#if VERSION_FEATURE_WIFI
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len);
#endif

#if VERSION_FEATURE_WIFI
static const esp8266_mqtt_config_t app_mqtt_cfg = {
    BOARD_ESP8266_WIFI_SSID,
    BOARD_ESP8266_WIFI_PASS,
    BOARD_ESP8266_MQTT_BROKER,
    BOARD_ESP8266_MQTT_PORT,
    BOARD_ESP8266_MQTT_CLIENT_ID,
    BOARD_ESP8266_MQTT_USER,
    BOARD_ESP8266_MQTT_PASS,
    BOARD_ESP8266_MQTT_SUB_TOPIC,
    BOARD_ESP8266_MQTT_PUB_TOPIC
};
#endif

static void app_logic_run(sched_event_t events, void *ctx)
{
    (void)ctx;
    app_logic_loop(events);
}

static void comm_loop_run(sched_event_t events, void *ctx)
{
    unsigned char rx;

    (void)ctx;

#if VERSION_FEATURE_WIFI
    if (esp8266_mqtt_is_ready() != 0) {
        if ((events & APP_EVENT_TICK) != 0U) {
            esp8266_mqtt_poll();
        }
    }
#endif

    if ((events & APP_EVENT_COMM_RX) != 0U) {
        while (comm_port_recv(&rx, 1U) > 0) {
        }
    }
}

static void key_loop_run(sched_event_t events, void *ctx)
{
    const input_driver_t *keys = devmgr_get_input("key");
    static unsigned char last_stable_key;
    static unsigned char pending_key;
    static uint32_t debounce_start_tick;
    unsigned char raw_key;

    (void)events;
    (void)ctx;

    if (keys == 0) {
        return;
    }

    raw_key = keys->read_key();
    if (raw_key > 4U) {
        raw_key = 0U;
    }

    if (raw_key != pending_key) {
        pending_key = raw_key;
        debounce_start_tick = sched_tick_get();
    } else if ((raw_key != 0U) &&
               ((sched_tick_get() - debounce_start_tick) >= APP_KEY_DEBOUNCE_MS) &&
               (raw_key != last_stable_key)) {
        last_stable_key = raw_key;
        app_logic_on_key(raw_key);
        event_set(APP_EVENT_KEY);
    }

    if (raw_key == 0U) {
        last_stable_key = 0U;
        pending_key = 0U;
    }
}

static void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    app_logic_on_sensor_poll();
}

static sched_loop_t logic_loop = SCHED_LOOP_DEF(
    "app_logic",
    app_logic_run,
    4U,
    0U,
    APP_EVENT_KEY | APP_EVENT_SENSOR | APP_EVENT_COMM_RX,
    0U
);

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
static sched_loop_t comm_loop = SCHED_LOOP_DEF(
    "comm",
    comm_loop_run,
    3U,
    0U,
    APP_EVENT_COMM_RX | APP_EVENT_TICK,
    0U
);
#endif

static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor",
    sensor_loop_run,
    2U,
    APP_SENSOR_POLL_MS,
    0U,
    APP_EVENT_SENSOR
);

static sched_loop_t key_loop = SCHED_LOOP_DEF(
    "key",
    key_loop_run,
    1U,
    0U,
    APP_EVENT_TICK,
    0U
);

static void app_comm_setup(void)
{
#if VERSION_FEATURE_WIFI
    int mqtt_rc;

    comm_port_bind(0);
    if ((comm_port_driver() != 0) &&
        (strcmp(comm_port_driver()->name, "esp8266") == 0)) {
        mqtt_rc = esp8266_mqtt_connect(&app_mqtt_cfg);
        (void)mqtt_rc;
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
        return;
    }
#endif

#if VERSION_FEATURE_BLE
    comm_port_bind(0);
    comm_port_register_rx_callback(app_comm_rx_callback);
    return;
#endif

    (void)comm_port_bind(0);
}

void app_main(void)
{
    bsp_init();
    sched_init();
    sched_loop_init();
    irq_event_init();
    devmgr_init_all();

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    app_comm_setup();

    if (comm_port_has_irq() != 0) {
        (void)irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX);
    }
#endif

    app_logic_init();

    (void)sched_loop_register(&logic_loop);
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    (void)sched_loop_register(&comm_loop);
#endif
    (void)sched_loop_register(&sensor_loop);
    (void)sched_loop_register(&key_loop);
    sched_start();
}

int main(void)
{
    app_main();

    for (;;) {
    }
}
