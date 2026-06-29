/* RTJK_001 健康监测业务逻辑：心率血氧、体温、血压、报警、显示和远程协议处理。 */
#include "app_logic.h"
#include "board_config.h"
#include "cjson_port.h"
#include "devmgr.h"
#include "version_config.h"
#include "comm_if.h"
#include <string.h>

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif

#define APP_MODE_AUTO 0U
#define APP_MODE_THRESHOLD 1U

#define APP_FIELD_HR_MIN 0U
#define APP_FIELD_HR_MAX 1U
#define APP_FIELD_SPO2_MIN 2U
#define APP_FIELD_TEMP_MIN 3U
#define APP_FIELD_TEMP_MAX 4U
#define APP_FIELD_BP_MIN 5U
#define APP_FIELD_BP_MAX 6U
#define APP_FIELD_COUNT 7U

typedef struct app_thresholds {
    unsigned char hr_min;
    unsigned char hr_max;
    unsigned char spo2_min;
    float temp_min;
    float temp_max;
    unsigned char bp_min;
    unsigned char bp_max;
} app_thresholds_t;

typedef struct app_context {
    unsigned char hr;
    unsigned char spo2;
    float temp;
    unsigned char bp_sys;
    unsigned char bp_dia;
    unsigned char mode;
    unsigned char threshold_field;
    unsigned char alarm_active;
    unsigned char alarm_hr;
    unsigned char alarm_spo2;
    unsigned char alarm_temp;
    unsigned char alarm_bp;
    unsigned char display_dirty;
    app_thresholds_t th;
} app_context_t;

static app_context_t app_ctx;
static const pulse_oximeter_t *app_pulse;
#if VERSION_FEATURE_TEMP
static const temp_hum_sensor_t *app_temp;
#endif
#if VERSION_FEATURE_BLOOD_PRESSURE
static const blood_pressure_sensor_t *app_bp;
#endif
static const display_driver_t *app_display;
static const misc_driver_t *app_buzzer;
static const misc_driver_t *app_led;
#if VERSION_FEATURE_VOICE
static const comm_driver_t *app_voice;
#endif

extern const unsigned char *app_get_last_rx(unsigned short *len);
extern void app_clear_last_rx(void);

static unsigned char app_field_is_valid(unsigned char field)
{
    if (field == APP_FIELD_HR_MIN || field == APP_FIELD_HR_MAX) {
        return 1U;
    }
#if VERSION_FEATURE_SPO2
    if (field == APP_FIELD_SPO2_MIN) {
        return 1U;
    }
#endif
#if VERSION_FEATURE_TEMP
    if (field == APP_FIELD_TEMP_MIN || field == APP_FIELD_TEMP_MAX) {
        return 1U;
    }
#endif
#if VERSION_FEATURE_BLOOD_PRESSURE
    if (field == APP_FIELD_BP_MIN || field == APP_FIELD_BP_MAX) {
        return 1U;
    }
#endif
    return 0U;
}

static unsigned char app_next_field(unsigned char field)
{
    unsigned char next = (unsigned char)((field + 1U) % APP_FIELD_COUNT);

    while (app_field_is_valid(next) == 0U) {
        next = (unsigned char)((next + 1U) % APP_FIELD_COUNT);
        if (next == field) {
            break;
        }
    }

    return next;
}

static void app_outputs_apply(unsigned char alarm_on)
{
    if (app_buzzer != 0) {
        app_buzzer->set_state(alarm_on);
    }
    if (app_led != 0) {
        app_led->set_state(alarm_on);
    }
}

#if VERSION_FEATURE_VOICE
static void app_voice_play(unsigned char cmd_id)
{
    unsigned char payload[2];

    if (app_voice == 0) {
        return;
    }

    payload[0] = cmd_id;
    payload[1] = 1U;
    (void)app_voice->send(payload, 2U);
}
#endif

static void app_read_sensors(void)
{
    if (app_pulse != 0) {
        app_ctx.hr = app_pulse->read_heart_rate();
#if VERSION_FEATURE_SPO2
        app_ctx.spo2 = app_pulse->read_spo2();
#endif
    }

#if VERSION_FEATURE_TEMP
    if (app_temp != 0) {
        app_ctx.temp = app_temp->read_temperature();
    }
#endif

#if VERSION_FEATURE_BLOOD_PRESSURE
    if (app_bp != 0) {
        app_ctx.bp_sys = app_bp->read_systolic();
        app_ctx.bp_dia = app_bp->read_diastolic();
    }
#endif
}

static void app_check_alarm(void)
{
    unsigned char hr_alarm = 0U;
    unsigned char spo2_alarm = 0U;
    unsigned char temp_alarm = 0U;
    unsigned char bp_alarm = 0U;

    if (app_ctx.mode != APP_MODE_AUTO) {
        app_ctx.alarm_active = 0U;
        app_outputs_apply(0U);
        return;
    }

    if ((app_ctx.hr < app_ctx.th.hr_min) || (app_ctx.hr > app_ctx.th.hr_max)) {
        hr_alarm = 1U;
    }

#if VERSION_FEATURE_SPO2
    if (app_ctx.spo2 < app_ctx.th.spo2_min) {
        spo2_alarm = 1U;
    }
#endif

#if VERSION_FEATURE_TEMP
    if ((app_ctx.temp < app_ctx.th.temp_min) || (app_ctx.temp > app_ctx.th.temp_max)) {
        temp_alarm = 1U;
    }
#endif

#if VERSION_FEATURE_BLOOD_PRESSURE
    if ((app_ctx.bp_sys < app_ctx.th.bp_min) || (app_ctx.bp_sys > app_ctx.th.bp_max)) {
        bp_alarm = 1U;
    }
#endif

    if (hr_alarm != app_ctx.alarm_hr) {
#if VERSION_FEATURE_VOICE
        if (hr_alarm != 0U) {
            app_voice_play(BOARD_VOICE_CMD_HR);
        }
#endif
        app_ctx.alarm_hr = hr_alarm;
    }

#if VERSION_FEATURE_SPO2
    if (spo2_alarm != app_ctx.alarm_spo2) {
#if VERSION_FEATURE_VOICE
        if (spo2_alarm != 0U) {
            app_voice_play(BOARD_VOICE_CMD_SPO2);
        }
#endif
        app_ctx.alarm_spo2 = spo2_alarm;
    }
#endif

#if VERSION_FEATURE_TEMP
    if (temp_alarm != app_ctx.alarm_temp) {
#if VERSION_FEATURE_VOICE
        if (temp_alarm != 0U) {
            app_voice_play(BOARD_VOICE_CMD_TEMP);
        }
#endif
        app_ctx.alarm_temp = temp_alarm;
    }
#endif

    app_ctx.alarm_bp = bp_alarm;
    app_ctx.alarm_active = (unsigned char)((hr_alarm != 0U) || (spo2_alarm != 0U) ||
                                           (temp_alarm != 0U) || (bp_alarm != 0U));
    app_outputs_apply(app_ctx.alarm_active);
}

static void app_display_auto(void)
{
    if (app_display == 0) {
        return;
    }

    app_display->clear();
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, "Mode:AUTO");
    app_display->print(0U, 2U, DISPLAY_FONT_SMALL, "HR:%3u BPM", (unsigned int)app_ctx.hr);

#if VERSION_FEATURE_SPO2
    app_display->print(0U, 3U, DISPLAY_FONT_SMALL, "SpO2:%3u %%", (unsigned int)app_ctx.spo2);
#endif

#if VERSION_FEATURE_BLOOD_PRESSURE
    app_display->print(0U, 4U, DISPLAY_FONT_SMALL, "BP:%3u/%3u",
                       (unsigned int)app_ctx.bp_sys, (unsigned int)app_ctx.bp_dia);
#endif

#if VERSION_FEATURE_TEMP
    {
        int temp_x10 = (int)(app_ctx.temp * 10.0f);
        app_display->print(0U, 5U, DISPLAY_FONT_SMALL, "Temp:%2d.%1dC",
                           temp_x10 / 10, temp_x10 % 10);
    }
#endif

    app_display->print(0U, 6U, DISPLAY_FONT_SMALL, "Alarm:%s",
                       app_ctx.alarm_active != 0U ? "ON " : "OFF");
    app_display->update();
}

static void app_print_threshold_line(unsigned char row, const char *label, const char *value,
                                     unsigned char field_id)
{
    char line[22];
    unsigned char pos = 0U;

    line[pos++] = (app_ctx.threshold_field == field_id) ? '*' : ' ';
    line[pos++] = label[0];
    line[pos++] = label[1];
    line[pos++] = label[2];
    line[pos++] = ':';
    while ((*value != '\0') && (pos < (sizeof(line) - 1U))) {
        line[pos++] = *value;
        ++value;
    }
    line[pos] = '\0';
    app_display->print(0U, row, DISPLAY_FONT_SMALL, "%s", line);
}

static void app_display_threshold(void)
{
    char buf[12];

    if (app_display == 0) {
        return;
    }

    app_display->clear();
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, "Mode:THRESH");

    buf[0] = (char)('0' + (app_ctx.th.hr_min / 10U));
    buf[1] = (char)('0' + (app_ctx.th.hr_min % 10U));
    buf[2] = '\0';
    app_print_threshold_line(2U, "Hmn", buf, APP_FIELD_HR_MIN);

    buf[0] = (char)('0' + (app_ctx.th.hr_max / 10U));
    buf[1] = (char)('0' + (app_ctx.th.hr_max % 10U));
    buf[2] = '\0';
    app_print_threshold_line(3U, "Hmx", buf, APP_FIELD_HR_MAX);

#if VERSION_FEATURE_SPO2
    buf[0] = (char)('0' + (app_ctx.th.spo2_min / 10U));
    buf[1] = (char)('0' + (app_ctx.th.spo2_min % 10U));
    buf[2] = '\0';
    app_print_threshold_line(4U, "Smn", buf, APP_FIELD_SPO2_MIN);
#endif

#if VERSION_FEATURE_TEMP
    {
        int tmin = (int)(app_ctx.th.temp_min * 10.0f);
        int tmax = (int)(app_ctx.th.temp_max * 10.0f);
        app_display->print(0U, 5U, DISPLAY_FONT_SMALL, "%cTmn:%2d.%1d",
                           (app_ctx.threshold_field == APP_FIELD_TEMP_MIN) ? '*' : ' ',
                           tmin / 10, tmin % 10);
        app_display->print(0U, 6U, DISPLAY_FONT_SMALL, "%cTmx:%2d.%1d",
                           (app_ctx.threshold_field == APP_FIELD_TEMP_MAX) ? '*' : ' ',
                           tmax / 10, tmax % 10);
    }
#endif

#if VERSION_FEATURE_BLOOD_PRESSURE
    app_display->print(0U, 4U, DISPLAY_FONT_SMALL, "%cBmn:%3u",
                       (app_ctx.threshold_field == APP_FIELD_BP_MIN) ? '*' : ' ',
                       (unsigned int)app_ctx.th.bp_min);
    app_display->print(0U, 5U, DISPLAY_FONT_SMALL, "%cBmx:%3u",
                       (app_ctx.threshold_field == APP_FIELD_BP_MAX) ? '*' : ' ',
                       (unsigned int)app_ctx.th.bp_max);
#endif

    app_display->update();
}

static void app_display_refresh(void)
{
    if (app_ctx.mode == APP_MODE_THRESHOLD) {
        app_display_threshold();
        return;
    }

    app_display_auto();
}

static void app_adjust_threshold(int delta)
{
    switch (app_ctx.threshold_field) {
    case APP_FIELD_HR_MIN:
        if (delta > 0 && app_ctx.th.hr_min < 255U) {
            app_ctx.th.hr_min++;
        } else if (delta < 0 && app_ctx.th.hr_min > 0U) {
            app_ctx.th.hr_min--;
        }
        break;
    case APP_FIELD_HR_MAX:
        if (delta > 0 && app_ctx.th.hr_max < 255U) {
            app_ctx.th.hr_max++;
        } else if (delta < 0 && app_ctx.th.hr_max > 0U) {
            app_ctx.th.hr_max--;
        }
        break;
#if VERSION_FEATURE_SPO2
    case APP_FIELD_SPO2_MIN:
        if (delta > 0 && app_ctx.th.spo2_min < 100U) {
            app_ctx.th.spo2_min++;
        } else if (delta < 0 && app_ctx.th.spo2_min > 0U) {
            app_ctx.th.spo2_min--;
        }
        break;
#endif
#if VERSION_FEATURE_TEMP
    case APP_FIELD_TEMP_MIN:
        app_ctx.th.temp_min += (float)delta * 0.1f;
        if (app_ctx.th.temp_min < 30.0f) {
            app_ctx.th.temp_min = 30.0f;
        }
        break;
    case APP_FIELD_TEMP_MAX:
        app_ctx.th.temp_max += (float)delta * 0.1f;
        if (app_ctx.th.temp_max > 45.0f) {
            app_ctx.th.temp_max = 45.0f;
        }
        break;
#endif
#if VERSION_FEATURE_BLOOD_PRESSURE
    case APP_FIELD_BP_MIN:
        if (delta > 0 && app_ctx.th.bp_min < 255U) {
            app_ctx.th.bp_min++;
        } else if (delta < 0 && app_ctx.th.bp_min > 0U) {
            app_ctx.th.bp_min--;
        }
        break;
    case APP_FIELD_BP_MAX:
        if (delta > 0 && app_ctx.th.bp_max < 255U) {
            app_ctx.th.bp_max++;
        } else if (delta < 0 && app_ctx.th.bp_max > 0U) {
            app_ctx.th.bp_max--;
        }
        break;
#endif
    default:
        break;
    }
}

void app_logic_on_key(unsigned char key_id)
{
    if (key_id == 0U) {
        return;
    }

    if (key_id == 1U) {
        app_ctx.mode = (unsigned char)(app_ctx.mode == APP_MODE_AUTO ? APP_MODE_THRESHOLD : APP_MODE_AUTO);
        if (app_ctx.mode == APP_MODE_THRESHOLD) {
            app_ctx.threshold_field = APP_FIELD_HR_MIN;
            app_outputs_apply(0U);
        }
        app_ctx.display_dirty = 1U;
        return;
    }

    if (app_ctx.mode != APP_MODE_THRESHOLD) {
        return;
    }

    if (key_id == 2U) {
        app_ctx.threshold_field = app_next_field(app_ctx.threshold_field);
        app_ctx.display_dirty = 1U;
        return;
    }

    if (key_id == 3U) {
        app_adjust_threshold(1);
        app_ctx.display_dirty = 1U;
        return;
    }

    if (key_id == 4U) {
        app_adjust_threshold(-1);
        app_ctx.display_dirty = 1U;
    }
}

void app_logic_on_sensor_poll(void)
{
    app_read_sensors();
    app_check_alarm();
    app_ctx.display_dirty = 1U;
}

static int app_json_get_int(cJSON *root, const char *name, int *out_value)
{
    cJSON *item;

    item = cJSON_GetObjectItem(root, name);
    if ((item == 0) || ((item->type & cJSON_Number) == 0)) {
        return -1;
    }

    *out_value = item->valueint;
    return 0;
}

#if VERSION_FEATURE_TEMP
static int app_json_get_double(cJSON *root, const char *name, double *out_value)
{
    cJSON *item;

    item = cJSON_GetObjectItem(root, name);
    if ((item == 0) || ((item->type & cJSON_Number) == 0)) {
        return -1;
    }

    *out_value = item->valuedouble;
    return 0;
}
#endif

static void app_publish_status(void)
{
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
    cJSON *root;
    char *json_text;
    const char *mode_text;
    const char *topic = BOARD_ESP8266_MQTT_PUB_TOPIC;

    root = cJSON_CreateObject();
    if (root == 0) {
        return;
    }

    mode_text = (app_ctx.mode == APP_MODE_AUTO) ? "auto" : "threshold";
    (void)cJSON_AddNumberToObject(root, "hr", app_ctx.hr);
#if VERSION_FEATURE_SPO2
    (void)cJSON_AddNumberToObject(root, "spo2", app_ctx.spo2);
#endif
#if VERSION_FEATURE_TEMP
    (void)cJSON_AddNumberToObject(root, "temp", app_ctx.temp);
#endif
#if VERSION_FEATURE_BLOOD_PRESSURE
    (void)cJSON_AddNumberToObject(root, "bp_sys", app_ctx.bp_sys);
    (void)cJSON_AddNumberToObject(root, "bp_dia", app_ctx.bp_dia);
#endif
    (void)cJSON_AddNumberToObject(root, "alarm", app_ctx.alarm_active);
    (void)cJSON_AddStringToObject(root, "mode", mode_text);

    json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_text == 0) {
        return;
    }

#if VERSION_FEATURE_WIFI
    if (esp8266_mqtt_is_ready() != 0) {
        (void)esp8266_mqtt_publish_json(topic, json_text);
    }
#endif
#if VERSION_FEATURE_BLE
    if (VERSION_FEATURE_WIFI == 0) {
        const comm_driver_t *comm = devmgr_get_comm("jdy31");
        if (comm != 0) {
            (void)comm->send((const unsigned char *)json_text,
                             (unsigned short)strlen(json_text));
        }
    }
#endif

    cjson_release_string(json_text);
#endif
}

static void app_handle_comm_json(const char *text, unsigned short len)
{
    cJSON *root;
    cJSON *cmd;
    int value;
#if VERSION_FEATURE_TEMP
    double dvalue;
#endif

    root = cjson_parse(text, len);
    if (root == 0) {
        return;
    }

    cmd = cJSON_GetObjectItem(root, "cmd");
    if ((cmd == 0) || ((cmd->type & cJSON_String) == 0) || (cmd->valuestring == 0)) {
        cjson_release(root);
        return;
    }

    if (strcmp(cmd->valuestring, "set_threshold") == 0) {
        if (app_json_get_int(root, "hr_min", &value) == 0) {
            app_ctx.th.hr_min = (unsigned char)value;
        }
        if (app_json_get_int(root, "hr_max", &value) == 0) {
            app_ctx.th.hr_max = (unsigned char)value;
        }
#if VERSION_FEATURE_SPO2
        if (app_json_get_int(root, "spo2_min", &value) == 0) {
            app_ctx.th.spo2_min = (unsigned char)value;
        }
#endif
#if VERSION_FEATURE_TEMP
        if (app_json_get_double(root, "temp_min", &dvalue) == 0) {
            app_ctx.th.temp_min = (float)dvalue;
        }
        if (app_json_get_double(root, "temp_max", &dvalue) == 0) {
            app_ctx.th.temp_max = (float)dvalue;
        }
#endif
#if VERSION_FEATURE_BLOOD_PRESSURE
        if (app_json_get_int(root, "bp_min", &value) == 0) {
            app_ctx.th.bp_min = (unsigned char)value;
        }
        if (app_json_get_int(root, "bp_max", &value) == 0) {
            app_ctx.th.bp_max = (unsigned char)value;
        }
#endif
        app_ctx.display_dirty = 1U;
        app_publish_status();
    } else if (strcmp(cmd->valuestring, "get_status") == 0) {
        app_publish_status();
    }

    cjson_release(root);
}

static void app_handle_comm(void)
{
    const unsigned char *rx_data;
    unsigned short rx_len = 0U;

    rx_data = app_get_last_rx(&rx_len);
    if (rx_len == 0U) {
        return;
    }

    app_handle_comm_json((const char *)rx_data, rx_len);
    app_clear_last_rx();
}

void app_logic_init(void)
{
    app_pulse = devmgr_get_pulse_oximeter("max30102");
#if VERSION_FEATURE_TEMP
    app_temp = devmgr_get_temp_hum_sensor("ds18b20");
#endif
#if VERSION_FEATURE_BLOOD_PRESSURE
    app_bp = devmgr_get_blood_pressure("msp20");
#endif
    app_display = devmgr_get_display("oled");
    app_buzzer = devmgr_get_misc("buzzer");
    app_led = devmgr_get_misc("led");
#if VERSION_FEATURE_VOICE
    app_voice = devmgr_get_comm("su03t");
#endif

    app_ctx.mode = APP_MODE_AUTO;
    app_ctx.threshold_field = APP_FIELD_HR_MIN;
    app_ctx.th.hr_min = 60U;
    app_ctx.th.hr_max = 100U;
    app_ctx.th.spo2_min = 95U;
    app_ctx.th.temp_min = 35.5f;
    app_ctx.th.temp_max = 37.5f;
    app_ctx.th.bp_min = 90U;
    app_ctx.th.bp_max = 140U;
    app_ctx.display_dirty = 1U;

    app_read_sensors();
    app_display_refresh();
}

void app_logic_loop(sched_event_t events)
{
    if ((events & APP_EVENT_COMM_RX) != 0U) {
        app_handle_comm();
    }

    if ((events & APP_EVENT_KEY) != 0U) {
        app_ctx.display_dirty = 1U;
    }

    if (app_ctx.display_dirty != 0U) {
        app_display_refresh();
        app_ctx.display_dirty = 0U;
    }
}
