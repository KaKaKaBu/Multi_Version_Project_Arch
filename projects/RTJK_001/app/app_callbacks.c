#include "version_config.h"
#include "scheduler.h"

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

    app_last_rx_len = copy_len;
    event_set(APP_EVENT_COMM_RX);
}

#if VERSION_FEATURE_WIFI
void app_mqtt_rx_callback(const char *topic,
                          const unsigned char *payload,
                          unsigned short len)
{
    (void)topic;
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
