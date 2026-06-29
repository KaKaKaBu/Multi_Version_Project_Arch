#include "app_logic.h"
#include "devmgr.h"
#include "gas_if.h"
#include "sensor_if.h"
#include "actuator_if.h"
#include "misc_if.h"
#include "display_if.h"
#include "board_config.h"
#if !defined(PLATFORM_MCS51)
#include "tiny_printf.h"
#else
#include "mcs51_memory.h"
#endif

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

#define APP_DEVICE_FAN 0U
#define APP_DEVICE_ALARM 1U

#define APP_DEFAULT_TEMP_THRESHOLD 35U
#define APP_MIN_TEMP_THRESHOLD 10U
#define APP_MAX_TEMP_THRESHOLD 60U
#define APP_TEMP_STEP 1U

#define APP_DEFAULT_AQ_THRESHOLD_PPM 500U
#define APP_MIN_AQ_THRESHOLD_PPM 100U
#define APP_MAX_AQ_THRESHOLD_PPM 2000U
#define APP_AQ_STEP_PPM 50U

#define APP_DEFAULT_CO_THRESHOLD_PPM 100U
#define APP_MIN_CO_THRESHOLD_PPM 10U
#define APP_MAX_CO_THRESHOLD_PPM 500U
#define APP_CO_STEP_PPM 10U

#define APP_TELEMETRY_INTERVAL_MS 3000U

#if defined(PLATFORM_MCS51)
MCS51_XDATA app_context_t g_ctx;
#else
app_context_t g_ctx;
#endif

static const gas_sensor_t *aq_sensor;
static const gas_sensor_t *co_sensor;
static const temp_hum_sensor_t *temp_sensor;
static const relay_driver_t *fan_relay;
static const misc_driver_t *buzzer_drv;
static const misc_driver_t *led_drv;
static const display_driver_t *display;

/* 根据当前风扇/报警状态同步继电器、蜂鸣器和 LED 输出。 */
static void app_apply_output(void)
{
    if (fan_relay != 0) {
        fan_relay->set_state(g_ctx.fan_on);
    }
    if (g_ctx.alarm_on != 0U) {
        if (buzzer_drv != 0) {
            buzzer_drv->set_state(1U);
        }
        if (led_drv != 0) {
            led_drv->set_state(1U);
        }
    } else {
        if (buzzer_drv != 0) {
            buzzer_drv->set_state(0U);
        }
        if (led_drv != 0) {
            led_drv->set_state(0U);
        }
    }
}

/* 自动模式判定：任一指标超过阈值时启动排风并报警。 */
static void app_auto_assess(void)
{
    unsigned char fan;
    unsigned char alarm;

    fan = 0U;
    alarm = 0U;

    #if defined(PLATFORM_MCS51)
    if (g_ctx.temperature > (int)g_ctx.temp_threshold) {
    #else
    if (g_ctx.temperature > (float)g_ctx.temp_threshold) {
    #endif
        fan = 1U;
        alarm = 1U;
    }
    if (g_ctx.air_quality_ppm > g_ctx.aq_threshold_ppm) {
        fan = 1U;
        alarm = 1U;
    }
    if (g_ctx.co_ppm > g_ctx.co_threshold_ppm) {
        fan = 1U;
        alarm = 1U;
    }

    if (fan != g_ctx.fan_on || alarm != g_ctx.alarm_on) {
        g_ctx.fan_on = fan;
        g_ctx.alarm_on = alarm;
        app_apply_output();
        g_ctx.display_dirty = 1U;
    }
}

/* 将内部模式枚举转换为 OLED/遥测使用的短文本。 */
static const char *app_mode_text(void)
{
    switch (g_ctx.mode) {
    case APP_MODE_AUTO:
        return "AUTO";
    case APP_MODE_MANUAL:
        return "MAN";
    case APP_MODE_THRESHOLD:
        return "THR";
    default:
        return "???";
    }
}

/* 将当前阈值项转换为 OLED 显示名称。 */
static const char *app_threshold_name(void)
{
    switch (g_ctx.selected_threshold) {
    case APP_THRESHOLD_TEMP:
        return "Temp";
    case APP_THRESHOLD_AQ:
        return "AQ";
    case APP_THRESHOLD_CO:
        return "CO";
    default:
        return "Temp";
    }
}

#if defined(PLATFORM_MCS51)
static char *app_copy_text(char *dst, const char *src)
{
    while (*src != '\0') {
        *dst = *src;
        ++dst;
        ++src;
    }
    *dst = '\0';
    return dst;
}

static void app_u16_to_text(unsigned int value, char *buf)
{
    char tmp[6];
    unsigned char i = 0U;

    if (value == 0U) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while ((value > 0U) && (i < sizeof(tmp))) {
        tmp[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (i > 0U) {
        --i;
        *buf++ = tmp[i];
    }
    *buf = '\0';
}

static void app_line_label_text(char *line, const char *label, const char *text)
{
    char *out = app_copy_text(line, label);
    (void)app_copy_text(out, text);
}

static void app_line_label_u16(char *line, const char *label, unsigned int value, const char *suffix)
{
    char *out = app_copy_text(line, label);
    app_u16_to_text(value, out);
    while (*out != '\0') {
        ++out;
    }
    (void)app_copy_text(out, suffix);
}
#endif

/* 按当前模式刷新 OLED：自动/手动显示实时值，阈值模式显示可调项。 */
static void app_display_refresh(void)
{
#if defined(PLATFORM_MCS51)
    char line[24];
#endif

    if ((display == 0) || (g_ctx.display_dirty == 0U)) {
        return;
    }

    display->clear();

#if defined(PLATFORM_MCS51)
    app_line_label_text(line, "Mode:", app_mode_text());
    display->print(0U, 0U, DISPLAY_FONT_SMALL, line);

    if (g_ctx.mode == APP_MODE_THRESHOLD) {
        app_line_label_text(line, "Set ", app_threshold_name());
        display->print(0U, 2U, DISPLAY_FONT_SMALL, line);
        if (g_ctx.selected_threshold == APP_THRESHOLD_TEMP) {
            app_line_label_u16(line, "", (unsigned int)g_ctx.temp_threshold, "C");
            display->print(0U, 4U, DISPLAY_FONT_LARGE, line);
        } else if (g_ctx.selected_threshold == APP_THRESHOLD_AQ) {
            app_line_label_u16(line, "", (unsigned int)g_ctx.aq_threshold_ppm, "ppm");
            display->print(0U, 4U, DISPLAY_FONT_LARGE, line);
        } else {
            app_line_label_u16(line, "", (unsigned int)g_ctx.co_threshold_ppm, "ppm");
            display->print(0U, 4U, DISPLAY_FONT_LARGE, line);
        }
        display->print(0U, 7U, DISPLAY_FONT_SMALL, "K2 Sel K3+ K4-");
    } else {
        app_line_label_u16(line, "Temp:", (unsigned int)g_ctx.temperature, "C");
        display->print(0U, 2U, DISPLAY_FONT_SMALL, line);
        app_line_label_u16(line, "AQ:", (unsigned int)g_ctx.air_quality_ppm, "ppm");
        display->print(0U, 3U, DISPLAY_FONT_SMALL, line);
        app_line_label_u16(line, "CO:", (unsigned int)g_ctx.co_ppm, "ppm");
        display->print(0U, 4U, DISPLAY_FONT_SMALL, line);

        if (g_ctx.mode == APP_MODE_MANUAL) {
            app_line_label_text(line, "Dev:",
                                (g_ctx.selected_device == APP_DEVICE_FAN) ? "FAN" : "ALARM");
            display->print(0U, 5U, DISPLAY_FONT_SMALL, line);
            display->print(0U, 6U, DISPLAY_FONT_SMALL, "K2 Sel K3 Toggle");
        }

        display->print(0U, 7U, DISPLAY_FONT_SMALL,
                       (g_ctx.alarm_on != 0U) ? "ALARM!" : "SAFE");
    }
#else
    display->print(0U, 0U, DISPLAY_FONT_SMALL, "Mode:%s", app_mode_text());

    if (g_ctx.mode == APP_MODE_THRESHOLD) {
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "Set %s", app_threshold_name());
        if (g_ctx.selected_threshold == APP_THRESHOLD_TEMP) {
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%dC",
                           (int)g_ctx.temp_threshold);
        } else if (g_ctx.selected_threshold == APP_THRESHOLD_AQ) {
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%uppm",
                           (unsigned int)g_ctx.aq_threshold_ppm);
        } else {
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%uppm",
                           (unsigned int)g_ctx.co_threshold_ppm);
        }
        display->print(0U, 7U, DISPLAY_FONT_SMALL, "K2 Sel K3+ K4-");
    } else {
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "Temp:%dC", (int)g_ctx.temperature);
        display->print(0U, 3U, DISPLAY_FONT_SMALL, "AQ:%uppm", (unsigned int)g_ctx.air_quality_ppm);
        display->print(0U, 4U, DISPLAY_FONT_SMALL, "CO:%uppm", (unsigned int)g_ctx.co_ppm);

        if (g_ctx.mode == APP_MODE_MANUAL) {
            display->print(0U, 5U, DISPLAY_FONT_SMALL, "Dev:%s",
                           (g_ctx.selected_device == APP_DEVICE_FAN) ? "FAN" : "ALARM");
            display->print(0U, 6U, DISPLAY_FONT_SMALL, "K2 Sel K3 Toggle");
        }

        display->print(0U, 7U, DISPLAY_FONT_SMALL,
                       (g_ctx.alarm_on != 0U) ? "ALARM!" : "SAFE");
    }
#endif

    display->update();
    g_ctx.display_dirty = 0U;
}

#if VERSION_FEATURE_REMOTE
/* 发布遥测 JSON：WiFi 版本走 MQTT，蓝牙版本走串口透传。 */
void app_publish_telemetry(void)
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
        cJSON_AddNumberToObject(data, "temperature", g_ctx.temperature);
        cJSON_AddNumberToObject(data, "air_quality_ppm", g_ctx.air_quality_ppm);
        cJSON_AddNumberToObject(data, "co_ppm", g_ctx.co_ppm);
        cJSON_AddNumberToObject(data, "alarm", g_ctx.alarm_on);
        cJSON_AddNumberToObject(data, "fan", g_ctx.fan_on);
        cJSON_AddStringToObject(data, "mode", app_mode_text());

        thresholds = cJSON_CreateObject();
        if (thresholds != 0) {
            cJSON_AddNumberToObject(thresholds, "temp", g_ctx.temp_threshold);
            cJSON_AddNumberToObject(thresholds, "air_quality_ppm", g_ctx.aq_threshold_ppm);
            cJSON_AddNumberToObject(thresholds, "co_ppm", g_ctx.co_threshold_ppm);
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
        (void)comm_port_send((const unsigned char *)json_text,
                             (unsigned short)strlen(json_text));
#endif
        cjson_release_string(json_text);
    }

    cjson_release(root);
}

/* 兼容两种下行格式：优先读取 params，没有 params 时使用根对象。 */
static const cJSON *app_json_params(const cJSON *root)
{
    const cJSON *params;

    params = cJSON_GetObjectItem(root, "params");
    if ((params != 0) && ((params->type & cJSON_Object) != 0)) {
        return params;
    }

    return root;
}

/* 应用远程阈值设置，并限制在本机按键设置允许的范围内。 */
static void app_apply_threshold_json(const cJSON *params)
{
    const cJSON *item;

    item = cJSON_GetObjectItem(params, "temp");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        int value = item->valueint;
        if (value < (int)APP_MIN_TEMP_THRESHOLD) {
            value = (int)APP_MIN_TEMP_THRESHOLD;
        }
        if (value > (int)APP_MAX_TEMP_THRESHOLD) {
            value = (int)APP_MAX_TEMP_THRESHOLD;
        }
        g_ctx.temp_threshold = (unsigned char)value;
    }

    item = cJSON_GetObjectItem(params, "air_quality_ppm");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        int value = item->valueint;
        if (value < (int)APP_MIN_AQ_THRESHOLD_PPM) {
            value = (int)APP_MIN_AQ_THRESHOLD_PPM;
        }
        if (value > (int)APP_MAX_AQ_THRESHOLD_PPM) {
            value = (int)APP_MAX_AQ_THRESHOLD_PPM;
        }
        g_ctx.aq_threshold_ppm = (unsigned short)value;
    }

    item = cJSON_GetObjectItem(params, "co_ppm");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        int value = item->valueint;
        if (value < (int)APP_MIN_CO_THRESHOLD_PPM) {
            value = (int)APP_MIN_CO_THRESHOLD_PPM;
        }
        if (value > (int)APP_MAX_CO_THRESHOLD_PPM) {
            value = (int)APP_MAX_CO_THRESHOLD_PPM;
        }
        g_ctx.co_threshold_ppm = (unsigned short)value;
    }

    g_ctx.display_dirty = 1U;
}

/* 手动控制命令直接修改风扇和报警输出。 */
static void app_apply_control_json(const cJSON *params)
{
    const cJSON *item;

    item = cJSON_GetObjectItem(params, "fan");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_ctx.fan_on = (item->valueint != 0) ? 1U : 0U;
    }

    item = cJSON_GetObjectItem(params, "alarm");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_ctx.alarm_on = (item->valueint != 0) ? 1U : 0U;
    }

    app_apply_output();
    g_ctx.display_dirty = 1U;
}

/* 标记需要发送遥测，实际发送由 comm_loop_run 做节流。 */
void app_logic_request_telemetry(void)
{
    g_ctx.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
}
#endif

/* 按键入口：Key1 切模式，Key2 选择项，Key3/Key4 执行加减或切换。 */
void app_logic_on_key(unsigned char key_id)
{
    if (key_id == 1U) {
        g_ctx.mode = (app_mode_t)(((unsigned char)g_ctx.mode + 1U) % 3U);
        if (g_ctx.mode == APP_MODE_AUTO) {
            g_ctx.fan_on = 0U;
            g_ctx.alarm_on = 0U;
            app_apply_output();
            app_auto_assess();
        }
        g_ctx.display_dirty = 1U;
        return;
    }

    if (g_ctx.mode == APP_MODE_MANUAL) {
        if (key_id == 2U) {
            g_ctx.selected_device = (g_ctx.selected_device + 1U) % 2U;
            g_ctx.display_dirty = 1U;
            return;
        }
        if (key_id == 3U) {
            if (g_ctx.selected_device == APP_DEVICE_FAN) {
                g_ctx.fan_on = (g_ctx.fan_on == 0U) ? 1U : 0U;
            } else {
                g_ctx.alarm_on = (g_ctx.alarm_on == 0U) ? 1U : 0U;
            }
            app_apply_output();
            g_ctx.display_dirty = 1U;
        }
        return;
    }

    if (g_ctx.mode == APP_MODE_THRESHOLD) {
        if (key_id == 2U) {
            g_ctx.selected_threshold = (app_threshold_t)(((unsigned char)g_ctx.selected_threshold + 1U) % 3U);
            g_ctx.display_dirty = 1U;
            return;
        }
        if (key_id == 3U) {
            if (g_ctx.selected_threshold == APP_THRESHOLD_TEMP) {
                unsigned int value = (unsigned int)g_ctx.temp_threshold + APP_TEMP_STEP;
                if (value > APP_MAX_TEMP_THRESHOLD) {
                    value = APP_MAX_TEMP_THRESHOLD;
                }
                g_ctx.temp_threshold = (unsigned char)value;
            } else if (g_ctx.selected_threshold == APP_THRESHOLD_AQ) {
                unsigned int value = (unsigned int)g_ctx.aq_threshold_ppm + APP_AQ_STEP_PPM;
                if (value > APP_MAX_AQ_THRESHOLD_PPM) {
                    value = APP_MAX_AQ_THRESHOLD_PPM;
                }
                g_ctx.aq_threshold_ppm = (unsigned short)value;
            } else {
                unsigned int value = (unsigned int)g_ctx.co_threshold_ppm + APP_CO_STEP_PPM;
                if (value > APP_MAX_CO_THRESHOLD_PPM) {
                    value = APP_MAX_CO_THRESHOLD_PPM;
                }
                g_ctx.co_threshold_ppm = (unsigned short)value;
            }
            g_ctx.display_dirty = 1U;
#if VERSION_FEATURE_REMOTE
            app_logic_request_telemetry();
#endif
            return;
        }
        if (key_id == 4U) {
            if (g_ctx.selected_threshold == APP_THRESHOLD_TEMP) {
                if (g_ctx.temp_threshold > APP_MIN_TEMP_THRESHOLD) {
                    g_ctx.temp_threshold = (unsigned char)(g_ctx.temp_threshold - APP_TEMP_STEP);
                }
            } else if (g_ctx.selected_threshold == APP_THRESHOLD_AQ) {
                if (g_ctx.aq_threshold_ppm > APP_MIN_AQ_THRESHOLD_PPM) {
                    g_ctx.aq_threshold_ppm = (unsigned short)(g_ctx.aq_threshold_ppm - APP_AQ_STEP_PPM);
                }
            } else {
                if (g_ctx.co_threshold_ppm > APP_MIN_CO_THRESHOLD_PPM) {
                    g_ctx.co_threshold_ppm = (unsigned short)(g_ctx.co_threshold_ppm - APP_CO_STEP_PPM);
                }
            }
            g_ctx.display_dirty = 1U;
#if VERSION_FEATURE_REMOTE
            app_logic_request_telemetry();
#endif
            return;
        }
    }
}

/* 传感器循环：读取三路传感器，并在自动模式下重新判定输出。 */
void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (temp_sensor != 0) {
        g_ctx.temperature = temp_sensor->read_temperature();
    }
    if (aq_sensor != 0) {
        g_ctx.air_quality_ppm = aq_sensor->read_ppm();
    }
    if (co_sensor != 0) {
        g_ctx.co_ppm = co_sensor->read_ppm();
    }

    if (g_ctx.mode == APP_MODE_AUTO) {
        app_auto_assess();
    }

    g_ctx.display_dirty = 1U;
}

/* 业务循环：根据事件刷新显示，避免在每个事件源里直接操作 OLED。 */
void logic_loop_run(sched_event_t events, void *ctx)
{
    (void)ctx;

    if ((events & APP_EVENT_KEY) != 0U) {
        g_ctx.display_dirty = 1U;
    }
    if ((events & APP_EVENT_SENSOR) != 0U) {
        g_ctx.display_dirty = 1U;
    }
#if VERSION_FEATURE_REMOTE
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_COMM_TX)) != 0U) {
        g_ctx.display_dirty = 1U;
    }
#endif

    if (g_ctx.display_dirty != 0U) {
        app_display_refresh();
        g_ctx.display_dirty = 0U;
    }
}

/* 初始化业务驱动句柄和默认阈值。 */
void app_logic_init(void)
{
    aq_sensor = devmgr_get_gas_sensor("mq135");
    co_sensor = devmgr_get_gas_sensor("mq7_co");
    temp_sensor = devmgr_get_temp_hum_sensor("ds18b20");
    fan_relay = devmgr_get_relay("relay");
    buzzer_drv = devmgr_get_misc("buzzer");
    led_drv = devmgr_get_misc("led");
    display = devmgr_get_display("oled");

#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif

    g_ctx.mode = APP_MODE_AUTO;
    g_ctx.selected_device = APP_DEVICE_FAN;
    g_ctx.selected_threshold = APP_THRESHOLD_TEMP;
    g_ctx.fan_on = 0U;
    g_ctx.alarm_on = 0U;
    #if defined(PLATFORM_MCS51)
    g_ctx.temperature = 0;
    #else
    g_ctx.temperature = 0.0f;
    #endif
    g_ctx.air_quality_ppm = 0U;
    g_ctx.co_ppm = 0U;
    g_ctx.temp_threshold = APP_DEFAULT_TEMP_THRESHOLD;
    g_ctx.aq_threshold_ppm = APP_DEFAULT_AQ_THRESHOLD_PPM;
    g_ctx.co_threshold_ppm = APP_DEFAULT_CO_THRESHOLD_PPM;
    g_ctx.display_dirty = 1U;
#if VERSION_FEATURE_REMOTE
    g_ctx.telemetry_pending = 1U;
    g_ctx.last_telemetry_tick = 0U;
#endif

    app_apply_output();
    app_display_refresh();
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
    if (g_ctx.telemetry_pending != 0U) {
        if ((now - g_ctx.last_telemetry_tick) >= APP_TELEMETRY_INTERVAL_MS) {
            g_ctx.telemetry_pending = 0U;
            g_ctx.last_telemetry_tick = now;
            app_publish_telemetry();
        }
    }
}

/* 远程命令入口：解析 set_threshold/set_mode/set_control/get_status。 */
void app_logic_on_mqtt_rx(const char *json_data)
{
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
    params = app_json_params(root);

    if (strcmp(cmd, "set_threshold") == 0) {
        app_apply_threshold_json(params);
    } else if (strcmp(cmd, "set_mode") == 0) {
        mode_item = cJSON_GetObjectItem(params, "mode");
        if ((mode_item != 0) && cJSON_IsString(mode_item)) {
            if (strcmp(mode_item->valuestring, "auto") == 0) {
                g_ctx.mode = APP_MODE_AUTO;
                g_ctx.fan_on = 0U;
                g_ctx.alarm_on = 0U;
                app_apply_output();
                app_auto_assess();
            } else if (strcmp(mode_item->valuestring, "manual") == 0) {
                g_ctx.mode = APP_MODE_MANUAL;
            } else if (strcmp(mode_item->valuestring, "threshold") == 0) {
                g_ctx.mode = APP_MODE_THRESHOLD;
            }
            g_ctx.display_dirty = 1U;
        }
    } else if (strcmp(cmd, "set_control") == 0) {
        app_apply_control_json(params);
    } else if (strcmp(cmd, "get_status") == 0) {
        g_ctx.telemetry_pending = 1U;
    }

    app_logic_request_telemetry();
    cjson_release(root);
}
#endif
