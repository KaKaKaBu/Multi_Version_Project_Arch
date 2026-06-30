/* FJXT_001 通信回调：BLE 透传或 MQTT payload 组帧后交给业务逻辑。 */
#include "version_config.h"
#include "scheduler.h"
#include "app_logic.h"

#define APP_RX_BUFFER_SIZE 256U

static unsigned char app_rx_buffer[APP_RX_BUFFER_SIZE];
static unsigned short app_rx_len;

static void app_process_rx_frame(const unsigned char *data, unsigned short len)
{
    unsigned short i;
    unsigned short copy_len = len;

    if ((data == 0) || (len == 0U)) {
        return;
    }

    if (copy_len >= APP_RX_BUFFER_SIZE) {
        copy_len = APP_RX_BUFFER_SIZE - 1U;
    }

    for (i = 0U; i < copy_len; ++i) {
        app_rx_buffer[i] = data[i];
    }
    app_rx_buffer[copy_len] = '\0';

#if VERSION_FEATURE_REMOTE
    app_logic_on_remote_rx((const char *)app_rx_buffer);
#endif
    event_set(APP_EVENT_COMM_RX);
}

void app_comm_rx_callback(const unsigned char *data, unsigned short len)
{
    unsigned short i;

    if ((data == 0) || (len == 0U)) {
        return;
    }

    for (i = 0U; i < len; ++i) {
        unsigned char byte = data[i];

        if ((byte == '\r') || (byte == '\n')) {
            if (app_rx_len != 0U) {
                app_process_rx_frame(app_rx_buffer, app_rx_len);
                app_rx_len = 0U;
            }
            continue;
        }

        if (app_rx_len < (APP_RX_BUFFER_SIZE - 1U)) {
            app_rx_buffer[app_rx_len++] = byte;
        } else {
            app_rx_len = 0U;
        }
    }
}

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len)
{
    (void)topic;
    app_process_rx_frame(payload, len);
}
#endif
