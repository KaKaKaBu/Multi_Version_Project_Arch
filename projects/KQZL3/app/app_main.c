/* KQZL3 固件入口：初始化 BSP、设备管理、通讯、按键和调度循环。 */
#include "devmgr.h"
#include "hal_common.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"
#include "key_service.h"
#include "board_config.h"

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
#include "comm_port.h"
#endif

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#include <string.h>
#endif

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

static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor", sensor_loop_run, 2, 2000,
    SCHED_EVENT_TICK, APP_EVENT_SENSOR_READY);

static sched_loop_t alarm_loop = SCHED_LOOP_DEF(
    "alarm", alarm_loop_run, 1, 1000,
    SCHED_EVENT_TICK, APP_EVENT_TICK);

static sched_loop_t display_loop = SCHED_LOOP_DEF(
    "display", display_loop_run, 3, 500,
    SCHED_EVENT_TICK, APP_EVENT_TICK);

static sched_loop_t comm_loop = SCHED_LOOP_DEF(
    "comm", comm_loop_run, 4, 50,
    APP_EVENT_TICK | APP_EVENT_COMM_RX | APP_EVENT_COMM_TX, 0);

static void app_key_event_handler(uint8_t key_index, key_event_type_t event, void *user_data)
{
    (void)user_data;

    if (event != KEY_EVENT_SINGLE_CLICK) {
        return;
    }

    switch (key_index) {
    case 1U:
        app_logic_on_key1_press();
        break;
    case 2U:
        app_logic_on_key2_press();
        break;
    case 3U:
        app_logic_on_key3_press();
        break;
    case 4U:
        app_logic_on_key4_press();
        break;
    default:
        break;
    }
}

static void app_comm_setup(void)
{
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    comm_port_bind(0);

#if VERSION_FEATURE_WIFI
    if ((comm_port_driver() != 0) &&
        (strcmp(comm_port_driver()->name, "esp8266") == 0)) {
        (void)esp8266_mqtt_connect(&app_mqtt_cfg);
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
        return;
    }
#endif

    comm_port_register_rx_callback(app_comm_rx_callback);
#endif
}

void app_main(void)
{
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();
    key_register_callback(app_key_event_handler, 0);

    app_comm_setup();
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    if (comm_port_has_irq() != 0) {
        (void)irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX);
    }
#endif

    app_logic_init();

    sched_loop_init();
    sched_loop_register(&sensor_loop);
    sched_loop_register(&alarm_loop);
    sched_loop_register(&display_loop);
    sched_loop_register(&comm_loop);

    sched_start();
}

int main(void)
{
    app_main();
    for (;;) {
    }
}
