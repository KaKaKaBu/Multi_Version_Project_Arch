/* KQZL2 通讯回调：负责串口/MQTT 接收组帧，并把 JSON 命令转交业务逻辑。 */
#include "version_config.h"
#include "scheduler.h"
#include "debug_log.h"
#include "app_logic.h"

#define APP_RX_BUFFER_SIZE 256U

static unsigned char app_last_rx_buffer[APP_RX_BUFFER_SIZE];
static unsigned short app_last_rx_len;
static unsigned short app_stream_rx_len;
static unsigned char app_json_depth;
static unsigned char app_json_in_string;
static unsigned char app_json_escape;

static void app_reset_stream_state(void)
{
    app_stream_rx_len = 0U;
    app_json_depth = 0U;
    app_json_in_string = 0U;
    app_json_escape = 0U;
}

static unsigned char app_update_json_state(unsigned char byte)
{
    if (app_json_in_string != 0U) {
        if (app_json_escape != 0U) {
            app_json_escape = 0U;
        } else if (byte == '\\') {
            app_json_escape = 1U;
        } else if (byte == '"') {
            app_json_in_string = 0U;
        }
        return 0U;
    }

    if (byte == '"') {
        app_json_in_string = 1U;
        return 0U;
    }

    if (byte == '{') {
        if (app_json_depth < 0xFFU) {
            ++app_json_depth;
        }
        return 0U;
    }

    if ((byte == '}') && (app_json_depth != 0U)) {
        --app_json_depth;
        return (app_json_depth == 0U) ? 1U : 0U;
    }

    return 0U;
}

static void app_process_rx_frame(const unsigned char *data, unsigned short len)
{
    unsigned short i;
    unsigned short copy_len = len;

    if (copy_len >= APP_RX_BUFFER_SIZE) {
        copy_len = APP_RX_BUFFER_SIZE - 1U;
    }

    for (i = 0U; i < copy_len; ++i) {
        app_last_rx_buffer[i] = data[i];
    }

    app_last_rx_buffer[copy_len] = '\0';
    app_last_rx_len = copy_len;

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    app_logic_on_mqtt_rx((const char *)app_last_rx_buffer);
#endif

    DEBUG_LOG_APP_CB("[APP_CB] rx len=%u\r\n", (unsigned int)copy_len);
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
            if (app_stream_rx_len != 0U) {
                app_process_rx_frame(app_last_rx_buffer, app_stream_rx_len);
                app_reset_stream_state();
            }
            continue;
        }

        if (app_stream_rx_len < (APP_RX_BUFFER_SIZE - 1U)) {
            app_last_rx_buffer[app_stream_rx_len++] = byte;
            if ((app_stream_rx_len != 0U) && (app_update_json_state(byte) != 0U)) {
                app_process_rx_frame(app_last_rx_buffer, app_stream_rx_len);
                app_reset_stream_state();
            }
        } else {
            app_reset_stream_state();
        }
    }

#if VERSION_FEATURE_WIFI
    if (len > 1U) {
        app_process_rx_frame(data, len);
        app_reset_stream_state();
    }
#endif
}

#if VERSION_FEATURE_WIFI
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len)
{
    (void)topic;
    DEBUG_LOG_APP_CB("[APP_CB] mqtt topic=%s len=%u\r\n",
                     (topic != 0) ? topic : "null",
                     (unsigned int)len);
    app_process_rx_frame(payload, len);
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
}

