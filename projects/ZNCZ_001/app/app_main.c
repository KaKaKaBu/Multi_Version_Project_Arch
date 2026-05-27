#include "devmgr.h"
#include "comm_port.h"
#include "board_config.h"
#include "hal_common.h"
#include "input_if.h"
#include "irq_event.h"
#include "scheduler.h"
#include "version_config.h"
#include "app_logic.h"
#include "debug_log.h"

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#include <string.h>
#endif

#define APP_KEY_DEBOUNCE_MS 50U
#define APP_RTC_TICK_MS 1000U

void app_comm_rx_callback(const unsigned char *data, unsigned short len);
#if VERSION_FEATURE_WIFI
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len);
#endif

static void app_logic_task_entry(driver_task_t *task);
static void comm_poll_task_entry(driver_task_t *task);
static void key_poll_task_entry(driver_task_t *task);
static void rtc_tick_task_entry(driver_task_t *task);

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

static driver_task_t app_logic_task = {
    "app_logic",
    app_logic_task_entry,
    4U,
    TASK_READY,
    0U,
    0U,
    0
};

static driver_task_t comm_poll_task = {
    "comm_poll",
    comm_poll_task_entry,
    3U,
    TASK_READY,
    0U,
    0U,
    0
};

static driver_task_t key_poll_task = {
    "key_poll",
    key_poll_task_entry,
    2U,
    TASK_READY,
    0U,
    0U,
    0
};

static driver_task_t rtc_tick_task = {
    "rtc_tick",
    rtc_tick_task_entry,
    1U,
    TASK_READY,
    0U,
    0U,
    0
};

static void app_logic_task_entry(driver_task_t *task)
{
    sched_event_t events = task_events_get(task);

    app_logic_loop(events);
    task_block(APP_EVENT_TICK | APP_EVENT_KEY | APP_EVENT_COMM_RX);
}

static void comm_poll_task_entry(driver_task_t *task)
{
    static unsigned char mqtt_rx_defer_ticks;
    unsigned char rx;
    sched_event_t events = task_events_get(task);
    int mqtt_ready = 0;

    (void)task;

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

    task_block(APP_EVENT_COMM_RX | APP_EVENT_TICK);
}

static void key_poll_task_entry(driver_task_t *task)
{
    const input_driver_t *keys = devmgr_get_input("key");
    static unsigned char last_stable_key;
    static unsigned char pending_key;
    static uint32_t debounce_start_tick;
    unsigned char raw_key;

    (void)task;

    if (keys == 0) {
        task_block(APP_EVENT_TICK);
        return;
    }

    raw_key = keys->read_key();

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

    task_block(APP_EVENT_TICK);
}

static void rtc_tick_task_entry(driver_task_t *task)
{
    static uint32_t last_second_tick;
    uint32_t now_tick;

    (void)task;

    now_tick = sched_tick_get();
    if ((now_tick - last_second_tick) >= APP_RTC_TICK_MS) {
        DEBUG_LOG_SCHED("[SCHED] rtc_tick fired tick=%lu delta=%lu\r\n",
                        (unsigned long)now_tick,
                        (unsigned long)(now_tick - last_second_tick));
        last_second_tick = now_tick;
        app_logic_on_second_tick();
        event_set(APP_EVENT_TICK);
    }

    task_block(APP_EVENT_TICK);
}

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

    sched_add_task(&app_logic_task);
    sched_add_task(&comm_poll_task);
    sched_add_task(&key_poll_task);
    sched_add_task(&rtc_tick_task);
    sched_start();
}

int main(void)
{
    app_main();

    for (;;) {
    }
}
