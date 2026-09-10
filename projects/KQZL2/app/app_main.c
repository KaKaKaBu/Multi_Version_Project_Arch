/* KQZL2 固件入口：初始化 BSP、设备管理、通讯、按键和调度循环。 */
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
static uint8_t app_mqtt_connecting;
static uint32_t app_mqtt_next_connect_tick;
#endif

#if VERSION_FEATURE_WIFI
static const esp8266_mqtt_config_t app_mqtt_cfg = {
#if VERSION_FEATURE_CLOUD
    .wifi_ssid = BOARD_ESP8266_WIFI_SSID,
    .wifi_password = BOARD_ESP8266_WIFI_PASS,
    .broker = BOARD_ESP8266_HUAWEI_BROKER,
    .port = BOARD_ESP8266_HUAWEI_PORT,
    .client_id = BOARD_ESP8266_HUAWEI_CLIENT_ID,
    .mqtt_user = BOARD_ESP8266_HUAWEI_USER,
    .mqtt_password = BOARD_ESP8266_HUAWEI_PASS,
    .sub_topic = BOARD_ESP8266_HUAWEI_PROPERTY_SET_TOPIC,
    .pub_topic = BOARD_ESP8266_HUAWEI_PROPERTY_REPORT_TOPIC,
    .backend = ESP8266_MQTT_BACKEND_HUAWEI_IOTDA,
    .scheme = ESP8266_MQTT_SCHEME_TCP,
    .huawei_device_id = BOARD_ESP8266_HUAWEI_DEVICE_ID,
    .huawei_device_secret = BOARD_ESP8266_HUAWEI_DEVICE_SECRET,
    .huawei_service_id = BOARD_ESP8266_HUAWEI_SERVICE_ID,
    .huawei_property_report_topic = BOARD_ESP8266_HUAWEI_PROPERTY_REPORT_TOPIC,
    .huawei_property_set_topic = BOARD_ESP8266_HUAWEI_PROPERTY_SET_TOPIC,
    .huawei_custom_pub_topic = BOARD_ESP8266_HUAWEI_CUSTOM_PUB_TOPIC,
    .huawei_custom_sub_topic = BOARD_ESP8266_HUAWEI_CUSTOM_SUB_TOPIC
#else
    .wifi_ssid = BOARD_ESP8266_WIFI_SSID,
    .wifi_password = BOARD_ESP8266_WIFI_PASS,
    .broker = BOARD_ESP8266_MQTT_BROKER,
    .port = BOARD_ESP8266_MQTT_PORT,
    .client_id = BOARD_ESP8266_MQTT_CLIENT_ID,
    .mqtt_user = BOARD_ESP8266_MQTT_USER,
    .mqtt_password = BOARD_ESP8266_MQTT_PASS,
    .sub_topic = BOARD_ESP8266_MQTT_SUB_TOPIC,
    .pub_topic = BOARD_ESP8266_MQTT_PUB_TOPIC,
    .backend = ESP8266_MQTT_BACKEND_GENERIC,
    .scheme = ESP8266_MQTT_SCHEME_TCP
#endif
};
#endif

#if VERSION_FEATURE_WIFI
#define APP_COMM_LOOP_PRIORITY 0
#define APP_COMM_LOOP_PERIOD_MS 100
#elif VERSION_FEATURE_BLE
#define APP_COMM_LOOP_PRIORITY 2
#define APP_COMM_LOOP_PERIOD_MS 10
#else
#define APP_COMM_LOOP_PRIORITY 0
#define APP_COMM_LOOP_PERIOD_MS 100
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
    "comm", comm_loop_run, APP_COMM_LOOP_PRIORITY, APP_COMM_LOOP_PERIOD_MS,
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
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
        return;
    }
#endif

    comm_port_register_rx_callback(app_comm_rx_callback);
#endif
}

void app_comm_poll(sched_event_t events)
{
#if VERSION_FEATURE_WIFI
    uint32_t now;

    if ((comm_port_driver() == 0) ||
        (strcmp(comm_port_driver()->name, "esp8266") != 0)) {
        (void)events;
        return;
    }

    now = sched_tick_get();
    if (esp8266_mqtt_is_ready() != 0) {
        if ((events & (APP_EVENT_COMM_RX | APP_EVENT_TICK)) != 0U) {
            esp8266_mqtt_poll();
        }
        return;
    }

    if ((events & APP_EVENT_TICK) == 0U) {
        return;
    }
    if (app_mqtt_connecting != 0U) {
        return;
    }
    if ((int32_t)(now - app_mqtt_next_connect_tick) < 0) {
        return;
    }

    app_mqtt_connecting = 1U;
    if (esp8266_mqtt_connect(&app_mqtt_cfg) == 0) {
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
        app_logic_request_telemetry();
    } else {
        app_mqtt_next_connect_tick = now + 10000U;
    }
    app_mqtt_connecting = 0U;
#else
    (void)events;
#endif
}

void app_main(void)
{
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();
    key_register_callback(app_key_event_handler, 0);

    app_logic_init();
    app_comm_setup();
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    if (comm_port_has_irq() != 0) {
        (void)irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX);
    }
#endif

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

