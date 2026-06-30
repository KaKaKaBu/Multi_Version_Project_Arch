/* DCLD_001 倒车雷达业务逻辑：测距、温度补偿、报警、显示和远程协议处理。 */
#include "app_logic.h"
#include "devmgr.h"
#include "distance_if.h"
#include "sensor_if.h"
#include "display_if.h"
#include "misc_if.h"
#include "comm_if.h"
#include "board_config.h"
#include "tiny_printf.h"

#if VERSION_FEATURE_REMOTE
#include "comm_port.h"
#include "cJSON.h"
#include "cjson_port.h"
#include <string.h>
#endif

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif

#define DCLD_DEFAULT_THRESHOLD_CM 80U
#define DCLD_MIN_THRESHOLD_CM 20U
#define DCLD_MAX_THRESHOLD_CM 400U
#define DCLD_THRESHOLD_STEP_CM 5U
#define DCLD_TEMP_SAMPLE_MS 5000U
#define DCLD_VOICE_INTERVAL_MS 5000U
#define DCLD_TELEMETRY_BUFFER_SIZE 384U

static const distance_sensor_t *distance_sensor;
#if VERSION_FEATURE_TEMP_COMP
static const temp_hum_sensor_t *temp_sensor;
#endif
static const display_driver_t *display;
static const misc_driver_t *led_drv;
static const misc_driver_t *buzzer_drv;
#if VERSION_FEATURE_VOICE
static const comm_driver_t *voice_drv;
#endif

dcld_context_t g_dcld;

static uint16_t dcld_clamp_threshold(uint16_t value)
{
    if (value < DCLD_MIN_THRESHOLD_CM) {
        return DCLD_MIN_THRESHOLD_CM;
    }
    if (value > DCLD_MAX_THRESHOLD_CM) {
        return DCLD_MAX_THRESHOLD_CM;
    }
    return value;
}

static uint16_t dcld_apply_temperature_comp(uint16_t raw_distance_cm)
{
#if VERSION_FEATURE_TEMP_COMP
    float speed_ratio = (331.3f + (0.606f * g_dcld.temperature_c)) / 343.42f;
    float compensated = (float)raw_distance_cm * speed_ratio;

    if (compensated < 0.0f) {
        return 0U;
    }
    if (compensated > 65535.0f) {
        return 65535U;
    }
    return (uint16_t)(compensated + 0.5f);
#else
    return raw_distance_cm;
#endif
}

static uint32_t dcld_alarm_period_for_distance(uint16_t distance_cm)
{
    if (distance_cm <= 20U) {
        return 120U;
    }
    if (distance_cm <= 40U) {
        return 220U;
    }
    if (distance_cm <= 60U) {
        return 380U;
    }
    return 650U;
}

static void dcld_set_alarm_output(uint8_t on)
{
    g_dcld.alarm_output_on = (on != 0U) ? 1U : 0U;

    if (led_drv != 0) {
        led_drv->set_state(g_dcld.alarm_output_on);
    }
    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_dcld.alarm_output_on);
    }
}

#if VERSION_FEATURE_VOICE
static uint8_t dcld_voice_level_for_distance(uint16_t distance_cm)
{
    if (distance_cm <= 20U) {
        return 4U;
    }
    if (distance_cm <= 40U) {
        return 3U;
    }
    if (distance_cm <= 60U) {
        return 2U;
    }
    return 1U;
}

static void dcld_voice_maybe_announce(uint32_t now)
{
    unsigned char cmd[2];
    uint8_t level;

    if ((voice_drv == 0) || (g_dcld.alarm_active == 0U)) {
        return;
    }

    level = dcld_voice_level_for_distance(g_dcld.distance_cm);
    if ((level == g_dcld.voice_level) && ((now - g_dcld.last_voice_tick) < DCLD_VOICE_INTERVAL_MS)) {
        return;
    }

    cmd[0] = 0x10U;
    cmd[1] = level;
    (void)voice_drv->send(cmd, (unsigned short)sizeof(cmd));
    g_dcld.voice_level = level;
    g_dcld.last_voice_tick = now;
}
#endif

static void dcld_refresh_display(void)
{
    char line[16];

    if ((display == 0) || (g_dcld.display_dirty == 0U)) {
        return;
    }

    display->clear();
    if (g_dcld.mode == DCLD_MODE_THRESHOLD) {
        DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "Set Threshold");
        DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "K2:+ K3:-");
        (void)tiny_snprintf(line, sizeof(line), "%ucm", (unsigned int)g_dcld.threshold_cm);
        display->print(0U, 4U, DISPLAY_FONT_LARGE, "%s", line);
    } else {
        DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "Reverse Radar");
        (void)tiny_snprintf(line, sizeof(line), "%ucm", (unsigned int)g_dcld.distance_cm);
        display->print(0U, 2U, DISPLAY_FONT_LARGE, "%s", line);
        display->print(0U, 5U, DISPLAY_FONT_SMALL, "Th:%ucm", (unsigned int)g_dcld.threshold_cm);
#if VERSION_FEATURE_TEMP_COMP
        display->print(10U, 5U, DISPLAY_FONT_SMALL, "T:%dC", (int)g_dcld.temperature_c);
#endif
        display->print(0U, 7U, DISPLAY_FONT_SMALL, g_dcld.alarm_active ? "ALARM" : "SAFE");
    }
    display->update();
    g_dcld.display_dirty = 0U;
}

#if VERSION_FEATURE_REMOTE
static void dcld_publish_telemetry(void)
{
    cJSON *root;
    cJSON *thresholds;
    char *json_text;

    root = cJSON_CreateObject();
    if (root == 0) {
        return;
    }

    cJSON_AddStringToObject(root, "version", VERSION_NAME);
    cJSON_AddNumberToObject(root, "version_no", APP_VERSION);
    cJSON_AddStringToObject(root, "mode", (g_dcld.mode == DCLD_MODE_THRESHOLD) ? "threshold" : "auto");
    cJSON_AddNumberToObject(root, "distance_cm", g_dcld.distance_cm);
    cJSON_AddNumberToObject(root, "raw_distance_cm", g_dcld.raw_distance_cm);
    cJSON_AddNumberToObject(root, "threshold_cm", g_dcld.threshold_cm);
    cJSON_AddNumberToObject(root, "alarm", g_dcld.alarm_active);
#if VERSION_FEATURE_TEMP_COMP
    cJSON_AddNumberToObject(root, "temperature_c", g_dcld.temperature_c);
#endif
#if VERSION_FEATURE_CAMERA
    cJSON_AddStringToObject(root, "camera_url", BOARD_ESP32_CAM_STREAM_URL);
#endif

    thresholds = cJSON_CreateObject();
    if (thresholds != 0) {
        cJSON_AddNumberToObject(thresholds, "distance", g_dcld.threshold_cm);
        cJSON_AddItemToObject(root, "thresholds", thresholds);
    }

    json_text = cJSON_PrintUnformatted(root);
    if (json_text != 0) {
#if VERSION_FEATURE_WIFI
        if (esp8266_mqtt_is_ready() != 0) {
            (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, json_text);
        }
#endif
#if VERSION_FEATURE_BLE
        (void)comm_port_send((const unsigned char *)json_text, (unsigned short)strlen(json_text));
        (void)comm_port_send((const unsigned char *)"\n", 1U);
#endif
        cjson_release_string(json_text);
    }

    cjson_release(root);
}
#endif

void app_logic_init(void)
{
    g_dcld.mode = DCLD_MODE_AUTO;
    g_dcld.raw_distance_cm = 0U;
    g_dcld.distance_cm = 0U;
    g_dcld.threshold_cm = DCLD_DEFAULT_THRESHOLD_CM;
    g_dcld.temperature_c = 20.0f;
    g_dcld.alarm_active = 0U;
    g_dcld.alarm_output_on = 0U;
    g_dcld.display_dirty = 1U;
    g_dcld.telemetry_pending = 1U;
    g_dcld.voice_level = 0U;
    g_dcld.last_alarm_toggle_tick = sched_tick_get();
    g_dcld.alarm_period_ms = dcld_alarm_period_for_distance(DCLD_DEFAULT_THRESHOLD_CM);
    g_dcld.last_temp_sample_tick = 0U;
    g_dcld.last_voice_tick = 0U;

    distance_sensor = devmgr_get_distance_sensor("hcsr04");
#if VERSION_FEATURE_TEMP_COMP
    temp_sensor = devmgr_get_temp_hum_sensor("ds18b20");
#endif
    display = devmgr_get_display("oled");
    led_drv = devmgr_get_misc("led");
    buzzer_drv = devmgr_get_misc("buzzer");
#if VERSION_FEATURE_VOICE
    voice_drv = devmgr_get_comm("su03t");
#endif
#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif
    dcld_set_alarm_output(0U);
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
    uint16_t raw;

    (void)events;
    (void)ctx;

#if VERSION_FEATURE_TEMP_COMP
    {
        uint32_t now = sched_tick_get();
        if ((temp_sensor != 0) && ((now - g_dcld.last_temp_sample_tick) >= DCLD_TEMP_SAMPLE_MS)) {
            g_dcld.temperature_c = temp_sensor->read_temperature();
            g_dcld.last_temp_sample_tick = sched_tick_get();
        }
    }
#endif

    if (distance_sensor != 0) {
        raw = distance_sensor->read_distance_cm();
        if (raw != 0U) {
            g_dcld.raw_distance_cm = raw;
            g_dcld.distance_cm = dcld_apply_temperature_comp(raw);
        }
    }

    g_dcld.alarm_active = ((g_dcld.distance_cm > 0U) && (g_dcld.distance_cm < g_dcld.threshold_cm)) ? 1U : 0U;
    g_dcld.alarm_period_ms = dcld_alarm_period_for_distance(g_dcld.distance_cm);
    g_dcld.display_dirty = 1U;
    app_logic_request_telemetry();
}

void alarm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now = sched_tick_get();

    (void)events;
    (void)ctx;

    if ((g_dcld.mode != DCLD_MODE_AUTO) || (g_dcld.alarm_active == 0U)) {
        if (g_dcld.alarm_output_on != 0U) {
            dcld_set_alarm_output(0U);
            g_dcld.display_dirty = 1U;
        }
        g_dcld.voice_level = 0U;
        return;
    }

    if ((now - g_dcld.last_alarm_toggle_tick) >= g_dcld.alarm_period_ms) {
        dcld_set_alarm_output((g_dcld.alarm_output_on == 0U) ? 1U : 0U);
        g_dcld.last_alarm_toggle_tick = now;
        g_dcld.display_dirty = 1U;
        event_set(APP_EVENT_ALARM);
    }

#if VERSION_FEATURE_VOICE
    dcld_voice_maybe_announce(now);
#endif
}

void display_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    dcld_refresh_display();
}

void app_logic_on_key1_press(void)
{
    if (g_dcld.mode == DCLD_MODE_AUTO) {
        g_dcld.mode = DCLD_MODE_THRESHOLD;
        dcld_set_alarm_output(0U);
    } else {
        g_dcld.mode = DCLD_MODE_AUTO;
    }
    g_dcld.display_dirty = 1U;
    app_logic_request_telemetry();
}

void app_logic_on_key2_press(void)
{
    if (g_dcld.mode == DCLD_MODE_THRESHOLD) {
        g_dcld.threshold_cm = dcld_clamp_threshold((uint16_t)(g_dcld.threshold_cm + DCLD_THRESHOLD_STEP_CM));
        g_dcld.display_dirty = 1U;
        app_logic_request_telemetry();
    }
}

void app_logic_on_key3_press(void)
{
    if (g_dcld.mode == DCLD_MODE_THRESHOLD) {
        if (g_dcld.threshold_cm > DCLD_THRESHOLD_STEP_CM) {
            g_dcld.threshold_cm = dcld_clamp_threshold((uint16_t)(g_dcld.threshold_cm - DCLD_THRESHOLD_STEP_CM));
        }
        g_dcld.display_dirty = 1U;
        app_logic_request_telemetry();
    }
}

void app_logic_request_telemetry(void)
{
#if VERSION_FEATURE_REMOTE
    g_dcld.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
#endif
}

void comm_loop_run(sched_event_t events, void *ctx)
{
#if VERSION_FEATURE_BLE
    unsigned char rx;
#endif

    (void)ctx;

#if VERSION_FEATURE_WIFI
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_TICK)) != 0U) {
        esp8266_mqtt_poll();
    }
#endif

#if VERSION_FEATURE_BLE
    if ((events & APP_EVENT_COMM_RX) != 0U) {
        while (comm_port_recv(&rx, 1U) > 0) {
        }
    }
#endif

#if VERSION_FEATURE_REMOTE
    if (((events & (APP_EVENT_COMM_TX | APP_EVENT_TICK | APP_EVENT_SENSOR)) != 0U) &&
        (g_dcld.telemetry_pending != 0U)) {
        g_dcld.telemetry_pending = 0U;
        dcld_publish_telemetry();
    }
#else
    (void)events;
#endif
}

void app_logic_on_mqtt_rx(const char *json_data)
{
#if VERSION_FEATURE_REMOTE
    cJSON *root;
    cJSON *cmd_item;
    const cJSON *value_item;
    const char *cmd;

    if (json_data == 0) {
        return;
    }

    root = cjson_parse(json_data, strlen(json_data));
    if (root == 0) {
        return;
    }

    cmd_item = cJSON_GetObjectItem(root, "cmd");
    if ((cmd_item == 0) || !cJSON_IsString(cmd_item)) {
        cjson_release(root);
        return;
    }

    cmd = cmd_item->valuestring;
    if (strcmp(cmd, "set_threshold") == 0) {
        value_item = cJSON_GetObjectItem(root, "value");
        if ((value_item != 0) && ((value_item->type & cJSON_Number) != 0)) {
            g_dcld.threshold_cm = dcld_clamp_threshold((uint16_t)value_item->valueint);
            g_dcld.display_dirty = 1U;
        }
    } else if (strcmp(cmd, "get_status") == 0) {
        g_dcld.telemetry_pending = 1U;
    } else if (strcmp(cmd, "set_mode") == 0) {
        const cJSON *mode_item = cJSON_GetObjectItem(root, "mode");
        if ((mode_item != 0) && cJSON_IsString(mode_item)) {
            if (strcmp(mode_item->valuestring, "threshold") == 0) {
                g_dcld.mode = DCLD_MODE_THRESHOLD;
            } else if (strcmp(mode_item->valuestring, "auto") == 0) {
                g_dcld.mode = DCLD_MODE_AUTO;
            }
            g_dcld.display_dirty = 1U;
        }
    }

    app_logic_request_telemetry();
    cjson_release(root);
#else
    (void)json_data;
#endif
}
