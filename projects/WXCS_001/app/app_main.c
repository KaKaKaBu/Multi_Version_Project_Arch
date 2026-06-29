#include "devmgr.h"
#include "hal_common.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"
#include "key_service.h"
#include "board_config.h"

#include <string.h>

#if VERSION_FEATURE_WIFI
#include "comm_port.h"
#include "esp8266_mqtt.h"
#endif

#if VERSION_FEATURE_BLE
#include "comm_port.h"
#endif

#define APP_KEY_DEBOUNCE_MS 50U

/* 按键驱动延迟获取：devmgr 初始化后才可通过注册名查找。 */
static const input_driver_t *app_keys;

void app_comm_rx_callback(const unsigned char *data, unsigned short len);
#if VERSION_FEATURE_WIFI
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len);
#endif

#if VERSION_FEATURE_WIFI
/* WiFi 版本的 MQTT 参数来自 board_config.h，避免业务层写死账号和 Topic。 */
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

/* 传感器周期采样，采样后触发逻辑/显示刷新。 */
static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor",
    sensor_loop_run,
    3U,
    500U,
    0U,
    APP_EVENT_SENSOR
);

/* 业务循环负责按事件刷新 OLED 和处理状态变化。 */
static sched_loop_t logic_loop = SCHED_LOOP_DEF(
    "app_logic",
    logic_loop_run,
    2U,
    0U,
    APP_EVENT_KEY | APP_EVENT_SENSOR | APP_EVENT_COMM_RX | APP_EVENT_COMM_TX,
    0U
);

/* 按键扫描使用 tick 驱动，内部做 50ms 软件消抖。 */
static sched_loop_t key_loop = SCHED_LOOP_DEF(
    "key",
    key_loop_run,
    1U,
    0U,
    APP_EVENT_TICK,
    0U
);

#if VERSION_FEATURE_REMOTE
/* 远程循环负责 MQTT 轮询和遥测节流发送。 */
static sched_loop_t comm_loop = SCHED_LOOP_DEF(
    "comm",
    comm_loop_run,
    0U,
    100U,
    APP_EVENT_COMM_RX | APP_EVENT_COMM_TX | APP_EVENT_SENSOR,
    APP_EVENT_TICK
);
#endif

static void app_key_event_handler(uint8_t key_index, key_event_type_t event, void *user_data)
{
    (void)user_data;

    if (event != KEY_EVENT_SINGLE_CLICK) {
        return;
    }

    switch (key_index) {
    case 1U:
        app_logic_on_key(1U);
        break;
    case 2U:
        app_logic_on_key(2U);
        break;
    case 3U:
        app_logic_on_key(3U);
        break;
    case 4U:
        app_logic_on_key(4U);
        break;
    default:
        break;
    }

    event_set(APP_EVENT_KEY);
}

static void app_comm_setup(void)
{
#if VERSION_FEATURE_WIFI
    /* WiFi 版本优先走 ESP8266 MQTT；若当前绑定不是 esp8266，则退回原始透传。 */
    comm_port_bind(0);
    if ((comm_port_driver() != 0) &&
        (strcmp(comm_port_driver()->name, "esp8266") == 0)) {
        (void)esp8266_mqtt_connect(&app_mqtt_cfg);
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
    } else {
        comm_port_register_rx_callback(app_comm_rx_callback);
    }
#endif

#if VERSION_FEATURE_BLE
    comm_port_bind(0);
    comm_port_register_rx_callback(app_comm_rx_callback);
#endif
}

void app_main(void)
{
    /* BSP/调度/事件/设备管理按固定顺序初始化，驱动自注册依赖 devmgr_init_all。 */
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();
    key_register_callback(app_key_event_handler, 0);

    app_comm_setup();
#if VERSION_FEATURE_WIFI
    if (comm_port_has_irq() != 0) {
        (void)irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX);
    }
#endif
#if VERSION_FEATURE_BLE
    if (comm_port_has_irq() != 0) {
        (void)irq_event_bind(comm_port_irq_source(), APP_EVENT_COMM_RX);
    }
#endif

    app_logic_init();

    sched_loop_init();
    (void)sched_loop_register(&logic_loop);
    (void)sched_loop_register(&key_loop);
    (void)sched_loop_register(&sensor_loop);
#if VERSION_FEATURE_REMOTE
    (void)sched_loop_register(&comm_loop);
#endif

    sched_start();
}

void key_loop_run(sched_event_t events, void *ctx)
{
    static unsigned char last_stable_key;
    static unsigned char pending_key;
    static uint32_t debounce_start_tick;
    unsigned char raw_key;

    (void)events;
    (void)ctx;

    /* 第一次进入循环时查找按键驱动，允许驱动初始化顺序保持解耦。 */
    if (app_keys == 0) {
        app_keys = devmgr_get_input("key");
        if (app_keys == 0) {
            return;
        }
    }

    raw_key = app_keys->read_key();

    /* 简单状态机消抖：读数稳定超过 APP_KEY_DEBOUNCE_MS 后才触发单次按键。 */
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

int main(void)
{
    app_main();

    for (;;) {
    }
}
