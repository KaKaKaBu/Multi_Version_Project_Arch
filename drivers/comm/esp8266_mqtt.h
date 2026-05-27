#ifndef ESP8266_MQTT_H
#define ESP8266_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp8266_mqtt_config {
    const char *wifi_ssid;
    const char *wifi_password;
    const char *broker;
    unsigned short port;
    const char *client_id;
    const char *mqtt_user;
    const char *mqtt_password;
    const char *sub_topic;
    const char *pub_topic;
} esp8266_mqtt_config_t;

typedef void (*esp8266_mqtt_rx_callback_t)(const char *topic,
                                           const unsigned char *payload,
                                           unsigned short len);

int esp8266_mqtt_connect(const esp8266_mqtt_config_t *cfg);
int esp8266_mqtt_is_ready(void);
const esp8266_mqtt_config_t *esp8266_mqtt_active_config(void);

int esp8266_mqtt_publish_raw(const char *topic,
                             const unsigned char *payload,
                             unsigned short len);
int esp8266_mqtt_publish_json(const char *topic, const char *json_text);
int esp8266_mqtt_publish_telemetry(const char *topic,
                                   const char *device,
                                   float temperature,
                                   float humidity);

void esp8266_mqtt_register_rx_callback(esp8266_mqtt_rx_callback_t callback);
void esp8266_mqtt_poll(void);

#ifdef __cplusplus
}
#endif

#endif
