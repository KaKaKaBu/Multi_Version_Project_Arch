#include "app_logic.h"
#include "devmgr.h"
#include "sensor_if.h"
#include "analog_probe_if.h"
#if VERSION_FEATURE_WEIGHT
#include "weight_if.h"
#endif
#include "stepper_if.h"
#include "misc_if.h"
#include "display_if.h"
#include "board_config.h"
#include "tiny_printf.h"

#if VERSION_FEATURE_REMOTE
#include "cJSON.h"
#include "cjson_port.h"
#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif
#if VERSION_FEATURE_BLE
#include "comm_port.h"
#endif
#include <string.h>
#endif

#define ZNLYJ_ALARM_BLINK_MS 300U
#define ZNLYJ_STEP_DELAY_MS 3U
#define ZNLYJ_RACK_ANGLE_DEG 180U
#define ZNLYJ_STEP_THRESHOLD_COUNT 2U
#if VERSION_FEATURE_LIGHT
#define ZNLYJ_STEP_THRESHOLD_COUNT_LIGHT 4U
#endif

#define ZNLYJ_DEFAULT_TEMP_THR 30.0f
#define ZNLYJ_DEFAULT_HUMIDITY_THR 60.0f
#define ZNLYJ_DEFAULT_LIGHT_THR 50.0f
#define ZNLYJ_DEFAULT_WEIGHT_THR 5.0f

znlyj_context_t g_znlyj;

static const temp_hum_sensor_t *dht11;
static const analog_probe_t *presence;
static const analog_probe_t *ir_clothes;
static const stepper_driver_t *stepper;
static const misc_driver_t *buzzer_drv;
static const display_driver_t *display;
#if VERSION_FEATURE_LIGHT
static const analog_probe_t *light_sensor;
#endif
#if VERSION_FEATURE_WEIGHT
static const weight_sensor_t *weight_sensor;
#endif

/* 控制步进电机开合，并缓存当前晾衣架状态。 */
static void znlyj_set_rack(unsigned char open)
{
    if (stepper == 0) {
        return;
    }

    if (open != 0U) {
        stepper->step_angle(0U, ZNLYJ_RACK_ANGLE_DEG, ZNLYJ_STEP_DELAY_MS);
    } else {
        stepper->step_angle(1U, ZNLYJ_RACK_ANGLE_DEG, ZNLYJ_STEP_DELAY_MS);
    }
    g_znlyj.rack_open = (open != 0U) ? 1U : 0U;
}

/* 报警输出统一入口，避免多处直接操作蜂鸣器。 */
static void znlyj_set_alarm(unsigned char on)
{
    g_znlyj.alarm_on = (on != 0U) ? 1U : 0U;

    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_znlyj.alarm_on);
    }
}

/* 自动模式判定：无人、温度/湿度/光照条件满足时打开晾衣架。 */
static unsigned char znlyj_should_open(void)
{
    if (g_znlyj.human_present != 0U) {
        return 0U;
    }

    if (g_znlyj.temperature <= g_znlyj.temp_threshold) {
        return 0U;
    }
    if (g_znlyj.humidity >= g_znlyj.humidity_threshold) {
        return 0U;
    }
#if VERSION_FEATURE_LIGHT
    if (g_znlyj.light_percent < g_znlyj.light_threshold) {
        return 0U;
    }
#endif
    return 1U;
}

/* 报警判定：衣物受潮且有人靠近，或 v2+ 重量超限。 */
static unsigned char znlyj_check_alarm(void)
{
#if VERSION_FEATURE_WEIGHT
    if (g_znlyj.weight_overload != 0U) {
        return 1U;
    }
#endif
    if (g_znlyj.humidity > g_znlyj.humidity_threshold &&
        g_znlyj.human_present != 0U &&
        g_znlyj.clothes_present != 0U) {
        return 1U;
    }
    return 0U;
}

/* 将阈值项转换为 OLED 显示名称。 */
static const char *znlyj_threshold_name(unsigned char item)
{
    switch (item) {
    case 0U:  return "Temp";
    case 1U:  return "Humidity";
#if VERSION_FEATURE_LIGHT
    case 2U:  return "Light";
    case 3U:  return "Weight";
#endif
    default:  return "Temp";
    }
}

/* 按当前选择项调整阈值，并限制在合理范围内。 */
static void znlyj_adjust_threshold(int direction)
{
    unsigned char item = g_znlyj.selected_threshold;
    float *val = 0;

    switch (item) {
    case 0U: val = &g_znlyj.temp_threshold; break;
    case 1U: val = &g_znlyj.humidity_threshold; break;
#if VERSION_FEATURE_LIGHT
    case 2U: val = &g_znlyj.light_threshold; break;
    case 3U: val = &g_znlyj.weight_threshold; break;
#endif
    default: return;
    }

    if (val != 0) {
        *val += (float)direction;
        if (item == 0U) {
            if (*val < 10.0f) *val = 10.0f;
            if (*val > 50.0f) *val = 50.0f;
        } else if (item == 1U) {
            if (*val < 20.0f) *val = 20.0f;
            if (*val > 95.0f) *val = 95.0f;
#if VERSION_FEATURE_LIGHT
        } else if (item == 2U) {
            if (*val < 10.0f) *val = 10.0f;
            if (*val > 90.0f) *val = 90.0f;
        } else {
            if (*val < 1.0f) *val = 1.0f;
            if (*val > 20.0f) *val = 20.0f;
#endif
        }
    }
    g_znlyj.display_dirty = 1U;
}

/* 按当前模式刷新 OLED：普通界面显示状态，阈值模式显示可调项。 */
static void znlyj_refresh_display(void)
{
    if ((display == 0) || (g_znlyj.display_dirty == 0U)) {
        return;
    }

    display->clear();

    if (g_znlyj.mode == ZNLYJ_MODE_THRESHOLD) {
        display->print(0U, 0U, DISPLAY_FONT_SMALL, "Set Threshold");
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "%s",
                       znlyj_threshold_name(g_znlyj.selected_threshold));

        switch (g_znlyj.selected_threshold) {
        case 0U:
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%.0fC", g_znlyj.temp_threshold);
            break;
        case 1U:
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%.0f%%", g_znlyj.humidity_threshold);
            break;
#if VERSION_FEATURE_LIGHT
        case 2U:
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%.0f%%", g_znlyj.light_threshold);
            break;
        case 3U:
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%.1fkg", g_znlyj.weight_threshold);
            break;
#endif
        }
        display->print(0U, 7U, DISPLAY_FONT_SMALL, "K2 Sel K3+ K4-");
    } else {
        display->print(0U, 0U, DISPLAY_FONT_SMALL, "Rack:%s",
                       g_znlyj.rack_open ? "OPEN " : "CLOSE");
        display->print(0U, 1U, DISPLAY_FONT_SMALL, "T:%.0fC H:%.0f%%",
                       g_znlyj.temperature, g_znlyj.humidity);
#if VERSION_FEATURE_LIGHT
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "Light:%.0f%%", g_znlyj.light_percent);
#endif
        display->print(0U, 3U, DISPLAY_FONT_SMALL, "Human:%s",
                       g_znlyj.human_present ? "YES" : "NO ");
        display->print(0U, 4U, DISPLAY_FONT_SMALL, "Clothes:%s",
                       g_znlyj.clothes_present ? "YES" : "NO ");
#if VERSION_FEATURE_WEIGHT
        display->print(0U, 5U, DISPLAY_FONT_SMALL, "W:%.1fkg %s",
                       g_znlyj.weight_kg,
                       g_znlyj.weight_overload ? "OVER!" : "");
#endif
        display->print(0U, 6U, DISPLAY_FONT_SMALL, "Mode:%s",
                       (g_znlyj.mode == ZNLYJ_MODE_AUTO) ? "AUTO" : "MAN");
        display->print(0U, 7U, DISPLAY_FONT_SMALL,
                       g_znlyj.alarm_active ? "ALARM!" : "SAFE");
    }

    display->update();
    g_znlyj.display_dirty = 0U;
}

#if VERSION_FEATURE_REMOTE
/* 发布遥测 JSON：WiFi 版本走 MQTT，蓝牙版本走串口透传。 */
void znlyj_publish_telemetry(void)
{
    cJSON *root;
    cJSON *data;
    cJSON *thresholds;
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
        cJSON_AddNumberToObject(data, "temperature", g_znlyj.temperature);
        cJSON_AddNumberToObject(data, "humidity", g_znlyj.humidity);
        cJSON_AddNumberToObject(data, "rack_open", g_znlyj.rack_open);
        cJSON_AddNumberToObject(data, "human_present", g_znlyj.human_present);
        cJSON_AddNumberToObject(data, "clothes_present", g_znlyj.clothes_present);
        cJSON_AddNumberToObject(data, "alarm", g_znlyj.alarm_active);
#if VERSION_FEATURE_LIGHT
        cJSON_AddNumberToObject(data, "light", g_znlyj.light_percent);
#endif
#if VERSION_FEATURE_WEIGHT
        cJSON_AddNumberToObject(data, "weight_kg", g_znlyj.weight_kg);
        cJSON_AddNumberToObject(data, "weight_overload", g_znlyj.weight_overload);
#endif

        thresholds = cJSON_CreateObject();
        if (thresholds != 0) {
            cJSON_AddNumberToObject(thresholds, "temp", g_znlyj.temp_threshold);
            cJSON_AddNumberToObject(thresholds, "humidity", g_znlyj.humidity_threshold);
#if VERSION_FEATURE_LIGHT
            cJSON_AddNumberToObject(thresholds, "light", g_znlyj.light_threshold);
            cJSON_AddNumberToObject(thresholds, "weight", g_znlyj.weight_threshold);
#endif
            cJSON_AddItemToObject(data, "thresholds", thresholds);
        }
        cJSON_AddItemToObject(root, "data", data);
    }

    json_text = cJSON_PrintUnformatted(root);
    if (json_text != 0) {
#if VERSION_FEATURE_WIFI
        if (esp8266_mqtt_is_ready() != 0) {
            (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, json_text);
        }
#elif VERSION_FEATURE_BLE
        (void)comm_port_send((const unsigned char *)json_text, (unsigned short)strlen(json_text));
#endif
        cjson_release_string(json_text);
    }
    cjson_release(root);
}

/* 兼容两种下行格式：优先读取 params，没有 params 时使用根对象。 */
static const cJSON *znlyj_json_params(const cJSON *root)
{
    const cJSON *params = cJSON_GetObjectItem(root, "params");
    if ((params != 0) && ((params->type & cJSON_Object) != 0)) {
        return params;
    }
    return root;
}

/* 应用远程阈值设置；v2+ 才解析光照和重量阈值。 */
static void znlyj_apply_threshold_json(const cJSON *params)
{
    const cJSON *item;

    item = cJSON_GetObjectItem(params, "temp");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_znlyj.temp_threshold = (float)item->valuedouble;
    }

    item = cJSON_GetObjectItem(params, "humidity");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_znlyj.humidity_threshold = (float)item->valuedouble;
    }
#if VERSION_FEATURE_LIGHT
    item = cJSON_GetObjectItem(params, "light");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_znlyj.light_threshold = (float)item->valuedouble;
    }

    item = cJSON_GetObjectItem(params, "weight");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_znlyj.weight_threshold = (float)item->valuedouble;
    }
#endif
    g_znlyj.display_dirty = 1U;
}

/* 标记需要发送遥测，实际发送由 comm_loop_run 做节流。 */
void znlyj_request_telemetry(void)
{
    g_znlyj.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
}

/* 远程命令入口：解析 set_threshold/set_rack/set_mode/get_status。 */
void znlyj_on_remote_rx(const char *json_data)
{
    cJSON *root;
    cJSON *cmd_item;
    const cJSON *params;
    const cJSON *rack_item;
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
    params = znlyj_json_params(root);

    if (strcmp(cmd, "set_threshold") == 0) {
        znlyj_apply_threshold_json(params);
    } else if (strcmp(cmd, "set_rack") == 0) {
        rack_item = cJSON_GetObjectItem(params, "open");
        if ((rack_item != 0) && ((rack_item->type & cJSON_Number) != 0)) {
            znlyj_set_rack((rack_item->valueint != 0) ? 1U : 0U);
            g_znlyj.mode = ZNLYJ_MODE_MANUAL;
        }
    } else if (strcmp(cmd, "set_mode") == 0) {
        rack_item = cJSON_GetObjectItem(params, "mode");
        if ((rack_item != 0) && cJSON_IsString(rack_item)) {
            if (strcmp(rack_item->valuestring, "auto") == 0) {
                g_znlyj.mode = ZNLYJ_MODE_AUTO;
            } else if (strcmp(rack_item->valuestring, "manual") == 0) {
                g_znlyj.mode = ZNLYJ_MODE_MANUAL;
            } else if (strcmp(rack_item->valuestring, "threshold") == 0) {
                g_znlyj.mode = ZNLYJ_MODE_THRESHOLD;
            }
        }
    } else if (strcmp(cmd, "get_status") == 0) {
        g_znlyj.telemetry_pending = 1U;
    }

    g_znlyj.display_dirty = 1U;
    znlyj_request_telemetry();
    cjson_release(root);
}
#endif

/* 按键入口：Key1 切模式，Key2 开合/选择，Key3/Key4 调整阈值。 */
void znlyj_logic_on_key(unsigned char key_id)
{
    if (key_id == 1U) {
        g_znlyj.mode = (znlyj_mode_t)(((unsigned char)g_znlyj.mode + 1U) % 3U);
        if (g_znlyj.mode == ZNLYJ_MODE_AUTO) {
            unsigned char should = znlyj_should_open();
            znlyj_set_rack(should);
        }
        g_znlyj.display_dirty = 1U;
        return;
    }

    if (g_znlyj.mode == ZNLYJ_MODE_MANUAL) {
        if (key_id == 2U) {
            znlyj_set_rack(g_znlyj.rack_open ? 0U : 1U);
            g_znlyj.display_dirty = 1U;
        }
        return;
    }

    if (g_znlyj.mode == ZNLYJ_MODE_THRESHOLD) {
        if (key_id == 2U) {
#if VERSION_FEATURE_LIGHT
            g_znlyj.selected_threshold = (znlyj_threshold_item_t)(
                ((unsigned char)g_znlyj.selected_threshold + 1U) % ZNLYJ_STEP_THRESHOLD_COUNT_LIGHT);
#else
            g_znlyj.selected_threshold = (unsigned char)(
                ((unsigned char)g_znlyj.selected_threshold + 1U) % ZNLYJ_STEP_THRESHOLD_COUNT);
#endif
            g_znlyj.display_dirty = 1U;
            return;
        }
        if (key_id == 3U) {
            znlyj_adjust_threshold(1);
#if VERSION_FEATURE_REMOTE
            znlyj_request_telemetry();
#endif
            return;
        }
        if (key_id == 4U) {
            znlyj_adjust_threshold(-1);
#if VERSION_FEATURE_REMOTE
            znlyj_request_telemetry();
#endif
        }
    }
}

/* 传感器循环：采集环境、人/衣物、光照、重量，并驱动自动开合判定。 */
void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (dht11 != 0) {
        g_znlyj.temperature = dht11->read_temperature();
        g_znlyj.humidity = dht11->read_humidity();
    }
    if (presence != 0) {
        g_znlyj.human_present = (presence->read_value() >= 0.5f) ? 1U : 0U;
    }
    if (ir_clothes != 0) {
        g_znlyj.clothes_present = (ir_clothes->read_value() >= 0.5f) ? 1U : 0U;
    }
#if VERSION_FEATURE_LIGHT
    if (light_sensor != 0) {
        g_znlyj.light_percent = light_sensor->read_value();
    }
#endif
#if VERSION_FEATURE_WEIGHT
    if (weight_sensor != 0) {
        g_znlyj.weight_kg = weight_sensor->read_grams() / 1000.0f;
        g_znlyj.weight_overload = (g_znlyj.weight_kg > g_znlyj.weight_threshold) ? 1U : 0U;
    }
#endif

    if (g_znlyj.mode == ZNLYJ_MODE_AUTO) {
        unsigned char should = znlyj_should_open();
        if (should != g_znlyj.rack_open) {
            znlyj_set_rack(should);
        }
    }

    g_znlyj.alarm_active = znlyj_check_alarm();
    g_znlyj.display_dirty = 1U;
}

/* 报警循环：报警激活时周期翻转蜂鸣器，形成闪烁/间歇提示。 */
void alarm_loop_run(sched_event_t events, void *ctx)
{
    unsigned long now;

    (void)events;
    (void)ctx;

    now = sched_tick_get();

    if (g_znlyj.alarm_active == 0U) {
        if (g_znlyj.alarm_on != 0U) {
            znlyj_set_alarm(0U);
        }
        return;
    }

    if ((now - g_znlyj.last_alarm_toggle_tick) >= ZNLYJ_ALARM_BLINK_MS) {
        znlyj_set_alarm(g_znlyj.alarm_on ? 0U : 1U);
        g_znlyj.last_alarm_toggle_tick = now;
        g_znlyj.display_dirty = 1U;
    }
}

/* 业务循环：根据事件刷新显示，避免在每个事件源里直接操作 OLED。 */
void logic_loop_run(sched_event_t events, void *ctx)
{
    (void)ctx;

    if ((events & APP_EVENT_KEY) != 0U) {
        g_znlyj.display_dirty = 1U;
    }
    if ((events & APP_EVENT_SENSOR) != 0U) {
        g_znlyj.display_dirty = 1U;
    }
#if VERSION_FEATURE_REMOTE
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_COMM_TX)) != 0U) {
        g_znlyj.display_dirty = 1U;
    }
#endif

    if (g_znlyj.display_dirty != 0U) {
        znlyj_refresh_display();
        g_znlyj.display_dirty = 0U;
    }
}

/* 初始化业务驱动句柄和默认阈值。 */
void znlyj_logic_init(void)
{
    dht11 = devmgr_get_temp_hum_sensor("dht11");
    presence = devmgr_get_analog_probe("presence");
    ir_clothes = devmgr_get_analog_probe("ir_clothes");
    stepper = devmgr_get_stepper("stepmotor");
    buzzer_drv = devmgr_get_misc("buzzer");
    display = devmgr_get_display("oled");
#if VERSION_FEATURE_LIGHT
    light_sensor = devmgr_get_analog_probe("gl5506");
#endif
#if VERSION_FEATURE_WEIGHT
    weight_sensor = devmgr_get_weight_sensor("hx711");
#endif
#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif

    g_znlyj.mode = ZNLYJ_MODE_AUTO;
    g_znlyj.selected_threshold = 0U;
    g_znlyj.rack_open = 0U;
    g_znlyj.temperature = 0.0f;
    g_znlyj.humidity = 0.0f;
    g_znlyj.human_present = 0U;
    g_znlyj.clothes_present = 0U;
    g_znlyj.alarm_active = 0U;
    g_znlyj.alarm_on = 0U;
    g_znlyj.last_alarm_toggle_tick = sched_tick_get();
    g_znlyj.temp_threshold = ZNLYJ_DEFAULT_TEMP_THR;
    g_znlyj.humidity_threshold = ZNLYJ_DEFAULT_HUMIDITY_THR;
#if VERSION_FEATURE_LIGHT
    g_znlyj.light_percent = 0.0f;
    g_znlyj.weight_kg = 0.0f;
    g_znlyj.weight_overload = 0U;
    g_znlyj.light_threshold = ZNLYJ_DEFAULT_LIGHT_THR;
    g_znlyj.weight_threshold = ZNLYJ_DEFAULT_WEIGHT_THR;
#endif
#if VERSION_FEATURE_REMOTE
    g_znlyj.telemetry_pending = 1U;
    g_znlyj.last_telemetry_tick = 0U;
#endif
    g_znlyj.display_dirty = 1U;

    znlyj_set_alarm(0U);
    znlyj_refresh_display();
}

#if VERSION_FEATURE_REMOTE
/* 通讯循环：WiFi 轮询 MQTT，并按固定间隔发送待发布遥测。 */
void comm_loop_run(sched_event_t events, void *ctx)
{
    unsigned long now;

    (void)events;
    (void)ctx;
#if VERSION_FEATURE_WIFI
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_TICK)) != 0U) {
        esp8266_mqtt_poll();
    }
#endif

    now = sched_tick_get();
    if (g_znlyj.telemetry_pending != 0U) {
        if ((now - g_znlyj.last_telemetry_tick) >= APP_TELEMETRY_INTERVAL_MS) {
            g_znlyj.telemetry_pending = 0U;
            g_znlyj.last_telemetry_tick = now;
            znlyj_publish_telemetry();
        }
    }
}
#endif
