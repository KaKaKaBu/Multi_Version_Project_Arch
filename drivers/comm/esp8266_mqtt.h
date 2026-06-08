/**
 * @file esp8266_mqtt.h
 * @brief ESP8266 AT-based MQTT client API built on esp8266_port.h.
 */

#ifndef ESP8266_MQTT_H
#define ESP8266_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wi-Fi and MQTT connection parameters for esp8266_mqtt_connect().
 */
typedef struct esp8266_mqtt_config {
    const char *wifi_ssid;      /**< Wi-Fi network SSID. */
    const char *wifi_password;  /**< Wi-Fi network password. */
    const char *broker;         /**< MQTT broker hostname or IP address. */
    unsigned short port;        /**< MQTT broker port number. */
    const char *client_id;      /**< MQTT client identifier. */
    const char *mqtt_user;      /**< MQTT username, or empty string. */
    const char *mqtt_password;  /**< MQTT password, or empty string. */
    const char *sub_topic;      /**< Topic to subscribe after connect, or null. */
    const char *pub_topic;      /**< Default publish topic for telemetry helpers. */
} esp8266_mqtt_config_t;

/**
 * @brief Callback invoked when an MQTT message is received.
 *
 * @param topic Null-terminated topic string.
 * @param payload Raw message payload bytes.
 * @param len Payload length in bytes.
 */
typedef void (*esp8266_mqtt_rx_callback_t)(const char *topic,
                                           const unsigned char *payload,
                                           unsigned short len);

/**
 * @brief Connects Wi-Fi and establishes an MQTT session using AT commands.
 *
 * @param cfg Connection parameters; must not be null.
 * @return 0 on success, -1 on failure.
 */
int esp8266_mqtt_connect(const esp8266_mqtt_config_t *cfg);

/**
 * @brief Reports whether the last esp8266_mqtt_connect() succeeded.
 *
 * @return 1 if connected and ready, 0 otherwise.
 */
int esp8266_mqtt_is_ready(void);

/**
 * @brief Returns the active configuration after a successful connect.
 *
 * @return Pointer to stored config, or null if not ready.
 */
const esp8266_mqtt_config_t *esp8266_mqtt_active_config(void);

/**
 * @brief Publishes a raw payload using AT+MQTTPUBRAW.
 *
 * @param topic MQTT topic string.
 * @param payload Raw payload bytes.
 * @param len Payload length in bytes.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_raw(const char *topic,
                             const unsigned char *payload,
                             unsigned short len);

/**
 * @brief Publishes a null-terminated JSON string.
 *
 * @param topic MQTT topic string.
 * @param json_text JSON payload C string.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_json(const char *topic, const char *json_text);

/**
 * @brief Builds and publishes a standard temperature/humidity telemetry JSON message.
 *
 * @param topic MQTT topic; uses cfg.pub_topic when null.
 * @param device Device identifier string embedded in JSON.
 * @param temperature Temperature value in degrees Celsius.
 * @param humidity Relative humidity percentage.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_telemetry(const char *topic,
                                   const char *device,
                                   float temperature,
                                   float humidity);

/**
 * @brief Registers a callback for incoming MQTT subscription messages.
 *
 * @param callback Function to invoke on receive, or null to disable.
 */
void esp8266_mqtt_register_rx_callback(esp8266_mqtt_rx_callback_t callback);

/** @brief Drains USART data and parses incoming MQTT URC lines; call periodically. */
void esp8266_mqtt_poll(void);

#ifdef __cplusplus
}
#endif

#endif
