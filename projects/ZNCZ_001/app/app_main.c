/* ZNCZ_001 固件入口：初始化 BSP、设备管理、WiFi、按键、RTC 和调度循环。 */
#include "devmgr.h"
#include "comm_port.h"
#include "board_config.h"
#include "hal_common.h"
#include "key_service.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"
#include "debug_log.h"

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#include <string.h>
#endif

#define APP_RTC_TICK_MS 1000U

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
    static unsigned char mqtt_rx_defer_ticks;
    unsigned char rx;
    int mqtt_ready = 0;

    (void)ctx;

#if VERSION_FEATURE_WIFI
    if (esp8266_mqtt_is_ready() != 0) {
        mqtt_ready = 1;
        if ((events & APP_EVENT_COMM_RX) != 0U) {
            mqtt_rx_defer_ticks = 2U;
        }
        if (((events & APP_EVENT_TICK) != 0U) || (mqtt_rx_defer_ticks != 0U)) {
            esp8266_mqtt_poll();
            if (mqtt_rx_defer_ticks != 0U) {
                --mqtt_rx_defer_ticks;
                event_set(APP_EVENT_TICK);
            }
        }
    } else if ((events & APP_EVENT_COMM_RX) != 0U) {
        DEBUG_LOG_APP("[APP] mqtt not ready on comm rx\r\n");
    }
#endif

    if (((events & APP_EVENT_COMM_RX) != 0U) && (mqtt_ready == 0)) {
        while (comm_port_recv(&rx, 1U) > 0) {
        }
    }
}

static void rtc_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    app_logic_on_second_tick();
}

static void app_key_event_handler(uint8_t key_index, key_event_type_t event, void *ctx)
{
    (void)ctx;

    if (key_index == 0U) {
        return;
    }

    switch (event) {
    case KEY_EVENT_SINGLE_CLICK:
    case KEY_EVENT_DOUBLE_CLICK:
    case KEY_EVENT_LONG_PRESS:
        app_logic_on_key(key_index);
        event_set(APP_EVENT_KEY);
        break;
    default:
        break;
    }
}

static sched_loop_t logic_loop = SCHED_LOOP_DEF(
    "app_logic",
    app_logic_run,
    4U,
    0U,
    APP_EVENT_TICK | APP_EVENT_KEY | APP_EVENT_COMM_RX,
    0U
);

static sched_loop_t comm_loop = SCHED_LOOP_DEF(
    "comm",
    comm_loop_run,
    3U,
    0U,
    APP_EVENT_COMM_RX | APP_EVENT_TICK,
    0U
);

static sched_loop_t rtc_loop = SCHED_LOOP_DEF(
    "rtc",
    rtc_loop_run,
    1U,
    APP_RTC_TICK_MS,
    0U,
    APP_EVENT_TICK
);

static void app_comm_setup(void)
{
    int mqtt_rc;

    comm_port_bind(0);
    DEBUG_LOG_APP("[APP] comm bind driver=%s irq=%d source=%u\r\n",
                  (comm_port_driver() != 0) ? comm_port_driver()->name : "null",
                  comm_port_has_irq(),
                  (unsigned int)comm_port_irq_source());

#if VERSION_FEATURE_WIFI
    if ((comm_port_driver() != 0) &&
        (strcmp(comm_port_driver()->name, "esp8266") == 0)) {
        mqtt_rc = esp8266_mqtt_connect(&app_mqtt_cfg);
        (void)mqtt_rc;
        DEBUG_LOG_APP("[APP] mqtt connect rc=%d ready=%d\r\n", mqtt_rc, esp8266_mqtt_is_ready());
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
        return;
    }
#endif

    comm_port_register_rx_callback(app_comm_rx_callback);
}

void app_main(void)
{
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();
    sched_loop_init();

    key_register_callback(app_key_event_handler, 0);

    app_comm_setup();

    if (comm_port_has_irq() != 0) {
        int bind_rc = irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX);
        (void)bind_rc;
        DEBUG_LOG_APP("[APP] irq bind rc=%d source=%u event=0x%08lX\r\n",
                      bind_rc,
                      (unsigned int)comm_port_irq_source(),
                      (unsigned long)APP_EVENT_COMM_RX);
    }

    app_logic_init();

    (void)sched_loop_register(&logic_loop);
    (void)sched_loop_register(&comm_loop);
    (void)sched_loop_register(&rtc_loop);
    sched_start();
}

int main(void)
{
    app_main();

    for (;;) {
    }
}
