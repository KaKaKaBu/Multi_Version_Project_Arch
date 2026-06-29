#include "app_logic.h"
#include "devmgr.h"
#include "imu_if.h"
#include "health_if.h"
#include "sensor_if.h"
#include "misc_if.h"
#include "display_if.h"
#include "board_config.h"
#include "tiny_printf.h"
#include <math.h>

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

#define SMZL_ALARM_BLINK_MS 300U
#define SMZL_TURNOVER_THRESHOLD_DEG 30.0f
#define SMZL_TURNOVER_COOLDOWN_MS 2000U

#define SMZL_DEFAULT_HR_UPPER 100U
#define SMZL_DEFAULT_HR_LOWER 50U
#define SMZL_DEFAULT_SPO2_UPPER 100U
#define SMZL_DEFAULT_SPO2_LOWER 90U
#define SMZL_DEFAULT_TEMP_UPPER 38.0f
#define SMZL_DEFAULT_TEMP_LOWER 35.0f

#define SMZL_HR_STEP 5U
#define SMZL_SPO2_STEP 1U
#define SMZL_TEMP_STEP 1U

smzl_context_t g_smzl;

static const imu_sensor_t *imu_sensor;
static const pulse_oximeter_t *pulse_ox;
static const temp_hum_sensor_t *temp_sensor;
static const display_driver_t *display;
static const misc_driver_t *buzzer_drv;
static const misc_driver_t *led_drv;

/* 将三轴加速度样本换算为倾斜角度，用于翻身变化检测。 */
static float smzl_tilt_from_sample(const imu_sample_t *sample)
{
    float ax, ay, az, horizontal;

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

    return atan2f(horizontal, fabsf(az)) * 57.2958f;
}

/* 基于倾角变化统计翻身次数，并用冷却时间避免一次动作重复计数。 */
static void smzl_detect_turnover(float current_tilt)
{
    unsigned long now;
    float diff;

    now = sched_tick_get();

    if (g_smzl.turnover_cooldown_tick != 0U &&
        (now - g_smzl.turnover_cooldown_tick) < SMZL_TURNOVER_COOLDOWN_MS) {
        g_smzl.last_tilt_degree = current_tilt;
        return;
    }

    diff = fabsf(current_tilt - g_smzl.stable_tilt_degree);

    if (diff >= SMZL_TURNOVER_THRESHOLD_DEG) {
        g_smzl.turnover_count++;
        g_smzl.stable_tilt_degree = current_tilt;
        g_smzl.turnover_cooldown_tick = now;
        g_smzl.display_dirty = 1U;
    }

    g_smzl.last_tilt_degree = current_tilt;
}

/* 报警输出统一入口，避免多处直接操作蜂鸣器和 LED。 */
static void smzl_set_alarm_output(unsigned char on)
{
    g_smzl.alarm_output_on = (on != 0U) ? 1U : 0U;

    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_smzl.alarm_output_on);
    }
    if (led_drv != 0) {
        led_drv->set_state(g_smzl.alarm_output_on);
    }
}

/* 将当前阈值项转换为 OLED 显示名称。 */
static const char *smzl_threshold_name(void)
{
    switch (g_smzl.selected_threshold) {
    case SMZL_THR_HR_UPPER:  return "HR Hi";
    case SMZL_THR_HR_LOWER:  return "HR Lo";
    case SMZL_THR_SPO2_UPPER: return "SpO2 Hi";
    case SMZL_THR_SPO2_LOWER: return "SpO2 Lo";
    case SMZL_THR_TEMP_UPPER: return "Tmp Hi";
    case SMZL_THR_TEMP_LOWER: return "Tmp Lo";
    default:                 return "HR Hi";
    }
}

/* 报警判定：心率、血氧或体温越界即报警。 */
static unsigned char smzl_check_alarm(void)
{
    if (g_smzl.heart_rate > g_smzl.hr_upper ||
        g_smzl.heart_rate < g_smzl.hr_lower) {
        return 1U;
    }
    if (g_smzl.spo2 < g_smzl.spo2_lower) {
        return 1U;
    }
    if (g_smzl.temperature > g_smzl.temp_upper ||
        g_smzl.temperature < g_smzl.temp_lower) {
        return 1U;
    }
    return 0U;
}

/* 按当前模式刷新 OLED：监测模式显示实时值，阈值模式显示可调项。 */
static void smzl_refresh_display(void)
{
    if ((display == 0) || (g_smzl.display_dirty == 0U)) {
        return;
    }

    display->clear();

    if (g_smzl.mode == SMZL_MODE_THRESHOLD) {
        display->print(0U, 0U, DISPLAY_FONT_SMALL, "Set Threshold");
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "%s", smzl_threshold_name());

        if (g_smzl.selected_threshold == SMZL_THR_HR_UPPER ||
            g_smzl.selected_threshold == SMZL_THR_HR_LOWER) {
            unsigned char val = (g_smzl.selected_threshold == SMZL_THR_HR_UPPER)
                                ? g_smzl.hr_upper : g_smzl.hr_lower;
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%ubpm", val);
        } else if (g_smzl.selected_threshold == SMZL_THR_SPO2_UPPER ||
                   g_smzl.selected_threshold == SMZL_THR_SPO2_LOWER) {
            unsigned char val = (g_smzl.selected_threshold == SMZL_THR_SPO2_UPPER)
                                ? g_smzl.spo2_upper : g_smzl.spo2_lower;
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%u%%", val);
        } else {
            float val = (g_smzl.selected_threshold == SMZL_THR_TEMP_UPPER)
                        ? g_smzl.temp_upper : g_smzl.temp_lower;
            display->print(0U, 4U, DISPLAY_FONT_LARGE, "%.1fC", val);
        }
        display->print(0U, 7U, DISPLAY_FONT_SMALL, "K2 Sel K3+ K4-");
    } else {
        display->print(0U, 0U, DISPLAY_FONT_SMALL, "Turn:%u",
                       (unsigned int)g_smzl.turnover_count);
        display->print(0U, 2U, DISPLAY_FONT_SMALL, "HR:%ubpm",
                       (unsigned int)g_smzl.heart_rate);
        display->print(0U, 3U, DISPLAY_FONT_SMALL, "SpO2:%u%%",
                       (unsigned int)g_smzl.spo2);
        display->print(0U, 4U, DISPLAY_FONT_SMALL, "Temp:%.1fC",
                       g_smzl.temperature);
        display->print(0U, 6U, DISPLAY_FONT_SMALL, "K1 Set K2 Clear");
        display->print(0U, 7U, DISPLAY_FONT_SMALL,
                       g_smzl.alarm_active ? "ALARM!" : "SAFE");
    }

    display->update();
    g_smzl.display_dirty = 0U;
}

#if VERSION_FEATURE_REMOTE
/* 发布遥测 JSON：WiFi 版本走 MQTT，蓝牙版本走串口透传。 */
void smzl_publish_telemetry(void)
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
        cJSON_AddNumberToObject(data, "heart_rate", g_smzl.heart_rate);
        cJSON_AddNumberToObject(data, "spo2", g_smzl.spo2);
        cJSON_AddNumberToObject(data, "temperature", g_smzl.temperature);
        cJSON_AddNumberToObject(data, "turnover_count", g_smzl.turnover_count);
        cJSON_AddNumberToObject(data, "alarm", g_smzl.alarm_active);

        thresholds = cJSON_CreateObject();
        if (thresholds != 0) {
            cJSON_AddNumberToObject(thresholds, "hr_upper", g_smzl.hr_upper);
            cJSON_AddNumberToObject(thresholds, "hr_lower", g_smzl.hr_lower);
            cJSON_AddNumberToObject(thresholds, "spo2_upper", g_smzl.spo2_upper);
            cJSON_AddNumberToObject(thresholds, "spo2_lower", g_smzl.spo2_lower);
            cJSON_AddNumberToObject(thresholds, "temp_upper", g_smzl.temp_upper);
            cJSON_AddNumberToObject(thresholds, "temp_lower", g_smzl.temp_lower);
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
static const cJSON *smzl_json_params(const cJSON *root)
{
    const cJSON *params;

    params = cJSON_GetObjectItem(root, "params");
    if ((params != 0) && ((params->type & cJSON_Object) != 0)) {
        return params;
    }

    return root;
}

/* 应用远程阈值设置。 */
static void smzl_apply_threshold_json(const cJSON *params)
{
    const cJSON *item;

    item = cJSON_GetObjectItem(params, "hr_upper");
    if ((item != 0) && ((item->type & cJSON_Number) != 0) && item->valueint > 0) {
        g_smzl.hr_upper = (unsigned char)item->valueint;
    }
    item = cJSON_GetObjectItem(params, "hr_lower");
    if ((item != 0) && ((item->type & cJSON_Number) != 0) && item->valueint > 0) {
        g_smzl.hr_lower = (unsigned char)item->valueint;
    }
    item = cJSON_GetObjectItem(params, "spo2_upper");
    if ((item != 0) && ((item->type & cJSON_Number) != 0) && item->valueint > 0) {
        g_smzl.spo2_upper = (unsigned char)item->valueint;
    }
    item = cJSON_GetObjectItem(params, "spo2_lower");
    if ((item != 0) && ((item->type & cJSON_Number) != 0) && item->valueint > 0) {
        g_smzl.spo2_lower = (unsigned char)item->valueint;
    }
    item = cJSON_GetObjectItem(params, "temp_upper");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_smzl.temp_upper = (float)item->valuedouble;
    }
    item = cJSON_GetObjectItem(params, "temp_lower");
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        g_smzl.temp_lower = (float)item->valuedouble;
    }

    g_smzl.display_dirty = 1U;
}

/* 标记需要发送遥测，实际发送由 comm_loop_run 做节流。 */
void smzl_request_telemetry(void)
{
    g_smzl.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
}

/* 远程命令入口：解析 set_threshold/clear_turnover/get_status。 */
void smzl_on_remote_rx(const char *json_data)
{
    cJSON *root;
    cJSON *cmd_item;
    const cJSON *params;
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
    params = smzl_json_params(root);

    if (strcmp(cmd, "set_threshold") == 0) {
        smzl_apply_threshold_json(params);
    } else if (strcmp(cmd, "clear_turnover") == 0) {
        g_smzl.turnover_count = 0U;
        g_smzl.display_dirty = 1U;
    } else if (strcmp(cmd, "get_status") == 0) {
        g_smzl.telemetry_pending = 1U;
    }

    smzl_request_telemetry();
    cjson_release(root);
}
#endif

/* 按键入口：Key1 切模式，Key2 清零/选择，Key3/Key4 调整阈值。 */
void smzl_logic_on_key(unsigned char key_id)
{
    if (key_id == 1U) {
        g_smzl.mode = (g_smzl.mode == SMZL_MODE_MONITOR)
                      ? SMZL_MODE_THRESHOLD : SMZL_MODE_MONITOR;
        g_smzl.display_dirty = 1U;
        return;
    }

    if (g_smzl.mode == SMZL_MODE_MONITOR) {
        if (key_id == 2U) {
            g_smzl.turnover_count = 0U;
            g_smzl.display_dirty = 1U;
        }
        return;
    }

    if (g_smzl.mode == SMZL_MODE_THRESHOLD) {
        if (key_id == 2U) {
            g_smzl.selected_threshold = (smzl_threshold_item_t)(
                ((unsigned char)g_smzl.selected_threshold + 1U) % 6U);
            g_smzl.display_dirty = 1U;
            return;
        }

        if (key_id == 3U || key_id == 4U) {
            int direction = (key_id == 3U) ? 1 : -1;

            switch (g_smzl.selected_threshold) {
            case SMZL_THR_HR_UPPER:
                g_smzl.hr_upper = (unsigned char)((int)g_smzl.hr_upper +
                                    direction * (int)SMZL_HR_STEP);
                if (g_smzl.hr_upper < 60U) g_smzl.hr_upper = 60U;
                if (g_smzl.hr_upper > 200U) g_smzl.hr_upper = 200U;
                break;
            case SMZL_THR_HR_LOWER:
                g_smzl.hr_lower = (unsigned char)((int)g_smzl.hr_lower +
                                    direction * (int)SMZL_HR_STEP);
                if (g_smzl.hr_lower < 30U) g_smzl.hr_lower = 30U;
                if (g_smzl.hr_lower > 150U) g_smzl.hr_lower = 150U;
                break;
            case SMZL_THR_SPO2_UPPER:
                g_smzl.spo2_upper = (unsigned char)((int)g_smzl.spo2_upper +
                                      direction * (int)SMZL_SPO2_STEP);
                if (g_smzl.spo2_upper < 90U) g_smzl.spo2_upper = 90U;
                if (g_smzl.spo2_upper > 100U) g_smzl.spo2_upper = 100U;
                break;
            case SMZL_THR_SPO2_LOWER:
                g_smzl.spo2_lower = (unsigned char)((int)g_smzl.spo2_lower +
                                      direction * (int)SMZL_SPO2_STEP);
                if (g_smzl.spo2_lower < 80U) g_smzl.spo2_lower = 80U;
                if (g_smzl.spo2_lower > 100U) g_smzl.spo2_lower = 100U;
                break;
            case SMZL_THR_TEMP_UPPER:
                g_smzl.temp_upper += (float)direction * SMZL_TEMP_STEP;
                if (g_smzl.temp_upper < 36.0f) g_smzl.temp_upper = 36.0f;
                if (g_smzl.temp_upper > 42.0f) g_smzl.temp_upper = 42.0f;
                break;
            case SMZL_THR_TEMP_LOWER:
                g_smzl.temp_lower += (float)direction * SMZL_TEMP_STEP;
                if (g_smzl.temp_lower < 34.0f) g_smzl.temp_lower = 34.0f;
                if (g_smzl.temp_lower > 40.0f) g_smzl.temp_lower = 40.0f;
                break;
            }

            g_smzl.display_dirty = 1U;
#if VERSION_FEATURE_REMOTE
            smzl_request_telemetry();
#endif
        }
    }
}

/* 传感器循环：采集姿态、心率血氧和体温，并更新报警状态。 */
void sensor_loop_run(sched_event_t events, void *ctx)
{
    imu_sample_t sample;
    float tilt;

    (void)events;
    (void)ctx;

    if (imu_sensor != 0) {
        imu_sensor->read_sample(&sample);
        tilt = smzl_tilt_from_sample(&sample);
        smzl_detect_turnover(tilt);
    }
    if (pulse_ox != 0) {
        g_smzl.heart_rate = pulse_ox->read_heart_rate();
        g_smzl.spo2 = pulse_ox->read_spo2();
    }
    if (temp_sensor != 0) {
        g_smzl.temperature = temp_sensor->read_temperature();
    }

    g_smzl.alarm_active = smzl_check_alarm();
    g_smzl.display_dirty = 1U;
}

/* 报警循环：报警激活时周期翻转蜂鸣器/LED。 */
void alarm_loop_run(sched_event_t events, void *ctx)
{
    unsigned long now;

    (void)events;
    (void)ctx;

    now = sched_tick_get();

    if (g_smzl.alarm_active == 0U) {
        if (g_smzl.alarm_output_on != 0U) {
            smzl_set_alarm_output(0U);
        }
        return;
    }

    if ((now - g_smzl.last_alarm_toggle_tick) >= SMZL_ALARM_BLINK_MS) {
        smzl_set_alarm_output((g_smzl.alarm_output_on == 0U) ? 1U : 0U);
        g_smzl.last_alarm_toggle_tick = now;
        g_smzl.display_dirty = 1U;
    }
}

/* 业务循环：根据事件刷新显示，避免在每个事件源里直接操作 OLED。 */
void logic_loop_run(sched_event_t events, void *ctx)
{
    (void)ctx;

    if ((events & APP_EVENT_KEY) != 0U) {
        g_smzl.display_dirty = 1U;
    }
    if ((events & APP_EVENT_SENSOR) != 0U) {
        g_smzl.display_dirty = 1U;
    }
#if VERSION_FEATURE_REMOTE
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_COMM_TX)) != 0U) {
        g_smzl.display_dirty = 1U;
    }
#endif

    if (g_smzl.display_dirty != 0U) {
        smzl_refresh_display();
        g_smzl.display_dirty = 0U;
    }
}

/* 初始化业务驱动句柄、默认阈值和翻身统计状态。 */
void smzl_logic_init(void)
{
    imu_sensor = devmgr_get_imu_sensor("mpu6050");
    pulse_ox = devmgr_get_pulse_oximeter("max30102");
    temp_sensor = devmgr_get_temp_hum_sensor("ds18b20");
    display = devmgr_get_display("oled");
    buzzer_drv = devmgr_get_misc("buzzer");
    led_drv = devmgr_get_misc("led");

#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif

    g_smzl.mode = SMZL_MODE_MONITOR;
    g_smzl.selected_threshold = SMZL_THR_HR_UPPER;
    g_smzl.heart_rate = 0U;
    g_smzl.spo2 = 0U;
    g_smzl.temperature = 0.0f;
    g_smzl.turnover_count = 0U;
    g_smzl.display_dirty = 1U;
    g_smzl.alarm_active = 0U;
    g_smzl.alarm_output_on = 0U;
    g_smzl.last_alarm_toggle_tick = sched_tick_get();
    g_smzl.last_tilt_degree = 0.0f;
    g_smzl.stable_tilt_degree = 0.0f;
    g_smzl.turnover_cooldown_tick = 0U;
    g_smzl.hr_upper = SMZL_DEFAULT_HR_UPPER;
    g_smzl.hr_lower = SMZL_DEFAULT_HR_LOWER;
    g_smzl.spo2_upper = SMZL_DEFAULT_SPO2_UPPER;
    g_smzl.spo2_lower = SMZL_DEFAULT_SPO2_LOWER;
    g_smzl.temp_upper = SMZL_DEFAULT_TEMP_UPPER;
    g_smzl.temp_lower = SMZL_DEFAULT_TEMP_LOWER;
#if VERSION_FEATURE_REMOTE
    g_smzl.telemetry_pending = 1U;
    g_smzl.last_telemetry_tick = 0U;
#endif

    smzl_set_alarm_output(0U);
    smzl_refresh_display();
}

#if VERSION_FEATURE_REMOTE
/* 通讯循环：WiFi 轮询 MQTT，并按固定间隔发送待发布遥测。 */
void comm_loop_run(sched_event_t events, void *ctx)
{
    unsigned long now;

    (void)ctx;

#if VERSION_FEATURE_WIFI
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_TICK)) != 0U) {
        esp8266_mqtt_poll();
    }
#endif

    now = sched_tick_get();
    if (g_smzl.telemetry_pending != 0U) {
        if ((now - g_smzl.last_telemetry_tick) >= APP_TELEMETRY_INTERVAL_MS) {
            g_smzl.telemetry_pending = 0U;
            g_smzl.last_telemetry_tick = now;
            smzl_publish_telemetry();
        }
    }
}
#endif
