/**
 * @file esp8266_mqtt.c
 * @brief ESP8266 AT MQTT client: Wi-Fi join, publish, subscribe, and RX parsing.
 */

#include "esp8266_mqtt.h"
#include "esp8266_port.h"
#include "cjson_port.h"
#include "hmac_sha256.h"
#include "tiny_printf.h"
#include "hal_common.h"
#include "debug_log.h"

#include <string.h>
#include <stdlib.h>

/** @brief Default AT command response wait time in milliseconds. */
#define ESP8266_AT_TIMEOUT_MS 8000U
/** @brief Longer wait for MQTT broker connection/authentication. */
#define ESP8266_MQTT_CONN_TIMEOUT_MS 20000U
/** @brief Timeout for AT+MQTTPUBRAW payload upload and OK response. */
#define ESP8266_MQTT_PUBRAW_TIMEOUT_MS 10000U
/** @brief URC prefix for an incoming MQTT subscription message line. */
#define ESP8266_MQTT_RECV_PREFIX "+MQTTSUBRECV:"
/** @brief Maximum parsed line length when scanning the RX stream. */
#define ESP8266_MQTT_LINE_MAX 512U
/** @brief Maximum topic length retained from MQTT URC messages. */
#define ESP8266_MQTT_TOPIC_MAX 192U
/** @brief Maximum wrapped Huawei IoTDA property report payload length. */
#define ESP8266_HUAWEI_PAYLOAD_MAX 640U
/** @brief Non-zero when esp8266_mqtt_poll() should log trailing stream bytes. */
#define ESP8266_MQTT_POLL_DEBUG DEBUG_LOG_MQTT_POLL_ENABLE
#define ESP8266_HUAWEI_TIMESTAMP_LEN 10U
#define ESP8266_HUAWEI_CLIENT_ID_MAX 96U
#define ESP8266_HUAWEI_PASSWORD_HEX_LEN 64U

/** @brief Active MQTT configuration copied on successful connect. */
static esp8266_mqtt_config_t esp8266_mqtt_cfg;
/** @brief Non-zero after Wi-Fi and MQTT session setup succeed. */
static unsigned char esp8266_mqtt_ready;
/** @brief User callback for delivered subscribe payloads. */
static esp8266_mqtt_rx_callback_t esp8266_mqtt_rx_callback;
/** @brief Non-zero when a multi-line +MQTTSUBRECV payload is pending. */
static unsigned char esp8266_mqtt_pending;
/** @brief Expected payload byte count for a pending delivery. */
static unsigned short esp8266_mqtt_pending_len;
/** @brief Topic name buffer for a pending delivery. */
static char esp8266_mqtt_pending_topic[ESP8266_MQTT_TOPIC_MAX];

static int esp8266_at_cmd(const char *cmd, const char *expect, uint32_t timeout_ms);

static unsigned char esp8266_mqtt_scheme(const esp8266_mqtt_config_t *cfg)
{
    if ((cfg != 0) && (cfg->scheme != 0U)) {
        return cfg->scheme;
    }
    return ESP8266_MQTT_SCHEME_TCP;
}

static int esp8266_mqtt_subscribe_topic(const char *topic)
{
    char cmd[256];

    if ((topic == 0) || (topic[0] == '\0')) {
        return 0;
    }
    if (tiny_snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",0", topic) <= 0) {
        return -1;
    }
    return esp8266_at_cmd(cmd, 0, ESP8266_AT_TIMEOUT_MS);
}

#if ESP8266_MQTT_ENABLE_HUAWEI
static int esp8266_mqtt_is_huawei_backend(void)
{
    return (esp8266_mqtt_cfg.backend == ESP8266_MQTT_BACKEND_HUAWEI_IOTDA) ? 1 : 0;
}

static const char *esp8266_huawei_device_id(void)
{
    if ((esp8266_mqtt_cfg.huawei_device_id != 0) &&
        (esp8266_mqtt_cfg.huawei_device_id[0] != '\0')) {
        return esp8266_mqtt_cfg.huawei_device_id;
    }
    if ((esp8266_mqtt_cfg.mqtt_user != 0) && (esp8266_mqtt_cfg.mqtt_user[0] != '\0')) {
        return esp8266_mqtt_cfg.mqtt_user;
    }
    return (esp8266_mqtt_cfg.client_id != 0) ? esp8266_mqtt_cfg.client_id : "";
}

static const char *esp8266_huawei_service_id(void)
{
    if ((esp8266_mqtt_cfg.huawei_service_id != 0) &&
        (esp8266_mqtt_cfg.huawei_service_id[0] != '\0')) {
        return esp8266_mqtt_cfg.huawei_service_id;
    }
    return esp8266_huawei_device_id();
}

static int esp8266_huawei_topic(char *dst, unsigned int dst_size, const char *suffix)
{
    if ((dst == 0) || (dst_size == 0U) || (suffix == 0)) {
        return -1;
    }
    return (tiny_snprintf(dst,
                          dst_size,
                          "$oc/devices/%s/sys/%s",
                          esp8266_huawei_device_id(),
                          suffix) > 0) ? 0 : -1;
}

static int esp8266_parse_uint(const char **cursor, unsigned int *value)
{
    unsigned int result = 0U;
    unsigned int digits = 0U;

    if ((cursor == 0) || (*cursor == 0) || (value == 0)) {
        return -1;
    }

    while ((**cursor >= '0') && (**cursor <= '9')) {
        result = result * 10U + (unsigned int)(**cursor - '0');
        ++(*cursor);
        ++digits;
    }
    if (digits == 0U) {
        return -1;
    }
    *value = result;
    return 0;
}

static void esp8266_skip_spaces(const char **cursor)
{
    while ((cursor != 0) && (*cursor != 0) && ((**cursor == ' ') || (**cursor == '\t'))) {
        ++(*cursor);
    }
}

static int esp8266_month_from_text(const char *text)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int i;

    if (text == 0) {
        return 0;
    }
    for (i = 0; i < 12; ++i) {
        if (strncmp(text, &months[i * 3], 3U) == 0) {
            return i + 1;
        }
    }
    return 0;
}

static int esp8266_parse_sntp_timestamp(char *timestamp, unsigned int timestamp_size)
{
    char text[ESP8266_MQTT_LINE_MAX];
    const char *cursor;
    const char *month_text;
    unsigned int day;
    unsigned int hour;
    unsigned int year;
    int month;
    uint16_t len;

    if ((timestamp == 0) || (timestamp_size < (ESP8266_HUAWEI_TIMESTAMP_LEN + 1U))) {
        return -1;
    }

    len = esp8266_port_rx_length();
    if (len >= sizeof(text)) {
        len = (uint16_t)(sizeof(text) - 1U);
    }
    memcpy(text, esp8266_port_rx_data(), len);
    text[len] = '\0';

    cursor = strstr(text, "+CIPSNTPTIME:");
    if (cursor == 0) {
        return -1;
    }
    cursor += strlen("+CIPSNTPTIME:");
    esp8266_skip_spaces(&cursor);

    while ((*cursor != ' ') && (*cursor != '\t') && (*cursor != '\0')) {
        ++cursor;
    }
    esp8266_skip_spaces(&cursor);

    month_text = cursor;
    month = esp8266_month_from_text(month_text);
    if (month == 0) {
        return -1;
    }
    cursor += 3U;
    esp8266_skip_spaces(&cursor);

    if (esp8266_parse_uint(&cursor, &day) != 0) {
        return -1;
    }
    esp8266_skip_spaces(&cursor);

    if (esp8266_parse_uint(&cursor, &hour) != 0) {
        return -1;
    }
    while ((*cursor != ' ') && (*cursor != '\t') && (*cursor != '\0')) {
        ++cursor;
    }
    esp8266_skip_spaces(&cursor);

    if (esp8266_parse_uint(&cursor, &year) != 0) {
        return -1;
    }
    if ((year < 2024U) || (day == 0U) || (day > 31U) || (hour > 23U)) {
        return -1;
    }

    return (tiny_snprintf(timestamp,
                          timestamp_size,
                          "%04u%02u%02u%02u",
                          year,
                          (unsigned int)month,
                          day,
                          hour) > 0) ? 0 : -1;
}

static int esp8266_huawei_get_timestamp(char *timestamp, unsigned int timestamp_size)
{
    unsigned int attempt;

    if (esp8266_at_cmd("AT+CIPSNTPCFG=1,0,\"pool.ntp.org\",\"ntp.aliyun.com\"", 0, 3000U) != 0) {
        return -1;
    }

    for (attempt = 0U; attempt < 5U; ++attempt) {
        hal_delay_us(1000000U);
        if (esp8266_at_cmd("AT+CIPSNTPTIME?", "OK", 5000U) == 0) {
            if (esp8266_parse_sntp_timestamp(timestamp, timestamp_size) == 0) {
                DEBUG_LOG_MQTT("[MQTT] Huawei timestamp=%s\r\n", timestamp);
                return 0;
            }
        }
    }
    DEBUG_LOG_MQTT("[MQTT] Huawei SNTP timestamp failed\r\n");
    return -1;
}

static int esp8266_huawei_build_auth(const esp8266_mqtt_config_t *cfg,
                                     char *client_id,
                                     unsigned int client_id_size,
                                     char *password,
                                     unsigned int password_size)
{
    char timestamp[ESP8266_HUAWEI_TIMESTAMP_LEN + 1U];
    const char *device_id;
    const char *secret;

    if ((cfg == 0) || (client_id == 0) || (password == 0) ||
        (password_size < (ESP8266_HUAWEI_PASSWORD_HEX_LEN + 1U))) {
        return -1;
    }

    secret = cfg->huawei_device_secret;
    if ((secret == 0) || (secret[0] == '\0')) {
        return -1;
    }
    if (esp8266_huawei_get_timestamp(timestamp, sizeof(timestamp)) != 0) {
        return -1;
    }

    device_id = esp8266_huawei_device_id();
    if (tiny_snprintf(client_id,
                      client_id_size,
                      "%s_0_0_%s",
                      device_id,
                      timestamp) <= 0) {
        return -1;
    }

    return hmac_sha256_hex((const uint8_t *)timestamp,
                           strlen(timestamp),
                           (const uint8_t *)secret,
                           strlen(secret),
                           password);
}
#endif

/**
 * @brief Sends an AT command and waits for an expected response token.
 * @param cmd AT command string without line ending.
 * @param expect Response substring; uses "OK" when null.
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return 0 on success, -1 on timeout or ERROR.
 */
static int esp8266_at_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    if (cmd == 0) {
        return -1;
    }

    DEBUG_LOG_MQTT_AT("[MQTT_AT] send: %s expect=%s timeout=%lu\r\n",
                      cmd,
                      (expect != 0) ? expect : "OK",
                      (unsigned long)timeout_ms);
    esp8266_port_rx_clear();
    if (esp8266_port_send_str(cmd) < 0) {
        return -1;
    }
    if (esp8266_port_send_str("\r\n") < 0) {
        return -1;
    }

    if (expect == 0) {
        expect = "OK";
    }

    if (esp8266_port_wait_token(expect, timeout_ms) == 0) {
        DEBUG_LOG_MQTT_AT("[MQTT_AT] timeout: %s stream_len=%u\r\n",
                          cmd,
                          (unsigned int)esp8266_port_rx_length());
        if (esp8266_port_wait_token("ERROR", 200U) != 0) {
            DEBUG_LOG_MQTT_AT("[MQTT_AT] ERROR: %s\r\n", cmd);
            return -1;
        }
        return -1;
    }

    DEBUG_LOG_MQTT_AT("[MQTT_AT] ok: %s\r\n", cmd);
    return 0;
}

/**
 * @brief Joins a Wi-Fi access point using AT+CWJAP.
 * @param ssid Network SSID; must not be null.
 * @param password Network password; must not be null.
 * @return 0 on success, -1 on failure.
 */
static int esp8266_wifi_connect(const char *ssid, const char *password)
{
    char cmd[160];

    if ((ssid == 0) || (password == 0)) {
        return -1;
    }

    if (esp8266_at_cmd("AT", 0, 2000U) != 0) {
        return -1;
    }
    (void)esp8266_at_cmd("ATE0", 0, 2000U);
    if (esp8266_at_cmd("AT+CIPMUX=0", 0, 2000U) != 0) {
        return -1;
    }
    if (esp8266_at_cmd("AT+CWMODE=1", 0, 3000U) != 0) {
        return -1;
    }
    if (esp8266_at_cmd("AT+CWDHCP=1,1", 0, 2000U) != 0) {
        return -1;
    }

    if (tiny_snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password) <= 0) {
        return -1;
    }

    if (esp8266_at_cmd(cmd, "OK", 15000U) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Configures MQTT user credentials, connects, and subscribes.
 * @param cfg MQTT parameters; must not be null.
 * @return 0 on success, -1 on failure.
 */
static int esp8266_mqtt_setup(const esp8266_mqtt_config_t *cfg)
{
    char cmd[256];
    const char *client_id;
    const char *mqtt_user;
    const char *mqtt_password;
#if ESP8266_MQTT_ENABLE_HUAWEI
    char huawei_client_id[ESP8266_HUAWEI_CLIENT_ID_MAX];
    char huawei_password[ESP8266_HUAWEI_PASSWORD_HEX_LEN + 1U];
#endif

    if (cfg == 0) {
        return -1;
    }

    client_id = (cfg->client_id != 0) ? cfg->client_id : "";
    mqtt_user = (cfg->mqtt_user != 0) ? cfg->mqtt_user : "";
    mqtt_password = (cfg->mqtt_password != 0) ? cfg->mqtt_password : "";

#if ESP8266_MQTT_ENABLE_HUAWEI
    if ((cfg->backend == ESP8266_MQTT_BACKEND_HUAWEI_IOTDA) &&
        (cfg->huawei_device_secret != 0) &&
        (cfg->huawei_device_secret[0] != '\0')) {
        if (esp8266_huawei_build_auth(cfg,
                                      huawei_client_id,
                                      sizeof(huawei_client_id),
                                      huawei_password,
                                      sizeof(huawei_password)) != 0) {
            return -1;
        }
        client_id = huawei_client_id;
        mqtt_user = esp8266_huawei_device_id();
        mqtt_password = huawei_password;
    }
#endif

    if (tiny_snprintf(cmd, sizeof(cmd),
                      "AT+MQTTUSERCFG=0,%u,\"%s\",\"%s\",\"%s\",0,0,\"\"",
                      (unsigned int)esp8266_mqtt_scheme(cfg),
                      client_id,
                      mqtt_user,
                      mqtt_password) <= 0) {
        return -1;
    }

    if (esp8266_at_cmd(cmd, 0, ESP8266_AT_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (tiny_snprintf(cmd, sizeof(cmd),
                      "AT+MQTTCONN=0,\"%s\",%u,0",
                      cfg->broker,
                      (unsigned int)cfg->port) <= 0) {
        return -1;
    }

    if (esp8266_at_cmd(cmd, "+MQTTCONNECTED", ESP8266_MQTT_CONN_TIMEOUT_MS) != 0) {
        return -1;
    }

    if ((cfg->sub_topic != 0) && (cfg->sub_topic[0] != '\0')) {
        if (esp8266_mqtt_subscribe_topic(cfg->sub_topic) != 0) {
            return -1;
        }
    }

#if ESP8266_MQTT_ENABLE_HUAWEI
    if ((cfg->backend == ESP8266_MQTT_BACKEND_HUAWEI_IOTDA) &&
        (cfg->huawei_custom_sub_topic != 0) &&
        (cfg->huawei_custom_sub_topic[0] != '\0') &&
        ((cfg->sub_topic == 0) || (strcmp(cfg->sub_topic, cfg->huawei_custom_sub_topic) != 0))) {
        if (esp8266_mqtt_subscribe_topic(cfg->huawei_custom_sub_topic) != 0) {
            return -1;
        }
    }
#endif

    return 0;
}

/**
 * @brief Connects Wi-Fi and establishes an MQTT session using AT commands.
 * @param cfg Connection parameters; must not be null.
 * @return 0 on success, -1 on failure.
 */
int esp8266_mqtt_connect(const esp8266_mqtt_config_t *cfg)
{
    esp8266_mqtt_ready = 0U;

    if (cfg == 0) {
        DEBUG_LOG_MQTT("[MQTT] connect cfg null\r\n");
        return -1;
    }

    esp8266_mqtt_cfg = *cfg;
    DEBUG_LOG_MQTT("[MQTT] connect broker=%s port=%u sub=%s pub=%s\r\n",
                   cfg->broker,
                   (unsigned int)cfg->port,
                   (cfg->sub_topic != 0) ? cfg->sub_topic : "",
                   (cfg->pub_topic != 0) ? cfg->pub_topic : "");

    if (esp8266_wifi_connect(cfg->wifi_ssid, cfg->wifi_password) != 0) {
        DEBUG_LOG_MQTT("[MQTT] wifi connect failed\r\n");
        return -1;
    }
    if (esp8266_mqtt_setup(cfg) != 0) {
        DEBUG_LOG_MQTT("[MQTT] mqtt setup failed\r\n");
        return -1;
    }

    esp8266_mqtt_ready = 1U;
    esp8266_port_rx_clear();
    DEBUG_LOG_MQTT("[MQTT] connect ready\r\n");
    return 0;
}

/**
 * @brief Reports whether the last esp8266_mqtt_connect() succeeded.
 * @return 1 if connected and ready, 0 otherwise.
 */
int esp8266_mqtt_is_ready(void)
{
    return (esp8266_mqtt_ready != 0U) ? 1 : 0;
}

/**
 * @brief Returns the active configuration after a successful connect.
 * @return Pointer to stored config, or null if not ready.
 */
const esp8266_mqtt_config_t *esp8266_mqtt_active_config(void)
{
    if (esp8266_mqtt_ready == 0U) {
        return 0;
    }

    return &esp8266_mqtt_cfg;
}

/**
 * @brief Publishes a raw payload using AT+MQTTPUBRAW.
 * @param topic MQTT topic string.
 * @param payload Raw payload bytes.
 * @param len Payload length in bytes.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_raw(const char *topic,
                             const unsigned char *payload,
                             unsigned short len)
{
    char cmd[128];

    if ((esp8266_mqtt_ready == 0U) || (topic == 0) || (payload == 0) || (len == 0U)) {
        return -1;
    }

    if (tiny_snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%u,0,0",
                      topic, (unsigned int)len) <= 0) {
        return -1;
    }

    esp8266_port_rx_clear();
    if (esp8266_port_send_str(cmd) < 0) {
        return -1;
    }
    if (esp8266_port_send_str("\r\n") < 0) {
        return -1;
    }

    if (esp8266_port_wait_prompt(ESP8266_MQTT_PUBRAW_TIMEOUT_MS) == 0) {
        return -1;
    }

    if (esp8266_port_send(payload, len) < 0) {
        return -1;
    }

    if (esp8266_port_wait_token("OK", ESP8266_MQTT_PUBRAW_TIMEOUT_MS) == 0) {
        return -1;
    }

    return (int)len;
}

/**
 * @brief Publishes a null-terminated JSON string.
 * @param topic MQTT topic string.
 * @param json_text JSON payload C string.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_json(const char *topic, const char *json_text)
{
    if (json_text == 0) {
        return -1;
    }

    return esp8266_mqtt_publish_raw(topic,
                                    (const unsigned char *)json_text,
                                    (unsigned short)strlen(json_text));
}

int esp8266_mqtt_publish_huawei_properties(const char *properties_json)
{
#if ESP8266_MQTT_ENABLE_HUAWEI
    char topic[ESP8266_MQTT_TOPIC_MAX];
    char payload[ESP8266_HUAWEI_PAYLOAD_MAX];
    const char *publish_topic;
    const char *body;

    if ((properties_json == 0) || (esp8266_mqtt_ready == 0U) ||
        (esp8266_mqtt_is_huawei_backend() == 0)) {
        return -1;
    }

    publish_topic = esp8266_mqtt_cfg.huawei_property_report_topic;
    if ((publish_topic == 0) || (publish_topic[0] == '\0')) {
        if (esp8266_huawei_topic(topic, sizeof(topic), "properties/report") != 0) {
            return -1;
        }
        publish_topic = topic;
    }

    body = properties_json;
    while ((*body == ' ') || (*body == '\t') || (*body == '\r') || (*body == '\n')) {
        ++body;
    }
    if (strncmp(body, "{\"services\"", 11U) == 0) {
        return esp8266_mqtt_publish_json(publish_topic, body);
    }

    if (tiny_snprintf(payload,
                      sizeof(payload),
                      "{\"services\":[{\"service_id\":\"%s\",\"properties\":%s}]}",
                      esp8266_huawei_service_id(),
                      body) <= 0) {
        return -1;
    }
    return esp8266_mqtt_publish_json(publish_topic, payload);
#else
    (void)properties_json;
    return -1;
#endif
}

int esp8266_mqtt_send_huawei_property_set_response(const char *request_id,
                                                   int result_code,
                                                   const char *result_desc)
{
#if ESP8266_MQTT_ENABLE_HUAWEI
    char topic[ESP8266_MQTT_TOPIC_MAX];
    char suffix[128];
    char payload[96];

    if ((request_id == 0) || (request_id[0] == '\0') ||
        (esp8266_mqtt_ready == 0U) || (esp8266_mqtt_is_huawei_backend() == 0)) {
        return -1;
    }

    if ((result_desc == 0) || (result_desc[0] == '\0')) {
        result_desc = "success";
    }
    if (tiny_snprintf(suffix,
                      sizeof(suffix),
                      "properties/set/response/request_id=%s",
                      request_id) <= 0) {
        return -1;
    }
    if (esp8266_huawei_topic(topic, sizeof(topic), suffix) != 0) {
        return -1;
    }
    if (tiny_snprintf(payload,
                      sizeof(payload),
                      "{\"result_code\":%d,\"result_desc\":\"%s\"}",
                      result_code,
                      result_desc) <= 0) {
        return -1;
    }
    return esp8266_mqtt_publish_json(topic, payload);
#else
    (void)request_id;
    (void)result_code;
    (void)result_desc;
    return -1;
#endif
}

/**
 * @brief Builds and publishes a standard temperature/humidity telemetry JSON message.
 * @param topic MQTT topic; uses cfg.pub_topic when null.
 * @param device Device identifier string embedded in JSON.
 * @param temperature Temperature value in degrees Celsius.
 * @param humidity Relative humidity percentage.
 * @return Number of bytes published on success, -1 on error.
 */
int esp8266_mqtt_publish_telemetry(const char *topic,
                                   const char *device,
                                   float temperature,
                                   float humidity)
{
    char *json_text;
    int rc;

    json_text = cjson_build_telemetry(device, temperature, humidity);
    if (json_text == 0) {
        return -1;
    }

    if ((topic == 0) && (esp8266_mqtt_cfg.pub_topic != 0)) {
        topic = esp8266_mqtt_cfg.pub_topic;
    }

    rc = esp8266_mqtt_publish_json(topic, json_text);
    cjson_release_string(json_text);
    return rc;
}

/**
 * @brief Registers a callback for incoming MQTT subscription messages.
 * @param callback Function to invoke on receive, or null to disable.
 */
void esp8266_mqtt_register_rx_callback(esp8266_mqtt_rx_callback_t callback)
{
    esp8266_mqtt_rx_callback = callback;
    DEBUG_LOG_MQTT("[MQTT] rx callback=%s\r\n", (callback != 0) ? "set" : "null");
}

/**
 * @brief Locates the payload pointer in a +MQTTSUBRECV line with inline data.
 * @param line Full receive line from the module.
 * @param payload_len Output payload length parsed from the line.
 * @return Pointer to payload bytes, or null when not inline.
 */
static const char *esp8266_mqtt_find_payload_start(const char *line, unsigned short *payload_len)
{
    const char *cursor;
    const char *topic_end;
    const char *len_start;
    unsigned short length = 0U;

    if ((line == 0) || (payload_len == 0)) {
        return 0;
    }

    cursor = strchr(line, ',');
    if (cursor == 0) {
        return 0;
    }
    ++cursor;

    if (*cursor != '"') {
        return 0;
    }
    ++cursor;
    topic_end = strchr(cursor, '"');
    if (topic_end == 0) {
        return 0;
    }

    len_start = topic_end + 1U;
    if (*len_start != ',') {
        return 0;
    }
    ++len_start;

    while ((*len_start >= '0') && (*len_start <= '9')) {
        length = (unsigned short)(length * 10U + (unsigned short)(*len_start - '0'));
        ++len_start;
    }

    *payload_len = length;
    if (*len_start != ',') {
        return 0;
    }

    return len_start + 1U;
}

/**
 * @brief Invokes the registered RX callback when parameters are valid.
 * @param topic Null-terminated topic string.
 * @param payload Payload bytes (not null-terminated).
 * @param payload_len Payload length in bytes.
 */
static void esp8266_mqtt_deliver(const char *topic,
                                 const unsigned char *payload,
                                 unsigned short payload_len)
{
    if ((esp8266_mqtt_rx_callback == 0) || (topic == 0) || (payload == 0) || (payload_len == 0U)) {
        DEBUG_LOG_MQTT("[MQTT] deliver skipped cb=%s topic=%s payload=%s len=%u\r\n",
                       (esp8266_mqtt_rx_callback != 0) ? "yes" : "no",
                       (topic != 0) ? topic : "null",
                       (payload != 0) ? "yes" : "null",
                       (unsigned int)payload_len);
        return;
    }

    DEBUG_LOG_MQTT("[MQTT] deliver topic=%s len=%u\r\n", topic, (unsigned int)payload_len);
    esp8266_mqtt_rx_callback(topic, payload, payload_len);
}

#if ESP8266_MQTT_ENABLE_HUAWEI
static int esp8266_huawei_extract_request_id(const char *text,
                                             char *request_id,
                                             unsigned short request_id_size)
{
    const char *start;
    const char *end;
    unsigned short len;

    if ((text == 0) || (request_id == 0) || (request_id_size == 0U)) {
        return 0;
    }

    start = strstr(text, "request_id=");
    if (start == 0) {
        return 0;
    }
    start += strlen("request_id=");
    end = start;
    while ((*end != '"') && (*end != ',') && (*end != '\r') &&
           (*end != '\n') && (*end != '\0')) {
        ++end;
    }

    len = (unsigned short)(end - start);
    if ((len == 0U) || (len >= request_id_size)) {
        return 0;
    }
    memcpy(request_id, start, len);
    request_id[len] = '\0';
    return 1;
}
#endif

/** @brief Clears multi-line payload reassembly state. */
static void esp8266_mqtt_clear_pending(void)
{
    esp8266_mqtt_pending = 0U;
    esp8266_mqtt_pending_len = 0U;
    esp8266_mqtt_pending_topic[0] = '\0';
}

/**
 * @brief Handles a follow-on line as payload for a pending subscription.
 * @param line Raw payload line from the module.
 */
static void esp8266_mqtt_handle_payload_line(const char *line)
{
    unsigned short line_len;
    unsigned short copy_len;

    if ((esp8266_mqtt_pending == 0U) || (line == 0)) {
        return;
    }

    line_len = (unsigned short)strlen(line);
    if (line_len == 0U) {
        return;
    }

    copy_len = line_len;
    if ((esp8266_mqtt_pending_len != 0U) && (copy_len > esp8266_mqtt_pending_len)) {
        copy_len = esp8266_mqtt_pending_len;
    }

    esp8266_mqtt_deliver(esp8266_mqtt_pending_topic,
                         (const unsigned char *)line,
                         copy_len);
    esp8266_mqtt_clear_pending();
}

/**
 * @brief Parses one USART line for +MQTTSUBRECV headers or pending payload.
 * @param line Complete CRLF-delimited line from the RX stream.
 */
static void esp8266_mqtt_handle_line(const char *line)
{
    const char *payload;
    const char *topic_start;
    const char *topic_end;
    unsigned short payload_len = 0U;
    unsigned short actual_len;
    char topic_buf[64];

    if (line == 0) {
        return;
    }

    if (line[0] == '\0') {
        return;
    }

    if (strstr(line, "+MQTTDISCONNECTED:") != 0) {
        esp8266_mqtt_ready = 0U;
        esp8266_mqtt_clear_pending();
        return;
    }

    if (strncmp(line, ESP8266_MQTT_RECV_PREFIX, strlen(ESP8266_MQTT_RECV_PREFIX)) != 0) {
        if (esp8266_mqtt_pending != 0U) {
            esp8266_mqtt_handle_payload_line(line);
        }
        return;
    }

    DEBUG_LOG_MQTT("[MQTT] recv line: %s\r\n", line);

    topic_start = strchr(line, '"');
    if (topic_start == 0) {
        return;
    }
    ++topic_start;
    topic_end = strchr(topic_start, '"');
    if (topic_end == 0) {
        return;
    }

    if ((unsigned short)(topic_end - topic_start) >= sizeof(topic_buf)) {
        return;
    }

    memcpy(topic_buf, topic_start, (size_t)(topic_end - topic_start));
    topic_buf[topic_end - topic_start] = '\0';

    payload = esp8266_mqtt_find_payload_start(line, &payload_len);
    if (payload == 0) {
        const char *len_cursor = topic_end + 1U;

        if (*len_cursor != ',') {
            return;
        }
        ++len_cursor;
        payload_len = 0U;
        while ((*len_cursor >= '0') && (*len_cursor <= '9')) {
            payload_len = (unsigned short)(payload_len * 10U + (unsigned short)(*len_cursor - '0'));
            ++len_cursor;
        }

        if (payload_len == 0U) {
            return;
        }

        esp8266_mqtt_pending = 1U;
        esp8266_mqtt_pending_len = payload_len;
        memcpy(esp8266_mqtt_pending_topic, topic_buf, (size_t)(topic_end - topic_start));
        esp8266_mqtt_pending_topic[topic_end - topic_start] = '\0';
        esp8266_mqtt_pending_topic[sizeof(esp8266_mqtt_pending_topic) - 1U] = '\0';
        return;
    }

    actual_len = (unsigned short)strlen(payload);
    if ((payload_len == 0U) || (actual_len == 0U)) {
        esp8266_mqtt_pending = 1U;
        esp8266_mqtt_pending_len = payload_len;
        memcpy(esp8266_mqtt_pending_topic, topic_buf, (size_t)(topic_end - topic_start));
        esp8266_mqtt_pending_topic[topic_end - topic_start] = '\0';
        esp8266_mqtt_pending_topic[sizeof(esp8266_mqtt_pending_topic) - 1U] = '\0';
        return;
    }

    if (actual_len < payload_len) {
        payload_len = actual_len;
    }

    esp8266_mqtt_deliver(topic_buf, (const unsigned char *)payload, payload_len);
#if ESP8266_MQTT_ENABLE_HUAWEI
    if ((esp8266_mqtt_is_huawei_backend() != 0) &&
        (strstr(topic_buf, "/sys/properties/set") != 0)) {
        char request_id[80];
        if (esp8266_huawei_extract_request_id(topic_buf, request_id, sizeof(request_id)) != 0) {
            (void)esp8266_mqtt_send_huawei_property_set_response(request_id, 0, "success");
        }
    }
#endif
    esp8266_mqtt_clear_pending();
}

/** @brief Extracts complete CRLF-delimited lines from the RX stream buffer. */
static void esp8266_mqtt_parse_stream(void)
{
    uint16_t i = 0U;

    while (i < esp8266_port_rx_length()) {
        if ((esp8266_port_rx_data()[i] == '\r') || (esp8266_port_rx_data()[i] == '\n')) {
            char line[ESP8266_MQTT_LINE_MAX];
            uint16_t line_len = i;

            if (line_len >= (ESP8266_MQTT_LINE_MAX - 1U)) {
                line_len = (uint16_t)(ESP8266_MQTT_LINE_MAX - 2U);
            }

            if (line_len > 0U) {
                memcpy(line, esp8266_port_rx_data(), line_len);
                line[line_len] = '\0';
                esp8266_mqtt_handle_line(line);
            }
            esp8266_port_rx_discard((uint16_t)(i + 1U));
            i = 0U;
            continue;
        }

        ++i;
    }
}

/**
 * @brief Returns 1 if the RX stream contains at least one complete line.
 * @return 1 when a CRLF delimiter is present; 0 otherwise.
 */
static int esp8266_mqtt_stream_has_line(void)
{
    uint16_t i;

    for (i = 0U; i < esp8266_port_rx_length(); ++i) {
        if ((esp8266_port_rx_data()[i] == '\r') || (esp8266_port_rx_data()[i] == '\n')) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Drains USART data and parses incoming MQTT URC lines; call periodically.
 */
void esp8266_mqtt_poll(void)
{
#if ESP8266_MQTT_POLL_DEBUG
    static uint16_t last_tail_len;
#endif
    uint16_t tail_len;

    esp8266_port_drain_rx();
    if ((esp8266_mqtt_stream_has_line() != 0) || (esp8266_mqtt_pending != 0U)) {
        esp8266_mqtt_parse_stream();
    }
    tail_len = esp8266_port_rx_length();
#if ESP8266_MQTT_POLL_DEBUG
    if ((tail_len != 0U) && (tail_len != last_tail_len)) {
        uint16_t i;
        uint16_t preview_len = tail_len;
        char preview[33];

        if (preview_len > 32U) {
            preview_len = 32U;
        }

        for (i = 0U; i < preview_len; ++i) {
            uint8_t byte = esp8266_port_rx_data()[i];
            preview[i] = ((byte >= 32U) && (byte <= 126U)) ? (char)byte : '.';
        }
        preview[preview_len] = '\0';
        DEBUG_LOG_MQTT_POLL("[MQTT] stream tail len=%u data=%s\r\n",
                            (unsigned int)tail_len,
                            preview);
    }
    last_tail_len = tail_len;
#else
    (void)tail_len;
#endif
}
