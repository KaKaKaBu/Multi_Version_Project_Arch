/* ZHJG_001 智能井盖业务逻辑：甲烷、水位、倾斜、定位、报警和远程协议处理。 */
#include "app_logic.h"
#include "devmgr.h"
#include "gas_if.h"
#include "analog_probe_if.h"
#include "imu_if.h"
#include "display_if.h"
#include "misc_if.h"
#include "board_config.h"
#include "tiny_printf.h"

#include <math.h>

#if VERSION_FEATURE_REMOTE
#include "cJSON.h"
#include "cjson_port.h"
#include "esp8266_mqtt.h"
#include <string.h>
#endif

#if VERSION_FEATURE_SMS
#include "a7670c_sms.h"
#endif

#define ZHJG_DEFAULT_METHANE_THRESHOLD_PPM 1000U
#define ZHJG_MIN_METHANE_THRESHOLD_PPM 100U
#define ZHJG_MAX_METHANE_THRESHOLD_PPM 5000U
#define ZHJG_METHANE_STEP_PPM 100U
#define ZHJG_DEFAULT_WATER_THRESHOLD_PERCENT 70.0f
#define ZHJG_MIN_WATER_THRESHOLD_PERCENT 5.0f
#define ZHJG_MAX_WATER_THRESHOLD_PERCENT 100.0f
#define ZHJG_WATER_STEP_PERCENT 5.0f
#define ZHJG_DEFAULT_TILT_THRESHOLD_DEGREE 15.0f
#define ZHJG_MIN_TILT_THRESHOLD_DEGREE 1.0f
#define ZHJG_MAX_TILT_THRESHOLD_DEGREE 60.0f
#define ZHJG_TILT_STEP_DEGREE 1.0f
#define ZHJG_ALARM_BLINK_MS 300U
#define ZHJG_TELEMETRY_INTERVAL_MS 3000U
#define ZHJG_SMS_COOLDOWN_MS 300000U
#define ZHJG_SMS_BUFFER_SIZE 160U

static const gas_sensor_t *methane_sensor;
static const analog_probe_t *water_sensor;
static const imu_sensor_t *imu_sensor;
static const display_driver_t *display;
static const misc_driver_t *led_drv;
static const misc_driver_t *buzzer_drv;
#if VERSION_FEATURE_GPS
static const gnss_driver_t *gnss_drv;
#endif

zhjg_context_t g_zhjg;

static uint16_t zhjg_clamp_u16(uint16_t value, uint16_t min_value, uint16_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float zhjg_clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float zhjg_abs_float(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float zhjg_tilt_from_sample(const imu_sample_t *sample)
{
    float ax;
    float ay;
    float az;
    float horizontal;

    if (sample == 0) {
        return 0.0f;
    }

    ax = (float)sample->ax;
    ay = (float)sample->ay;
    az = (float)sample->az;
    horizontal = sqrtf((ax * ax) + (ay * ay));

    if ((horizontal == 0.0f) && (az == 0.0f)) {
        return 0.0f;
    }

    return atan2f(horizontal, zhjg_abs_float(az)) * 57.2958f;
}

static void zhjg_set_alarm_output(uint8_t on)
{
    g_zhjg.alarm_output_on = (on != 0U) ? 1U : 0U;

    if (led_drv != 0) {
        led_drv->set_state(g_zhjg.alarm_output_on);
    }
    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_zhjg.alarm_output_on);
    }
}

static const char *zhjg_selected_threshold_name(void)
{
    switch (g_zhjg.selected_threshold) {
    case ZHJG_THRESHOLD_METHANE:
        return "CH4";
    case ZHJG_THRESHOLD_WATER:
        return "Water";
    case ZHJG_THRESHOLD_TILT:
        return "Tilt";
    default:
        return "CH4";
    }
}

static void zhjg_adjust_selected_threshold(int direction)
{
    if (g_zhjg.selected_threshold == ZHJG_THRESHOLD_METHANE) {
        int value = (int)g_zhjg.methane_threshold_ppm + (direction * (int)ZHJG_METHANE_STEP_PPM);
        if (value < 0) {
            value = 0;
        }
        g_zhjg.methane_threshold_ppm = zhjg_clamp_u16((uint16_t)value,
                                                       ZHJG_MIN_METHANE_THRESHOLD_PPM,
                                                       ZHJG_MAX_METHANE_THRESHOLD_PPM);
    } else if (g_zhjg.selected_threshold == ZHJG_THRESHOLD_WATER) {
        g_zhjg.water_threshold_percent = zhjg_clamp_float(
            g_zhjg.water_threshold_percent + ((float)direction * ZHJG_WATER_STEP_PERCENT),
            ZHJG_MIN_WATER_THRESHOLD_PERCENT,
            ZHJG_MAX_WATER_THRESHOLD_PERCENT);
    } else {
        g_zhjg.tilt_threshold_degree = zhjg_clamp_float(
            g_zhjg.tilt_threshold_degree + ((float)direction * ZHJG_TILT_STEP_DEGREE),
            ZHJG_MIN_TILT_THRESHOLD_DEGREE,
            ZHJG_MAX_TILT_THRESHOLD_DEGREE);
    }

    g_zhjg.display_dirty = 1U;
    app_logic_request_telemetry();
}

static void zhjg_refresh_display(void)
{
    char line[16];

    if ((display == 0) || (g_zhjg.display_dirty == 0U)) {
        return;
    }

    display->clear();
    if (g_zhjg.mode == ZHJG_MODE_THRESHOLD) {
        DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "Set Threshold");
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "%s", zhjg_selected_threshold_name());
        if (g_zhjg.selected_threshold == ZHJG_THRESHOLD_METHANE) {
            (void)tiny_snprintf(line, sizeof(line), "%uppm", (unsigned int)g_zhjg.methane_threshold_ppm);
        } else if (g_zhjg.selected_threshold == ZHJG_THRESHOLD_WATER) {
            (void)tiny_snprintf(line, sizeof(line), "%d%%", (int)g_zhjg.water_threshold_percent);
        } else {
            (void)tiny_snprintf(line, sizeof(line), "%ddeg", (int)g_zhjg.tilt_threshold_degree);
        }
        display->print(0U, 4U, DISPLAY_FONT_LARGE, "%s", line);
        DISPLAY_PRINT(display, 0U, 7U, DISPLAY_FONT_SMALL, "K2 Sel K3+ K4-");
    } else {
        DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "Smart Manhole");
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "CH4:%uppm", (unsigned int)g_zhjg.methane_ppm);
        display->print(0U, 3U, DISPLAY_FONT_SMALL, "Water:%d%%", (int)g_zhjg.water_level_percent);
        display->print(0U, 4U, DISPLAY_FONT_SMALL, "Tilt:%ddeg", (int)g_zhjg.tilt_degree);
#if VERSION_FEATURE_GPS
        display->print(0U, 5U, DISPLAY_FONT_SMALL, g_zhjg.gps_fix.valid ? "GPS:OK" : "GPS:--");
#else
        DISPLAY_PRINT(display, 0U, 5U, DISPLAY_FONT_SMALL, "GPS:N/A");
#endif
        display->print(0U, 7U, DISPLAY_FONT_SMALL, g_zhjg.alarm_active ? "ALARM" : "SAFE");
    }
    display->update();
    g_zhjg.display_dirty = 0U;
}

#if VERSION_FEATURE_SMS
static void zhjg_maybe_send_sms(uint32_t now)
{
    char text[ZHJG_SMS_BUFFER_SIZE];

    if (g_zhjg.alarm_active == 0U) {
        g_zhjg.sms_sent_for_alarm = 0U;
        return;
    }

    if (g_zhjg.sms_sent_for_alarm != 0U) {
        if ((now - g_zhjg.last_sms_tick) < ZHJG_SMS_COOLDOWN_MS) {
            return;
        }
        g_zhjg.sms_sent_for_alarm = 0U;
    }

    if ((g_zhjg.last_sms_tick != 0U) &&
        ((now - g_zhjg.last_sms_tick) < ZHJG_SMS_COOLDOWN_MS)) {
        return;
    }

    (void)tiny_snprintf(text,
                        sizeof(text),
                        "ZHJG_001 ALARM CH4:%uppm Water:%d%% Tilt:%d GPS:%d %.6f %.6f",
                        (unsigned int)g_zhjg.methane_ppm,
                        (int)g_zhjg.water_level_percent,
                        (int)g_zhjg.tilt_degree,
                        (int)g_zhjg.gps_fix.valid,
                        g_zhjg.gps_fix.latitude,
                        g_zhjg.gps_fix.longitude);

    g_zhjg.last_sms_tick = now;
    if (a7670c_sms_send_text(BOARD_A7670C_ALERT_PHONE, text) == 0) {
        g_zhjg.sms_sent_for_alarm = 1U;
    }
}
#endif

#if VERSION_FEATURE_REMOTE
static void zhjg_publish_telemetry(void)
{
    cJSON *root;
    cJSON *data;
    cJSON *alarms;
    cJSON *thresholds;
    cJSON *gps;
    char *json_text;

    root = cJSON_CreateObject();
    if (root == 0) {
        return;
    }

    cJSON_AddStringToObject(root, "type", "telemetry");
    cJSON_AddStringToObject(root, "version", VERSION_NAME);
    cJSON_AddNumberToObject(root, "version_no", APP_VERSION);

    data = cJSON_CreateObject();
    if (data != 0) {
        cJSON_AddNumberToObject(data, "methane_ppm", g_zhjg.methane_ppm);
        cJSON_AddNumberToObject(data, "water_level_percent", g_zhjg.water_level_percent);
        cJSON_AddNumberToObject(data, "tilt_degree", g_zhjg.tilt_degree);
        cJSON_AddNumberToObject(data, "alarm", g_zhjg.alarm_active);
        cJSON_AddStringToObject(data, "mode", (g_zhjg.mode == ZHJG_MODE_THRESHOLD) ? "threshold" : "auto");

        alarms = cJSON_CreateObject();
        if (alarms != 0) {
            cJSON_AddNumberToObject(alarms, "methane", g_zhjg.methane_alarm);
            cJSON_AddNumberToObject(alarms, "water", g_zhjg.water_alarm);
            cJSON_AddNumberToObject(alarms, "tilt", g_zhjg.tilt_alarm);
            cJSON_AddItemToObject(data, "alarms", alarms);
        }

        thresholds = cJSON_CreateObject();
        if (thresholds != 0) {
            cJSON_AddNumberToObject(thresholds, "methane_ppm", g_zhjg.methane_threshold_ppm);
            cJSON_AddNumberToObject(thresholds, "water_level_percent", g_zhjg.water_threshold_percent);
            cJSON_AddNumberToObject(thresholds, "tilt_degree", g_zhjg.tilt_threshold_degree);
            cJSON_AddItemToObject(data, "thresholds", thresholds);
        }

        gps = cJSON_CreateObject();
        if (gps != 0) {
#if VERSION_FEATURE_GPS
            cJSON_AddNumberToObject(gps, "valid", g_zhjg.gps_fix.valid);
            cJSON_AddNumberToObject(gps, "latitude", g_zhjg.gps_fix.latitude);
            cJSON_AddNumberToObject(gps, "longitude", g_zhjg.gps_fix.longitude);
#else
            cJSON_AddNumberToObject(gps, "valid", 0);
            cJSON_AddNumberToObject(gps, "latitude", 0);
            cJSON_AddNumberToObject(gps, "longitude", 0);
#endif
            cJSON_AddItemToObject(data, "gps", gps);
        }

        cJSON_AddItemToObject(root, "data", data);
    }

    json_text = cJSON_PrintUnformatted(root);
    if (json_text != 0) {
        if (esp8266_mqtt_is_ready() != 0) {
            (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, json_text);
        }
        cjson_release_string(json_text);
    }

    cjson_release(root);
}

static const cJSON *zhjg_json_params(const cJSON *root)
{
    const cJSON *params;

    params = cJSON_GetObjectItem(root, "params");
    if ((params != 0) && ((params->type & cJSON_Object) != 0)) {
        return params;
    }

    return root;
}

static void zhjg_apply_threshold_json(const cJSON *params)
{
    const cJSON *item;

    item = cJSON_GetObjectItem(params, "methane_ppm");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_zhjg.methane_threshold_ppm = zhjg_clamp_u16((uint16_t)item->valueint,
                                                       ZHJG_MIN_METHANE_THRESHOLD_PPM,
                                                       ZHJG_MAX_METHANE_THRESHOLD_PPM);
    }

    item = cJSON_GetObjectItem(params, "water_level_percent");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_zhjg.water_threshold_percent = zhjg_clamp_float((float)item->valuedouble,
                                                          ZHJG_MIN_WATER_THRESHOLD_PERCENT,
                                                          ZHJG_MAX_WATER_THRESHOLD_PERCENT);
    }

    item = cJSON_GetObjectItem(params, "tilt_degree");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_zhjg.tilt_threshold_degree = zhjg_clamp_float((float)item->valuedouble,
                                                        ZHJG_MIN_TILT_THRESHOLD_DEGREE,
                                                        ZHJG_MAX_TILT_THRESHOLD_DEGREE);
    }

    g_zhjg.display_dirty = 1U;
}
#endif

void app_logic_init(void)
{
    g_zhjg.mode = ZHJG_MODE_AUTO;
    g_zhjg.selected_threshold = ZHJG_THRESHOLD_METHANE;
    g_zhjg.methane_ppm = 0U;
    g_zhjg.water_level_percent = 0.0f;
    g_zhjg.tilt_degree = 0.0f;
    g_zhjg.methane_threshold_ppm = ZHJG_DEFAULT_METHANE_THRESHOLD_PPM;
    g_zhjg.water_threshold_percent = ZHJG_DEFAULT_WATER_THRESHOLD_PERCENT;
    g_zhjg.tilt_threshold_degree = ZHJG_DEFAULT_TILT_THRESHOLD_DEGREE;
    g_zhjg.methane_alarm = 0U;
    g_zhjg.water_alarm = 0U;
    g_zhjg.tilt_alarm = 0U;
    g_zhjg.alarm_active = 0U;
    g_zhjg.alarm_output_on = 0U;
    g_zhjg.display_dirty = 1U;
    g_zhjg.telemetry_pending = 1U;
    g_zhjg.last_alarm_toggle_tick = sched_tick_get();
    g_zhjg.last_telemetry_tick = 0U;
#if VERSION_FEATURE_GPS
    g_zhjg.gps_fix.valid = 0U;
    g_zhjg.gps_fix.latitude = 0.0f;
    g_zhjg.gps_fix.longitude = 0.0f;
    g_zhjg.gps_fix.speed_kmh = 0.0f;
    g_zhjg.gps_fix.course = 0.0f;
    g_zhjg.gps_fix.altitude = 0.0f;
#endif
#if VERSION_FEATURE_SMS
    g_zhjg.last_sms_tick = 0U;
    g_zhjg.sms_sent_for_alarm = 0U;
#endif

    methane_sensor = devmgr_get_gas_sensor("mq4_methane");
    water_sensor = devmgr_get_analog_probe("water_level");
    imu_sensor = devmgr_get_imu_sensor("mpu6050");
    display = devmgr_get_display("oled");
    led_drv = devmgr_get_misc("led");
    buzzer_drv = devmgr_get_misc("buzzer");
#if VERSION_FEATURE_GPS
    gnss_drv = devmgr_get_gnss("l76k");
#endif
#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif
    zhjg_set_alarm_output(0U);
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
    imu_sample_t sample;
    uint8_t old_alarm;

    (void)events;
    (void)ctx;

    old_alarm = g_zhjg.alarm_active;

    if (methane_sensor != 0) {
        g_zhjg.methane_ppm = methane_sensor->read_ppm();
    }
    if (water_sensor != 0) {
        g_zhjg.water_level_percent = water_sensor->read_value();
    }
    if (imu_sensor != 0) {
        imu_sensor->read_sample(&sample);
        g_zhjg.tilt_degree = zhjg_tilt_from_sample(&sample);
    }
#if VERSION_FEATURE_GPS
    if (gnss_drv != 0) {
        gnss_drv->poll();
        gnss_drv->get_fix(&g_zhjg.gps_fix);
    }
#endif

    g_zhjg.methane_alarm = (g_zhjg.methane_ppm > g_zhjg.methane_threshold_ppm) ? 1U : 0U;
    g_zhjg.water_alarm = (g_zhjg.water_level_percent > g_zhjg.water_threshold_percent) ? 1U : 0U;
    g_zhjg.tilt_alarm = (g_zhjg.tilt_degree > g_zhjg.tilt_threshold_degree) ? 1U : 0U;
    g_zhjg.alarm_active = (g_zhjg.methane_alarm || g_zhjg.water_alarm || g_zhjg.tilt_alarm) ? 1U : 0U;

    g_zhjg.display_dirty = 1U;
    if (old_alarm != g_zhjg.alarm_active) {
        app_logic_request_telemetry();
    }
}

void alarm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now;

    (void)events;
    (void)ctx;

    now = sched_tick_get();

#if VERSION_FEATURE_SMS
    a7670c_sms_poll();
#endif

    if ((g_zhjg.mode != ZHJG_MODE_AUTO) || (g_zhjg.alarm_active == 0U)) {
        if (g_zhjg.alarm_output_on != 0U) {
            zhjg_set_alarm_output(0U);
            g_zhjg.display_dirty = 1U;
        }
        return;
    }

    if ((now - g_zhjg.last_alarm_toggle_tick) >= ZHJG_ALARM_BLINK_MS) {
        zhjg_set_alarm_output((g_zhjg.alarm_output_on == 0U) ? 1U : 0U);
        g_zhjg.last_alarm_toggle_tick = now;
        g_zhjg.display_dirty = 1U;
        event_set(APP_EVENT_ALARM);
    }

#if VERSION_FEATURE_SMS
    zhjg_maybe_send_sms(now);
#endif
}

void display_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    zhjg_refresh_display();
}

void app_logic_on_key1_press(void)
{
    if (g_zhjg.mode == ZHJG_MODE_AUTO) {
        g_zhjg.mode = ZHJG_MODE_THRESHOLD;
        zhjg_set_alarm_output(0U);
    } else {
        g_zhjg.mode = ZHJG_MODE_AUTO;
    }
    g_zhjg.display_dirty = 1U;
    app_logic_request_telemetry();
}

void app_logic_on_key2_press(void)
{
    if (g_zhjg.mode == ZHJG_MODE_THRESHOLD) {
        if (g_zhjg.selected_threshold == ZHJG_THRESHOLD_TILT) {
            g_zhjg.selected_threshold = ZHJG_THRESHOLD_METHANE;
        } else {
            g_zhjg.selected_threshold = (zhjg_threshold_item_t)((uint8_t)g_zhjg.selected_threshold + 1U);
        }
        g_zhjg.display_dirty = 1U;
    }
}

void app_logic_on_key3_press(void)
{
    if (g_zhjg.mode == ZHJG_MODE_THRESHOLD) {
        zhjg_adjust_selected_threshold(1);
    }
}

void app_logic_on_key4_press(void)
{
    if (g_zhjg.mode == ZHJG_MODE_THRESHOLD) {
        zhjg_adjust_selected_threshold(-1);
    }
}

void app_logic_request_telemetry(void)
{
#if VERSION_FEATURE_REMOTE
    g_zhjg.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
#endif
}

void comm_loop_run(sched_event_t events, void *ctx)
{
#if VERSION_FEATURE_REMOTE
    uint32_t now;
#endif

    (void)ctx;

#if VERSION_FEATURE_WIFI
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_TICK)) != 0U) {
        esp8266_mqtt_poll();
    }
#endif

#if VERSION_FEATURE_REMOTE
    now = sched_tick_get();
    if ((now - g_zhjg.last_telemetry_tick) >= ZHJG_TELEMETRY_INTERVAL_MS) {
        g_zhjg.telemetry_pending = 1U;
    }

    if (((events & (APP_EVENT_COMM_TX | APP_EVENT_TICK | APP_EVENT_SENSOR)) != 0U) &&
        (g_zhjg.telemetry_pending != 0U)) {
        g_zhjg.telemetry_pending = 0U;
        g_zhjg.last_telemetry_tick = now;
        zhjg_publish_telemetry();
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
    const cJSON *params;
    const cJSON *mode_item;
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
    params = zhjg_json_params(root);

    if (strcmp(cmd, "set_threshold") == 0) {
        zhjg_apply_threshold_json(params);
    } else if (strcmp(cmd, "set_mode") == 0) {
        mode_item = cJSON_GetObjectItem(params, "mode");
        if ((mode_item != 0) && cJSON_IsString(mode_item)) {
            if (strcmp(mode_item->valuestring, "threshold") == 0) {
                g_zhjg.mode = ZHJG_MODE_THRESHOLD;
            } else if (strcmp(mode_item->valuestring, "auto") == 0) {
                g_zhjg.mode = ZHJG_MODE_AUTO;
            }
            g_zhjg.display_dirty = 1U;
        }
    } else if (strcmp(cmd, "get_status") == 0) {
        g_zhjg.telemetry_pending = 1U;
    }

    app_logic_request_telemetry();
    cjson_release(root);
#else
    (void)json_data;
#endif
}
