/**
 * @file esp8266_mqtt.h
 * @brief ESP8266 AT-based MQTT client API built on esp8266_port.h.
 */

#ifndef ESP8266_MQTT_H
#define ESP8266_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#define ESP8266_MQTT_BACKEND_GENERIC      0U
#define ESP8266_MQTT_BACKEND_HUAWEI_IOTDA 1U

#define ESP8266_MQTT_SCHEME_TCP 1U

#ifndef ESP8266_MQTT_ENABLE_HUAWEI
#define ESP8266_MQTT_ENABLE_HUAWEI 1
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
    unsigned char backend;      /**< ESP8266_MQTT_BACKEND_*; default 0 is generic MQTT. */
    unsigned char scheme;       /**< AT+MQTTUSERCFG scheme; default 0 maps to TCP scheme 1. */
    const char *huawei_device_id;             /**< Huawei IoTDA device_id; defaults to mqtt_user. */
    const char *huawei_device_secret;         /**< Huawei IoTDA device secret for dynamic HMAC auth. */
    const char *huawei_service_id;            /**< Huawei IoTDA service_id for property reports. */
    const char *huawei_property_report_topic; /**< Optional explicit property report topic. */
    const char *huawei_property_set_topic;    /**< Optional explicit property set subscribe topic. */
    const char *huawei_custom_pub_topic;      /**< Optional extra uplink topic for M2M custom messages. */
    const char *huawei_custom_sub_topic;      /**< Optional extra downlink topic. */
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
 * @brief Publishes a Huawei IoTDA property report.
 *
 * @param properties_json JSON object used as the "properties" object. If it is
 *        already a full {"services":[...]} payload, it is sent unchanged.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_huawei_properties(const char *properties_json);

/**
 * @brief Acknowledges a Huawei IoTDA property-set request.
 *
 * @param request_id Request id extracted from the property-set topic or payload.
 * @param result_code Huawei result_code, normally 0.
 * @param result_desc Text result description, normally "success".
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_send_huawei_property_set_response(const char *request_id,
                                                   int result_code,
                                                   const char *result_desc);

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
