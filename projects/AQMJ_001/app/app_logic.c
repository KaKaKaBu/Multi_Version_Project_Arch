#include "app_logic.h"
#include "version_config.h"
#include "board_config.h"

#include "actuator_if.h"
#include "analog_probe_if.h"
#include "audio_recorder_if.h"
#include "comm_if.h"
#include "comm_port.h"
#include "cJSON.h"
#include "cjson_port.h"
#include "devmgr.h"
#include "display_if.h"
#include "input_if.h"
#include "misc_if.h"
#include "tiny_printf.h"

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
#include "esp8266_mqtt.h"
#endif

#include <string.h>

#define AQMJ_STAY_ALERT_MS 8000U
#define AQMJ_ALARM_TOGGLE_MS 300U
#define AQMJ_TELEMETRY_INTERVAL_MS 2000U
#define AQMJ_JSON_BUFFER_SIZE 384U

static aqmj_state_t g_aqmj;

static const rtc_driver_t *rtc_drv;
static const display_driver_t *display_drv;
static const misc_driver_t *led_drv;
static const misc_driver_t *buzzer_drv;
static const analog_probe_t *presence_probe;
#if VERSION_FEATURE_LIGHT
static const analog_probe_t *light_probe;
static const relay_driver_t *lamp_drv;
#endif
#if VERSION_FEATURE_MESSAGE
static const audio_recorder_driver_t *audio_drv;
#endif
#if VERSION_FEATURE_TTS
static const comm_driver_t *tts_drv;
#endif
#if VERSION_FEATURE_IR_REMOTE
static const input_driver_t *ir_drv;
#endif

#if VERSION_FEATURE_TTS
static const unsigned char aqmj_voice_doorbell[] = {
    0xC3U, 0xC5U, 0xC1U, 0xE5U, 0xBAU, 0xF4U, 0xBDU, 0xD0U
};
static const unsigned char aqmj_voice_away[] = {
    0xC4U, 0xFAU, 0xBAU, 0xC3U, 0xD6U, 0xF7U, 0xC8U, 0xCBU,
    0xCDU, 0xE2U, 0xB3U, 0xF6U, 0xBCU, 0xD2U, 0xC0U, 0xEFU,
    0xC3U, 0xBBU, 0xC8U, 0xCBU
};
static const unsigned char aqmj_voice_stay[] = {
    0xC3U, 0xC5U, 0xC7U, 0xB0U, 0xD3U, 0xD0U, 0xC8U, 0xCBU,
    0xB6U, 0xBAU, 0xC1U, 0xF4U
};
#endif

static void aqmj_set_alarm_output(uint8_t on)
{
    g_aqmj.alarm_output_on = (on != 0U) ? 1U : 0U;
    if (led_drv != 0) {
        led_drv->set_state(g_aqmj.alarm_output_on);
    }
    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_aqmj.alarm_output_on);
    }
}

static void aqmj_set_lamp(uint8_t on)
{
#if VERSION_FEATURE_LIGHT
    g_aqmj.lamp_on = (on != 0U) ? 1U : 0U;
    if (lamp_drv != 0) {
        lamp_drv->set_state(g_aqmj.lamp_on);
    }
#else
    (void)on;
#endif
}

static void aqmj_stop_alarm(void)
{
    g_aqmj.alarm_active = 0U;
    g_aqmj.ir_alarm_latched = 0U;
    aqmj_set_alarm_output(0U);
    g_aqmj.display_dirty = 1U;
    app_logic_request_telemetry();
}

static void aqmj_start_alarm(void)
{
    if (g_aqmj.alarm_active == 0U) {
        g_aqmj.last_alarm_toggle_tick = sched_tick_get();
    }
    g_aqmj.alarm_active = 1U;
    aqmj_set_alarm_output(1U);
    g_aqmj.display_dirty = 1U;
    app_logic_request_telemetry();
}

#if VERSION_FEATURE_TTS
static void aqmj_voice_send(const unsigned char *data, unsigned short len)
{
    if ((tts_drv != 0) && (tts_drv->send != 0) && (data != 0) && (len != 0U)) {
        (void)tts_drv->send(data, len);
    }
}
#endif

static void aqmj_handle_doorbell(void)
{
    g_aqmj.doorbell_pressed = 1U;
    g_aqmj.last_call_time = g_aqmj.now;
    if (g_aqmj.owner_home != 0U) {
#if VERSION_FEATURE_TTS
        aqmj_voice_send(aqmj_voice_doorbell, (unsigned short)sizeof(aqmj_voice_doorbell));
#endif
        aqmj_start_alarm();
    } else {
#if VERSION_FEATURE_TTS
        aqmj_voice_send(aqmj_voice_away, (unsigned short)sizeof(aqmj_voice_away));
#endif
    }
    g_aqmj.display_dirty = 1U;
    app_logic_request_telemetry();
}

#if VERSION_FEATURE_MESSAGE
static void aqmj_audio_record(uint8_t on)
{
    if (audio_drv == 0) {
        return;
    }
    audio_drv->set_record(on);
    g_aqmj.recording = (on != 0U) ? 1U : 0U;
    if (on != 0U) {
        g_aqmj.record_stop_tick = sched_tick_get() + BOARD_ISD1820_RECORD_MS;
        g_aqmj.message_pending = 1U;
    }
    g_aqmj.display_dirty = 1U;
}

static void aqmj_audio_play(uint8_t on)
{
    if (audio_drv == 0) {
        return;
    }
    audio_drv->set_play(on);
    g_aqmj.playing_message = (on != 0U) ? 1U : 0U;
    if (on != 0U) {
        g_aqmj.play_stop_tick = sched_tick_get() + BOARD_ISD1820_PLAY_MS;
        g_aqmj.message_pending = 0U;
    }
    g_aqmj.display_dirty = 1U;
}

static void aqmj_audio_poll(void)
{
    uint32_t now = sched_tick_get();

    if ((g_aqmj.recording != 0U) && ((int32_t)(now - g_aqmj.record_stop_tick) >= 0)) {
        aqmj_audio_record(0U);
    }
    if ((g_aqmj.playing_message != 0U) && ((int32_t)(now - g_aqmj.play_stop_tick) >= 0)) {
        aqmj_audio_play(0U);
    }
}
#endif

static void aqmj_default_state(void)
{
    memset(&g_aqmj, 0, sizeof(g_aqmj));
    g_aqmj.owner_home = 1U;
    g_aqmj.armed = 1U;
    g_aqmj.video_enabled = VERSION_FEATURE_VIDEO ? 1U : 0U;
    g_aqmj.display_dirty = 1U;
    g_aqmj.telemetry_pending = 1U;
    g_aqmj.now.year = 2026U;
    g_aqmj.now.month = 7U;
    g_aqmj.now.day = 8U;
    g_aqmj.now.week = 3U;
}

void app_logic_init(void)
{
    aqmj_default_state();

    display_drv = devmgr_get_display("oled");
    led_drv = devmgr_get_misc("led");
    buzzer_drv = devmgr_get_misc("buzzer");
    presence_probe = devmgr_get_analog_probe("presence");
#if VERSION_FEATURE_LIGHT
    light_probe = devmgr_get_analog_probe("gl5506");
    lamp_drv = devmgr_get_relay("relay");
#endif
#if VERSION_FEATURE_MESSAGE
    audio_drv = devmgr_get_audio_recorder("isd1820");
#endif
#if VERSION_FEATURE_TTS
    tts_drv = devmgr_get_comm("tts_uart");
#endif
#if VERSION_FEATURE_IR_REMOTE
    ir_drv = devmgr_get_input("ir_remote");
#else
    rtc_drv = devmgr_get_rtc("ds1302");
    if (rtc_drv != 0) {
        rtc_drv->read_time(&g_aqmj.now);
        g_aqmj.last_call_time = g_aqmj.now;
    }
#endif

#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif
    aqmj_set_alarm_output(0U);
    aqmj_set_lamp(0U);
}

void app_logic_handle_key(uint8_t key_index)
{
    switch (key_index) {
    case 1U:
        g_aqmj.owner_home = (g_aqmj.owner_home == 0U) ? 1U : 0U;
        break;
    case 2U:
        g_aqmj.armed = (g_aqmj.armed == 0U) ? 1U : 0U;
        if (g_aqmj.armed == 0U) {
            aqmj_stop_alarm();
        }
        break;
    case 3U:
#if VERSION_FEATURE_MESSAGE
        aqmj_audio_play((g_aqmj.playing_message == 0U) ? 1U : 0U);
#else
        aqmj_stop_alarm();
#endif
        break;
    case 4U:
#if VERSION_FEATURE_MESSAGE
        aqmj_audio_record((g_aqmj.recording == 0U) ? 1U : 0U);
#elif VERSION_FEATURE_LIGHT
        aqmj_set_lamp((g_aqmj.lamp_on == 0U) ? 1U : 0U);
#else
        aqmj_stop_alarm();
#endif
        break;
    case 5U:
        aqmj_handle_doorbell();
        break;
    default:
        break;
    }

    g_aqmj.display_dirty = 1U;
    app_logic_request_telemetry();
}

void rtc_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (rtc_drv != 0) {
        rtc_drv->read_time(&g_aqmj.now);
        g_aqmj.display_dirty = 1U;
    }
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
#if !VERSION_FEATURE_IR_REMOTE
    uint8_t was_present;
    uint32_t now;
#endif

    (void)events;
    (void)ctx;

#if VERSION_FEATURE_IR_REMOTE
    return;
#else
    was_present = g_aqmj.presence;
    if (presence_probe != 0) {
        g_aqmj.presence = (presence_probe->read_value() > 0.5f) ? 1U : 0U;
    }

#if VERSION_FEATURE_LIGHT
    if (light_probe != 0) {
        float light = light_probe->read_value();
        if (light < 0.0f) {
            light = 0.0f;
        }
        if (light > 100.0f) {
            light = 100.0f;
        }
        g_aqmj.light_value = (uint16_t)light;
    }

    if ((g_aqmj.presence != 0U) && (g_aqmj.light_value < BOARD_LIGHT_DARK_THRESHOLD) &&
        (g_aqmj.alarm_active == 0U)) {
        aqmj_set_lamp(1U);
    } else if (g_aqmj.alarm_active == 0U) {
        aqmj_set_lamp(0U);
    }
#endif

    now = sched_tick_get();
    if ((g_aqmj.presence != 0U) && (was_present == 0U)) {
        g_aqmj.presence_start_tick = now;
    }
    if (g_aqmj.presence == 0U) {
        g_aqmj.presence_start_tick = 0U;
        g_aqmj.stay_seconds = 0U;
    } else if (g_aqmj.presence_start_tick != 0U) {
        g_aqmj.stay_seconds = (uint8_t)((now - g_aqmj.presence_start_tick) / 1000U);
        if ((g_aqmj.armed != 0U) && ((now - g_aqmj.presence_start_tick) >= AQMJ_STAY_ALERT_MS)) {
#if VERSION_FEATURE_TTS
            aqmj_voice_send(aqmj_voice_stay, (unsigned short)sizeof(aqmj_voice_stay));
#endif
            aqmj_start_alarm();
        }
    }

#if VERSION_FEATURE_MESSAGE
    aqmj_audio_poll();
#endif

    if (was_present != g_aqmj.presence) {
        g_aqmj.display_dirty = 1U;
        app_logic_request_telemetry();
    }
#endif
}

void alarm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now;

    (void)events;
    (void)ctx;

    if (g_aqmj.alarm_active == 0U) {
        return;
    }

    now = sched_tick_get();
    if ((now - g_aqmj.last_alarm_toggle_tick) >= AQMJ_ALARM_TOGGLE_MS) {
        aqmj_set_alarm_output((g_aqmj.alarm_output_on == 0U) ? 1U : 0U);
#if VERSION_FEATURE_LIGHT
        if ((g_aqmj.armed != 0U) && (g_aqmj.presence != 0U)) {
            aqmj_set_lamp((g_aqmj.lamp_on == 0U) ? 1U : 0U);
        }
#endif
        g_aqmj.last_alarm_toggle_tick = now;
    }
}

static void aqmj_display_main(void)
{
    if (display_drv == 0) {
        return;
    }

    display_drv->print(0U, 0U, DISPLAY_FONT_SMALL, "AQMJ-%02u %02u:%02u:%02u",
                       (unsigned)APP_VERSION,
                       (unsigned)g_aqmj.now.hour,
                       (unsigned)g_aqmj.now.minute,
                       (unsigned)g_aqmj.now.second);
    display_drv->print(0U, 1U, DISPLAY_FONT_SMALL, "%s %s 人:%u 警:%u",
                       g_aqmj.owner_home ? "在家" : "外出",
                       g_aqmj.armed ? "设防" : "撤防",
                       (unsigned)g_aqmj.presence,
                       (unsigned)g_aqmj.alarm_active);
#if VERSION_FEATURE_LIGHT
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "光:%u 灯:%u",
                       (unsigned)g_aqmj.light_value,
                       (unsigned)g_aqmj.lamp_on);
#elif VERSION_FEATURE_IR_REMOTE
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "遥控报警:%u", (unsigned)g_aqmj.ir_alarm_latched);
#else
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "逗留:%us 门铃:%u",
                       (unsigned)g_aqmj.stay_seconds,
                       (unsigned)g_aqmj.doorbell_pressed);
#endif
#if VERSION_FEATURE_MESSAGE
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "留言:%u 录:%u 放:%u",
                       (unsigned)g_aqmj.message_pending,
                       (unsigned)g_aqmj.recording,
                       (unsigned)g_aqmj.playing_message);
#else
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "最近%02u:%02u 视:%u",
                       (unsigned)g_aqmj.last_call_time.hour,
                       (unsigned)g_aqmj.last_call_time.minute,
                       (unsigned)g_aqmj.video_enabled);
#endif
}

void display_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if ((display_drv == 0) || (g_aqmj.display_dirty == 0U)) {
        return;
    }

    g_aqmj.display_dirty = 0U;
    display_drv->clear();
    aqmj_display_main();
    display_drv->update();
}

#if VERSION_FEATURE_REMOTE
static void aqmj_json_add_state(cJSON *obj)
{
    cJSON_AddNumberToObject(obj, "version", APP_VERSION);
    cJSON_AddNumberToObject(obj, "home", g_aqmj.owner_home);
    cJSON_AddNumberToObject(obj, "armed", g_aqmj.armed);
    cJSON_AddNumberToObject(obj, "presence", g_aqmj.presence);
    cJSON_AddNumberToObject(obj, "alarm", g_aqmj.alarm_active);
    cJSON_AddNumberToObject(obj, "lamp", g_aqmj.lamp_on);
    cJSON_AddNumberToObject(obj, "light", g_aqmj.light_value);
    cJSON_AddNumberToObject(obj, "message", g_aqmj.message_pending);
    cJSON_AddNumberToObject(obj, "video", g_aqmj.video_enabled);
    cJSON_AddNumberToObject(obj, "last_call_hour", g_aqmj.last_call_time.hour);
    cJSON_AddNumberToObject(obj, "last_call_minute", g_aqmj.last_call_time.minute);
}

static void aqmj_publish_telemetry(void)
{
    cJSON *root;
    cJSON *data;
    char *json_text;
#if VERSION_FEATURE_CLOUD
    char *properties_text;
#endif

    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if ((root == 0) || (data == 0)) {
        if (root != 0) {
            cjson_release(root);
        }
        if (data != 0) {
            cjson_release(data);
        }
        return;
    }

    aqmj_json_add_state(data);
#if VERSION_FEATURE_CLOUD
    properties_text = cJSON_PrintUnformatted(data);
    if (properties_text != 0) {
        (void)esp8266_mqtt_publish_huawei_properties(properties_text);
        cjson_release_string(properties_text);
    }
#endif

    cJSON_AddStringToObject(root, "device", "AQMJ_001");
    cJSON_AddItemToObject(root, "data", data);
    json_text = cJSON_PrintUnformatted(root);
    if (json_text != 0) {
#if VERSION_FEATURE_WIFI
        const esp8266_mqtt_config_t *cfg = esp8266_mqtt_active_config();
        (void)esp8266_mqtt_publish_json((cfg != 0) ? cfg->pub_topic : 0, json_text);
#elif VERSION_FEATURE_BLE
        (void)comm_port_send((const unsigned char *)json_text, (unsigned short)strlen(json_text));
        (void)comm_port_send((const unsigned char *)"\n", 1U);
#endif
        cjson_release_string(json_text);
    }
    cjson_release(root);
}

static cJSON *aqmj_json_child(cJSON *parent, const char *name)
{
    cJSON *child;

    if ((parent == 0) || (name == 0)) {
        return 0;
    }
    child = cJSON_GetObjectItem(parent, name);
    if (child != 0) {
        return child;
    }
    child = cJSON_GetObjectItem(parent, "params");
    if (child != 0) {
        cJSON *nested = cJSON_GetObjectItem(child, name);
        if (nested != 0) {
            return nested;
        }
    }
    child = cJSON_GetObjectItem(parent, "properties");
    if (child != 0) {
        cJSON *nested = cJSON_GetObjectItem(child, name);
        if (nested != 0) {
            return nested;
        }
    }
    child = cJSON_GetObjectItem(parent, "services");
    if ((child != 0) && (child->child != 0)) {
        cJSON *props = cJSON_GetObjectItem(child->child, "properties");
        if (props != 0) {
            return cJSON_GetObjectItem(props, name);
        }
    }
    return 0;
}

static uint8_t aqmj_json_bool(cJSON *root, const char *name, uint8_t old_value)
{
    cJSON *item = aqmj_json_child(root, name);
    if (item == 0) {
        return old_value;
    }
    if ((item->type & (cJSON_True | cJSON_False)) != 0) {
        return ((item->type & cJSON_True) != 0) ? 1U : 0U;
    }
    if ((item->type & 0xFF) == cJSON_Number) {
        return (item->valuedouble != 0.0) ? 1U : 0U;
    }
    return old_value;
}

void app_logic_on_remote_rx(const char *json_data)
{
    cJSON *root;
    cJSON *cmd_item;
    const char *cmd = 0;

    if (json_data == 0) {
        return;
    }

    root = cjson_parse(json_data, strlen(json_data));
    if (root == 0) {
        return;
    }

    cmd_item = aqmj_json_child(root, "cmd");
    if ((cmd_item == 0) || !cJSON_IsString(cmd_item)) {
        cmd_item = aqmj_json_child(root, "command");
    }
    if ((cmd_item != 0) && cJSON_IsString(cmd_item)) {
        cmd = cmd_item->valuestring;
    }

    if (cmd != 0) {
        if (strcmp(cmd, "get_status") == 0) {
            app_logic_request_telemetry();
        } else if (strcmp(cmd, "clear_alarm") == 0) {
            aqmj_stop_alarm();
        } else if (strcmp(cmd, "doorbell") == 0) {
            aqmj_handle_doorbell();
#if VERSION_FEATURE_MESSAGE
        } else if (strcmp(cmd, "play_message") == 0) {
            aqmj_audio_play(1U);
        } else if (strcmp(cmd, "record_start") == 0) {
            aqmj_audio_record(1U);
        } else if (strcmp(cmd, "record_stop") == 0) {
            aqmj_audio_record(0U);
#endif
        }
    }

    g_aqmj.armed = aqmj_json_bool(root, "armed", g_aqmj.armed);
    g_aqmj.owner_home = aqmj_json_bool(root, "home", g_aqmj.owner_home);
#if VERSION_FEATURE_LIGHT
    aqmj_set_lamp(aqmj_json_bool(root, "lamp", g_aqmj.lamp_on));
#endif
    g_aqmj.display_dirty = 1U;
    app_logic_request_telemetry();
    cjson_release(root);
}

void app_logic_request_telemetry(void)
{
    g_aqmj.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
}

void comm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now;

    (void)ctx;
    (void)events;

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
    app_comm_poll(events);
#endif

    now = sched_tick_get();
    if ((now - g_aqmj.last_telemetry_tick) >= AQMJ_TELEMETRY_INTERVAL_MS) {
        g_aqmj.telemetry_pending = 1U;
    }
    if (g_aqmj.telemetry_pending != 0U) {
        g_aqmj.telemetry_pending = 0U;
        g_aqmj.last_telemetry_tick = now;
        aqmj_publish_telemetry();
    }
}
#else
void app_logic_on_remote_rx(const char *json_data)
{
    (void)json_data;
}

void app_logic_request_telemetry(void)
{
}

void comm_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
}
#endif

void ir_loop_run(sched_event_t events, void *ctx)
{
#if VERSION_FEATURE_IR_REMOTE
    unsigned char key;
#endif

    (void)events;
    (void)ctx;

#if VERSION_FEATURE_IR_REMOTE
    if (ir_drv == 0) {
        return;
    }
    key = ir_drv->read_key();
    if (key == 0U) {
        return;
    }
    if (key == 5U) {
        aqmj_stop_alarm();
    } else {
        g_aqmj.ir_alarm_latched = 1U;
        aqmj_start_alarm();
    }
    g_aqmj.display_dirty = 1U;
#endif
}
