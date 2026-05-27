#include "esp8266_mqtt.h"
#include "esp8266_port.h"
#include "cjson_port.h"
#include "tiny_printf.h"
#include "hal_common.h"
#include "debug_log.h"

#include <string.h>
#include <stdlib.h>

#define ESP8266_AT_TIMEOUT_MS 8000U
#define ESP8266_MQTT_PUBRAW_TIMEOUT_MS 10000U
#define ESP8266_MQTT_RECV_PREFIX "+MQTTSUBRECV:"
#define ESP8266_MQTT_LINE_MAX 512U
#define ESP8266_MQTT_POLL_DEBUG DEBUG_LOG_MQTT_POLL_ENABLE

static esp8266_mqtt_config_t esp8266_mqtt_cfg;
static unsigned char esp8266_mqtt_ready;
static esp8266_mqtt_rx_callback_t esp8266_mqtt_rx_callback;
static unsigned char esp8266_mqtt_pending;
static unsigned short esp8266_mqtt_pending_len;
static char esp8266_mqtt_pending_topic[64];

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

static int esp8266_wifi_connect(const char *ssid, const char *password)
{
    char cmd[160];

    if ((ssid == 0) || (password == 0)) {
        return -1;
    }

    if (esp8266_at_cmd("AT", 0, 2000U) != 0) {
        return -1;
    }
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

    if (esp8266_at_cmd(cmd, "OK", 5000U) != 0) {
        return -1;
    }

    return 0;
}

static int esp8266_mqtt_setup(const esp8266_mqtt_config_t *cfg)
{
    char cmd[256];

    if (cfg == 0) {
        return -1;
    }

    if (tiny_snprintf(cmd, sizeof(cmd),
                      "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
                      (cfg->client_id != 0) ? cfg->client_id : "",
                      (cfg->mqtt_user != 0) ? cfg->mqtt_user : "",
                      (cfg->mqtt_password != 0) ? cfg->mqtt_password : "") <= 0) {
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

    if (esp8266_at_cmd(cmd, "OK", ESP8266_AT_TIMEOUT_MS) != 0) {
        return -1;
    }

    if ((cfg->sub_topic != 0) && (cfg->sub_topic[0] != '\0')) {
        if (tiny_snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",0", cfg->sub_topic) <= 0) {
            return -1;
        }
        if (esp8266_at_cmd(cmd, 0, ESP8266_AT_TIMEOUT_MS) != 0) {
            return -1;
        }
    }

    return 0;
}

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

int esp8266_mqtt_is_ready(void)
{
    return (esp8266_mqtt_ready != 0U) ? 1 : 0;
}

const esp8266_mqtt_config_t *esp8266_mqtt_active_config(void)
{
    if (esp8266_mqtt_ready == 0U) {
        return 0;
    }

    return &esp8266_mqtt_cfg;
}

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

int esp8266_mqtt_publish_json(const char *topic, const char *json_text)
{
    if (json_text == 0) {
        return -1;
    }

    return esp8266_mqtt_publish_raw(topic,
                                    (const unsigned char *)json_text,
                                    (unsigned short)strlen(json_text));
}

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

void esp8266_mqtt_register_rx_callback(esp8266_mqtt_rx_callback_t callback)
{
    esp8266_mqtt_rx_callback = callback;
    DEBUG_LOG_MQTT("[MQTT] rx callback=%s\r\n", (callback != 0) ? "set" : "null");
}

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

static void esp8266_mqtt_clear_pending(void)
{
    esp8266_mqtt_pending = 0U;
    esp8266_mqtt_pending_len = 0U;
    esp8266_mqtt_pending_topic[0] = '\0';
}

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
        memcpy(esp8266_mqtt_pending_topic, topic_buf, sizeof(esp8266_mqtt_pending_topic));
        esp8266_mqtt_pending_topic[sizeof(esp8266_mqtt_pending_topic) - 1U] = '\0';
        return;
    }

    actual_len = (unsigned short)strlen(payload);
    if ((payload_len == 0U) || (actual_len == 0U)) {
        esp8266_mqtt_pending = 1U;
        esp8266_mqtt_pending_len = payload_len;
        memcpy(esp8266_mqtt_pending_topic, topic_buf, sizeof(esp8266_mqtt_pending_topic));
        esp8266_mqtt_pending_topic[sizeof(esp8266_mqtt_pending_topic) - 1U] = '\0';
        return;
    }

    if (actual_len < payload_len) {
        payload_len = actual_len;
    }

    esp8266_mqtt_deliver(topic_buf, (const unsigned char *)payload, payload_len);
    esp8266_mqtt_clear_pending();
}

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
