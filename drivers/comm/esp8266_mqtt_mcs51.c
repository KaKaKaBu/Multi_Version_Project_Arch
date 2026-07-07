#include "esp8266_mqtt.h"
#include "esp8266_port.h"
#include "hal_common.h"
#include "mcs51_memory.h"

#define ESP8266_AT_TIMEOUT_MS 8000U
#define ESP8266_MQTT_PUBRAW_TIMEOUT_MS 10000U
#define ESP8266_MCS51_CMD_MAX 256U
#define ESP8266_MCS51_TOPIC_MAX 160U
#define ESP8266_MCS51_HUAWEI_PAYLOAD_MAX 320U

static esp8266_mqtt_config_t esp8266_mqtt_cfg;
static unsigned char esp8266_mqtt_ready;
static esp8266_mqtt_rx_callback_t esp8266_mqtt_rx_callback;
static MCS51_XDATA char esp8266_cmd[ESP8266_MCS51_CMD_MAX];
static MCS51_XDATA char esp8266_topic[ESP8266_MCS51_TOPIC_MAX];
#if ESP8266_MQTT_ENABLE_HUAWEI
static MCS51_XDATA char esp8266_huawei_payload[ESP8266_MCS51_HUAWEI_PAYLOAD_MAX];
#endif

static char *mqtt_puts(char *dst, const char *src)
{
    while ((src != 0) && (*src != '\0')) {
        *dst = *src;
        ++dst;
        ++src;
    }
    *dst = '\0';
    return dst;
}

static char *mqtt_put_u16(char *dst, unsigned short value)
{
    char buf[5];
    unsigned char i = 0U;

    if (value == 0U) {
        *dst = '0';
        ++dst;
        *dst = '\0';
        return dst;
    }

    while ((value > 0U) && (i < sizeof(buf))) {
        buf[i] = (char)('0' + (value % 10U));
        value = (unsigned short)(value / 10U);
        ++i;
    }

    while (i > 0U) {
        --i;
        *dst = buf[i];
        ++dst;
    }
    *dst = '\0';
    return dst;
}

static unsigned short mqtt_strlen(const char *text)
{
    unsigned short len = 0U;

    while ((text != 0) && (text[len] != '\0')) {
        ++len;
    }
    return len;
}

static unsigned char esp8266_mqtt_scheme(const esp8266_mqtt_config_t *cfg)
{
    if ((cfg != 0) && (cfg->scheme != 0U)) {
        return cfg->scheme;
    }
    return ESP8266_MQTT_SCHEME_TCP;
}

static unsigned char esp8266_mqtt_is_huawei_backend(void)
{
    return (esp8266_mqtt_cfg.backend == ESP8266_MQTT_BACKEND_HUAWEI_IOTDA) ? 1U : 0U;
}

static int esp8266_at_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    if (cmd == 0) {
        return -1;
    }

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
    return (esp8266_port_wait_token(expect, timeout_ms) != 0) ? 0 : -1;
}

static int esp8266_wifi_connect(const char *ssid, const char *password)
{
    char *p;

    if ((ssid == 0) || (password == 0)) {
        return -1;
    }

    if (esp8266_at_cmd("AT", 0, 2000U) != 0) {
        return -1;
    }
    (void)esp8266_at_cmd("ATE0", 0, 2000U);
    if (esp8266_at_cmd("AT+CWMODE=1", 0, 3000U) != 0) {
        return -1;
    }
    if (esp8266_at_cmd("AT+CIPMUX=0", 0, 2000U) != 0) {
        return -1;
    }

    p = esp8266_cmd;
    p = mqtt_puts(p, "AT+CWJAP=\"");
    p = mqtt_puts(p, ssid);
    p = mqtt_puts(p, "\",\"");
    p = mqtt_puts(p, password);
    (void)mqtt_puts(p, "\"");

    return esp8266_at_cmd(esp8266_cmd, "OK", 15000U);
}

static int esp8266_mqtt_setup(const esp8266_mqtt_config_t *cfg)
{
    char *p;

    p = esp8266_cmd;
    p = mqtt_puts(p, "AT+MQTTUSERCFG=0,");
    p = mqtt_put_u16(p, esp8266_mqtt_scheme(cfg));
    p = mqtt_puts(p, ",\"");
    p = mqtt_puts(p, (cfg->client_id != 0) ? cfg->client_id : "");
    p = mqtt_puts(p, "\",\"");
    p = mqtt_puts(p, (cfg->mqtt_user != 0) ? cfg->mqtt_user : "");
    p = mqtt_puts(p, "\",\"");
    p = mqtt_puts(p, (cfg->mqtt_password != 0) ? cfg->mqtt_password : "");
    (void)mqtt_puts(p, "\",0,0,\"\"");
    if (esp8266_at_cmd(esp8266_cmd, 0, ESP8266_AT_TIMEOUT_MS) != 0) {
        return -1;
    }

    p = esp8266_cmd;
    p = mqtt_puts(p, "AT+MQTTCONN=0,\"");
    p = mqtt_puts(p, cfg->broker);
    p = mqtt_puts(p, "\",");
    p = mqtt_put_u16(p, cfg->port);
    (void)mqtt_puts(p, ",0");
    if (esp8266_at_cmd(esp8266_cmd, "OK", ESP8266_AT_TIMEOUT_MS) != 0) {
        return -1;
    }

    if ((cfg->sub_topic != 0) && (cfg->sub_topic[0] != '\0')) {
        p = esp8266_cmd;
        p = mqtt_puts(p, "AT+MQTTSUB=0,\"");
        p = mqtt_puts(p, cfg->sub_topic);
        (void)mqtt_puts(p, "\",0");
        if (esp8266_at_cmd(esp8266_cmd, 0, ESP8266_AT_TIMEOUT_MS) != 0) {
            return -1;
        }
    }

#if ESP8266_MQTT_ENABLE_HUAWEI
    if ((cfg->backend == ESP8266_MQTT_BACKEND_HUAWEI_IOTDA) &&
        (cfg->huawei_custom_sub_topic != 0) &&
        (cfg->huawei_custom_sub_topic[0] != '\0')) {
        p = esp8266_cmd;
        p = mqtt_puts(p, "AT+MQTTSUB=0,\"");
        p = mqtt_puts(p, cfg->huawei_custom_sub_topic);
        (void)mqtt_puts(p, "\",0");
        if (esp8266_at_cmd(esp8266_cmd, 0, ESP8266_AT_TIMEOUT_MS) != 0) {
            return -1;
        }
    }
#endif

    return 0;
}

int esp8266_mqtt_connect(const esp8266_mqtt_config_t *cfg)
{
    esp8266_mqtt_ready = 0U;
    if ((cfg == 0) || (cfg->broker == 0)) {
        return -1;
    }

    esp8266_mqtt_cfg = *cfg;
    if (esp8266_wifi_connect(cfg->wifi_ssid, cfg->wifi_password) != 0) {
        return -1;
    }
    if (esp8266_mqtt_setup(cfg) != 0) {
        return -1;
    }

    esp8266_mqtt_ready = 1U;
    esp8266_port_rx_clear();
    return 0;
}

int esp8266_mqtt_is_ready(void)
{
    return (esp8266_mqtt_ready != 0U) ? 1 : 0;
}

const esp8266_mqtt_config_t *esp8266_mqtt_active_config(void)
{
    return (esp8266_mqtt_ready != 0U) ? &esp8266_mqtt_cfg : 0;
}

int esp8266_mqtt_publish_raw(const char *topic,
                             const unsigned char *payload,
                             unsigned short len)
{
    char *p;

    if ((esp8266_mqtt_ready == 0U) || (topic == 0) || (payload == 0) || (len == 0U)) {
        return -1;
    }

    p = esp8266_cmd;
    p = mqtt_puts(p, "AT+MQTTPUBRAW=0,\"");
    p = mqtt_puts(p, topic);
    p = mqtt_puts(p, "\",");
    p = mqtt_put_u16(p, len);
    (void)mqtt_puts(p, ",0,0");

    esp8266_port_rx_clear();
    if (esp8266_port_send_str(esp8266_cmd) < 0) {
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
    return esp8266_mqtt_publish_raw(topic, (const unsigned char *)json_text, mqtt_strlen(json_text));
}

#if ESP8266_MQTT_ENABLE_HUAWEI
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

static const char *esp8266_huawei_report_topic(void)
{
    char *p;

    if ((esp8266_mqtt_cfg.huawei_property_report_topic != 0) &&
        (esp8266_mqtt_cfg.huawei_property_report_topic[0] != '\0')) {
        return esp8266_mqtt_cfg.huawei_property_report_topic;
    }

    p = esp8266_topic;
    p = mqtt_puts(p, "$oc/devices/");
    p = mqtt_puts(p, esp8266_huawei_device_id());
    (void)mqtt_puts(p, "/sys/properties/report");
    return esp8266_topic;
}
#endif

int esp8266_mqtt_publish_huawei_properties(const char *properties_json)
{
#if ESP8266_MQTT_ENABLE_HUAWEI
    char *p;

    if ((properties_json == 0) || (esp8266_mqtt_ready == 0U) ||
        (esp8266_mqtt_is_huawei_backend() == 0U)) {
        return -1;
    }

    if ((properties_json[0] == '{') &&
        (properties_json[1] == '"') &&
        (properties_json[2] == 's')) {
        return esp8266_mqtt_publish_json(esp8266_huawei_report_topic(), properties_json);
    }

    p = esp8266_huawei_payload;
    p = mqtt_puts(p, "{\"services\":[{\"service_id\":\"");
    p = mqtt_puts(p, esp8266_huawei_service_id());
    p = mqtt_puts(p, "\",\"properties\":");
    p = mqtt_puts(p, properties_json);
    (void)mqtt_puts(p, "}]}");

    return esp8266_mqtt_publish_json(esp8266_huawei_report_topic(), esp8266_huawei_payload);
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
    char *p;

    if ((request_id == 0) || (request_id[0] == '\0') ||
        (esp8266_mqtt_ready == 0U) || (esp8266_mqtt_is_huawei_backend() == 0U)) {
        return -1;
    }
    if ((result_desc == 0) || (result_desc[0] == '\0')) {
        result_desc = "success";
    }

    p = esp8266_topic;
    p = mqtt_puts(p, "$oc/devices/");
    p = mqtt_puts(p, esp8266_huawei_device_id());
    p = mqtt_puts(p, "/sys/properties/set/response/request_id=");
    (void)mqtt_puts(p, request_id);

    p = esp8266_huawei_payload;
    p = mqtt_puts(p, "{\"result_code\":");
    p = mqtt_put_u16(p, (unsigned short)result_code);
    p = mqtt_puts(p, ",\"result_desc\":\"");
    p = mqtt_puts(p, result_desc);
    (void)mqtt_puts(p, "\"}");

    return esp8266_mqtt_publish_json(esp8266_topic, esp8266_huawei_payload);
#else
    (void)request_id;
    (void)result_code;
    (void)result_desc;
    return -1;
#endif
}

int esp8266_mqtt_publish_telemetry(const char *topic,
                                   const char *device,
                                   float temperature,
                                   float humidity)
{
    (void)topic;
    (void)device;
    (void)temperature;
    (void)humidity;
    return -1;
}

void esp8266_mqtt_register_rx_callback(esp8266_mqtt_rx_callback_t callback)
{
    esp8266_mqtt_rx_callback = callback;
}

static unsigned char mqtt_payload_contains(const unsigned char *payload, unsigned short len, const char *token)
{
    unsigned short i;
    unsigned short j;
    unsigned short token_len = 0U;

    if ((payload == 0) || (token == 0)) {
        return 0U;
    }
    while (token[token_len] != '\0') {
        ++token_len;
    }
    if ((token_len == 0U) || (len < token_len)) {
        return 0U;
    }

    for (i = 0U; i <= (unsigned short)(len - token_len); ++i) {
        for (j = 0U; j < token_len; ++j) {
            if (payload[i + j] != (unsigned char)token[j]) {
                break;
            }
        }
        if (j == token_len) {
            return 1U;
        }
    }
    return 0U;
}

#if ESP8266_MQTT_ENABLE_HUAWEI
static unsigned char huawei_extract_request_id(const unsigned char *payload,
                                               unsigned short len,
                                               char *request_id,
                                               unsigned short request_id_size)
{
    unsigned short i;
    unsigned short j;
    unsigned short token_len = 0U;
    const char *token = "request_id=";

    if ((payload == 0) || (request_id == 0) || (request_id_size == 0U)) {
        return 0U;
    }
    while (token[token_len] != '\0') {
        ++token_len;
    }
    if (len < token_len) {
        return 0U;
    }

    for (i = 0U; i <= (unsigned short)(len - token_len); ++i) {
        for (j = 0U; j < token_len; ++j) {
            if (payload[i + j] != (unsigned char)token[j]) {
                break;
            }
        }
        if (j == token_len) {
            unsigned short out_len = 0U;
            i = (unsigned short)(i + token_len);
            while ((i < len) &&
                   (payload[i] != '"') &&
                   (payload[i] != ',') &&
                   (payload[i] != '\r') &&
                   (payload[i] != '\n') &&
                   (out_len < (unsigned short)(request_id_size - 1U))) {
                request_id[out_len] = (char)payload[i];
                ++out_len;
                ++i;
            }
            request_id[out_len] = '\0';
            return (out_len != 0U) ? 1U : 0U;
        }
    }
    return 0U;
}
#endif

void esp8266_mqtt_poll(void)
{
    const uint8_t *data;
    uint16_t len;
#if ESP8266_MQTT_ENABLE_HUAWEI
    char request_id[72];
#endif

    esp8266_port_drain_rx();
    if ((esp8266_mqtt_rx_callback == 0) || (esp8266_mqtt_ready == 0U)) {
        return;
    }

    data = esp8266_port_rx_data();
    len = esp8266_port_rx_length();
    if ((len != 0U) && (mqtt_payload_contains(data, len, "+MQTTSUBRECV:") != 0U)) {
        esp8266_mqtt_rx_callback((esp8266_mqtt_cfg.sub_topic != 0) ? esp8266_mqtt_cfg.sub_topic : "",
                                 data,
                                 len);
#if ESP8266_MQTT_ENABLE_HUAWEI
        if ((esp8266_mqtt_is_huawei_backend() != 0U) &&
            (huawei_extract_request_id(data, len, request_id, sizeof(request_id)) != 0U)) {
            (void)esp8266_mqtt_send_huawei_property_set_response(request_id, 0, "success");
        }
#endif
        esp8266_port_rx_clear();
    }
}
