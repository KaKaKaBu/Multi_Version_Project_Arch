#include "version_config.h"
#include "scheduler.h"
#include "debug_log.h"
#include "app_logic.h"

#define APP_RX_BUFFER_SIZE 256U

static unsigned char app_last_rx_buffer[APP_RX_BUFFER_SIZE];
static unsigned short app_last_rx_len;

void app_comm_rx_callback(const unsigned char *data, unsigned short len)
{
    unsigned short i;
    unsigned short copy_len = len;

    if (copy_len > APP_RX_BUFFER_SIZE) {
        copy_len = APP_RX_BUFFER_SIZE;
    }

    for (i = 0; i < copy_len; ++i) {
        app_last_rx_buffer[i] = data[i];
    }

    app_last_rx_buffer[copy_len] = '\0';
    app_last_rx_len = copy_len;

#if KQZL3_VERSION >= 3
    /* Parse JSON command from WiFi/BLE */
    app_logic_on_mqtt_rx((const char *)app_last_rx_buffer);
#endif

    DEBUG_LOG_APP_CB("[APP_CB] rx len=%u\r\n", (unsigned int)copy_len);
    event_set(APP_EVENT_COMM_RX);
}

#if KQZL3_HAS_WIFI || KQZL3_HAS_BLE
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len)
{
    (void)topic;
    DEBUG_LOG_APP_CB("[APP_CB] mqtt topic=%s len=%u\r\n",
                     (topic != 0) ? topic : "null",
                     (unsigned int)len);
    app_comm_rx_callback(payload, len);
}
#endif

const unsigned char *app_get_last_rx(unsigned short *len)
{
    if (len != 0) {
        *len = app_last_rx_len;
    }

    return app_last_rx_buffer;
}

void app_clear_last_rx(void)
{
    app_last_rx_len = 0U;
}

void app_callbacks_init(void)
{
    /* Callbacks are registered by comm drivers automatically */
}
