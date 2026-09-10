#include "app_logic.h"
#include "devmgr.h"
#include "display_if.h"
#include "misc_if.h"
#include "actuator_if.h"
#include "analog_probe_if.h"
#include "weight_if.h"
#include "sensor_if.h"
#include "comm_if.h"
#include "comm_port.h"
#include "board_config.h"
#include "tiny_printf.h"

#if VERSION_FEATURE_REMOTE
#include "cJSON.h"
#include "cjson_port.h"
#include <string.h>
#endif

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
#include "esp8266_mqtt.h"
void app_comm_poll(sched_event_t events);
#endif

#define YYYH_TELEMETRY_BUFFER_SIZE 512U
#define YYYH_TELEMETRY_MIN_INTERVAL_MS 1000U
#define YYYH_HUAWEI_PROPERTY_INTERVAL_MS 30000U
#define YYYH_ALARM_TOGGLE_MS 300U
#define YYYH_VOICE_INTERVAL_MS 8000U
#define YYYH_SENSOR_SAMPLE_MS 3000U

yyyh_context_t g_yyyh;

static const rtc_driver_t *rtc_drv;
static const display_driver_t *display_drv;
static const misc_driver_t *led_drv;
static const misc_driver_t *buzzer_drv;
static const analog_probe_t *take_probe;
#if VERSION_FEATURE_SERVO
static const servo_driver_t *servo_drv;
#endif
#if VERSION_FEATURE_WEIGHT
static const weight_sensor_t *weight_drv;
#endif
#if VERSION_FEATURE_DHT11
static const temp_hum_sensor_t *dht11_drv;
#endif
#if VERSION_FEATURE_VOICE
static const comm_driver_t *tts_drv;
#endif
#if VERSION_FEATURE_CLOUD
static uint32_t app_last_huawei_property_tick;
#endif

#if VERSION_FEATURE_VOICE
static const unsigned char yyyh_voice_take_medicine_gb2312[] = {
    0xB3U, 0xD4U, 0xD2U, 0xA9U, 0xCAU, 0xB1U, 0xBCU, 0xE4U,
    0xB5U, 0xBDU, 0xC1U, 0xCBU, 0xA3U, 0xACU, 0xC7U, 0xEBU,
    0xC8U, 0xA1U, 0xD2U, 0xA9U
};
static const unsigned char yyyh_voice_low_medicine_gb2312[] = {
    0xD2U, 0xA9U, 0xCEU, 0xEFU, 0xD2U, 0xD1U, 0xB2U, 0xBBU,
    0xD7U, 0xE3U, 0xA3U, 0xACU, 0xC7U, 0xEBU, 0xD7U, 0xBCU,
    0xB1U, 0xB8U, 0xD2U, 0xA9U, 0xCEU, 0xEFU
};
#endif

static uint8_t yyyh_clamp_u8(int value, uint8_t min_value, uint8_t max_value)
{
    if (value < (int)min_value) {
        return min_value;
    }
    if (value > (int)max_value) {
        return max_value;
    }
    return (uint8_t)value;
}

static void yyyh_set_outputs(uint8_t on)
{
    g_yyyh.alarm_output_on = (on != 0U) ? 1U : 0U;
    if (led_drv != 0) {
        led_drv->set_state(g_yyyh.alarm_output_on);
    }
    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_yyyh.alarm_output_on);
    }
}

static void yyyh_open_box(uint8_t open)
{
    g_yyyh.box_open = (open != 0U) ? 1U : 0U;
#if VERSION_FEATURE_SERVO
    if (servo_drv != 0) {
        servo_drv->set_angle(g_yyyh.box_open ? BOARD_SERVO_OPEN_ANGLE : BOARD_SERVO_CLOSED_ANGLE);
    }
#endif
}

static void yyyh_stop_alarm(void)
{
    g_yyyh.alarm_active = 0U;
    yyyh_set_outputs(0U);
    yyyh_open_box(0U);
    g_yyyh.display_dirty = 1U;
    app_logic_request_telemetry();
}

static void yyyh_start_alarm(uint8_t timer_index)
{
    yyyh_timer_t *timer;

    if (timer_index >= YYYH_TIMER_COUNT) {
        return;
    }

    timer = &g_yyyh.timers[timer_index];
    timer->fired_day = g_yyyh.now.day;
    timer->fired_hour = g_yyyh.now.hour;
    timer->fired_minute = g_yyyh.now.minute;
    g_yyyh.selected_timer = timer_index;
    g_yyyh.alarm_active = 1U;
    g_yyyh.medicine_taken = 0U;
    g_yyyh.last_alarm_toggle_tick = sched_tick_get();
    yyyh_open_box(1U);
    yyyh_set_outputs(1U);
    g_yyyh.display_dirty = 1U;
    app_logic_request_telemetry();
}

static uint8_t yyyh_timer_already_fired(const yyyh_timer_t *timer)
{
    return ((timer->fired_day == g_yyyh.now.day) &&
            (timer->fired_hour == g_yyyh.now.hour) &&
            (timer->fired_minute == g_yyyh.now.minute)) ? 1U : 0U;
}

static void yyyh_check_timers(void)
{
    uint8_t i;

    if (g_yyyh.alarm_active != 0U) {
        return;
    }

    for (i = 0U; i < YYYH_TIMER_COUNT; ++i) {
        yyyh_timer_t *timer = &g_yyyh.timers[i];
        if ((timer->enabled != 0U) &&
            (timer->hour == g_yyyh.now.hour) &&
            (timer->minute == g_yyyh.now.minute) &&
            (yyyh_timer_already_fired(timer) == 0U)) {
            yyyh_start_alarm(i);
            return;
        }
    }
}

static void yyyh_default_state(void)
{
    uint8_t i;

    g_yyyh.mode = YYYH_MODE_NORMAL;
    g_yyyh.selected_timer = 0U;
    g_yyyh.selected_field = 0U;
    g_yyyh.selected_medicine = 0U;
    g_yyyh.alarm_active = 0U;
    g_yyyh.alarm_output_on = 0U;
    g_yyyh.medicine_taken = 0U;
    g_yyyh.box_open = 0U;
    g_yyyh.weight_g = 0.0f;
    g_yyyh.low_medicine = 0U;
    g_yyyh.temperature = 25.0f;
    g_yyyh.humidity = 50.0f;
    g_yyyh.display_dirty = 1U;
    g_yyyh.telemetry_pending = 1U;
    g_yyyh.last_alarm_toggle_tick = sched_tick_get();

    g_yyyh.now.year = 2026U;
    g_yyyh.now.month = 7U;
    g_yyyh.now.day = 8U;
    g_yyyh.now.hour = 8U;
    g_yyyh.now.minute = 0U;
    g_yyyh.now.second = 0U;
    g_yyyh.now.week = 3U;

    for (i = 0U; i < YYYH_TIMER_COUNT; ++i) {
        g_yyyh.timers[i].enabled = 1U;
        g_yyyh.timers[i].hour = (uint8_t)(8U + (i * 6U));
        g_yyyh.timers[i].minute = 0U;
        g_yyyh.timers[i].dose = 1U;
        g_yyyh.timers[i].category = i;
        g_yyyh.timers[i].fired_day = 0U;
        g_yyyh.timers[i].fired_hour = 0xFFU;
        g_yyyh.timers[i].fired_minute = 0xFFU;
        g_yyyh.medicine_counts[i] = 10U;
    }
}

void app_logic_init(void)
{
    yyyh_default_state();

    rtc_drv = devmgr_get_rtc("ds1302");
    display_drv = devmgr_get_display("oled");
    led_drv = devmgr_get_misc("led");
    buzzer_drv = devmgr_get_misc("buzzer");
    take_probe = devmgr_get_analog_probe("presence");
#if VERSION_FEATURE_SERVO
    servo_drv = devmgr_get_servo("sg90");
#endif
#if VERSION_FEATURE_WEIGHT
    weight_drv = devmgr_get_weight_sensor("hx711");
#endif
#if VERSION_FEATURE_DHT11
    dht11_drv = devmgr_get_temp_hum_sensor("dht11");
#endif
#if VERSION_FEATURE_VOICE
    tts_drv = devmgr_get_comm("tts_uart");
#endif
#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif
    if (rtc_drv != 0) {
        rtc_drv->read_time(&g_yyyh.now);
    }
    yyyh_open_box(0U);
    yyyh_set_outputs(0U);
}

static void yyyh_cycle_mode(void)
{
    if (g_yyyh.mode == YYYH_MODE_NORMAL) {
        g_yyyh.mode = YYYH_MODE_TIMER_SET;
        g_yyyh.selected_field = 0U;
    } else if (g_yyyh.mode == YYYH_MODE_TIMER_SET) {
        g_yyyh.mode = YYYH_MODE_MEDICINE_SET;
    } else if (g_yyyh.mode == YYYH_MODE_MEDICINE_SET) {
        g_yyyh.mode = YYYH_MODE_MANUAL;
    } else {
        g_yyyh.mode = YYYH_MODE_NORMAL;
    }
}

static void yyyh_adjust_timer(int delta)
{
    yyyh_timer_t *timer = &g_yyyh.timers[g_yyyh.selected_timer];

    if (g_yyyh.selected_field == 0U) {
        timer->enabled = (timer->enabled == 0U) ? 1U : 0U;
    } else if (g_yyyh.selected_field == 1U) {
        timer->hour = yyyh_clamp_u8((int)timer->hour + delta, 0U, 23U);
    } else if (g_yyyh.selected_field == 2U) {
        timer->minute = yyyh_clamp_u8((int)timer->minute + delta, 0U, 59U);
    } else if (g_yyyh.selected_field == 3U) {
        timer->dose = yyyh_clamp_u8((int)timer->dose + delta, 1U, 9U);
    } else {
        timer->category = yyyh_clamp_u8((int)timer->category + delta, 0U, YYYH_MEDICINE_SLOT_COUNT - 1U);
    }
}

void app_logic_handle_key(uint8_t key_index)
{
    if (key_index == 1U) {
        yyyh_cycle_mode();
    } else if (key_index == 2U) {
        if (g_yyyh.mode == YYYH_MODE_TIMER_SET) {
            g_yyyh.selected_field = (uint8_t)((g_yyyh.selected_field + 1U) % 5U);
            if (g_yyyh.selected_field == 0U) {
                g_yyyh.selected_timer = (uint8_t)((g_yyyh.selected_timer + 1U) % YYYH_TIMER_COUNT);
            }
        } else if (g_yyyh.mode == YYYH_MODE_MEDICINE_SET) {
            g_yyyh.selected_medicine = (uint8_t)((g_yyyh.selected_medicine + 1U) % YYYH_MEDICINE_SLOT_COUNT);
        } else if (g_yyyh.mode == YYYH_MODE_MANUAL) {
            yyyh_open_box(g_yyyh.box_open == 0U);
        }
    } else if (key_index == 3U) {
        if (g_yyyh.alarm_active != 0U) {
            yyyh_stop_alarm();
        } else if (g_yyyh.mode == YYYH_MODE_TIMER_SET) {
            yyyh_adjust_timer(1);
        } else if (g_yyyh.mode == YYYH_MODE_MEDICINE_SET) {
            if (g_yyyh.medicine_counts[g_yyyh.selected_medicine] < 99U) {
                ++g_yyyh.medicine_counts[g_yyyh.selected_medicine];
            }
        } else if (g_yyyh.mode == YYYH_MODE_MANUAL) {
            g_yyyh.alarm_active = (g_yyyh.alarm_active == 0U) ? 1U : 0U;
        }
    } else if (key_index == 4U) {
        if (g_yyyh.mode == YYYH_MODE_TIMER_SET) {
            yyyh_adjust_timer(-1);
        } else if (g_yyyh.mode == YYYH_MODE_MEDICINE_SET) {
            if (g_yyyh.medicine_counts[g_yyyh.selected_medicine] > 0U) {
                --g_yyyh.medicine_counts[g_yyyh.selected_medicine];
            }
        } else {
            yyyh_stop_alarm();
        }
    }

    g_yyyh.display_dirty = 1U;
    app_logic_request_telemetry();
}

void rtc_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (rtc_drv != 0) {
        rtc_drv->read_time(&g_yyyh.now);
    }
    yyyh_check_timers();
    g_yyyh.display_dirty = 1U;
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now = sched_tick_get();

    (void)events;
    (void)ctx;

    if ((now - g_yyyh.last_sensor_tick) < YYYH_SENSOR_SAMPLE_MS) {
        return;
    }
    g_yyyh.last_sensor_tick = now;

    if (take_probe != 0) {
        uint8_t taken = (take_probe->read_value() > 0.5f) ? 1U : 0U;
        if ((taken != 0U) && (g_yyyh.alarm_active != 0U)) {
            yyyh_timer_t *timer = &g_yyyh.timers[g_yyyh.selected_timer];
            uint8_t cat = timer->category;
            uint8_t dose = timer->dose;
            if (cat < YYYH_MEDICINE_SLOT_COUNT) {
                if (g_yyyh.medicine_counts[cat] > dose) {
                    g_yyyh.medicine_counts[cat] = (uint8_t)(g_yyyh.medicine_counts[cat] - dose);
                } else {
                    g_yyyh.medicine_counts[cat] = 0U;
                }
            }
            g_yyyh.medicine_taken = 1U;
            yyyh_stop_alarm();
        }
    }

#if VERSION_FEATURE_WEIGHT
    if (weight_drv != 0) {
        g_yyyh.weight_g = weight_drv->read_grams();
        g_yyyh.low_medicine = (g_yyyh.weight_g <= BOARD_LOW_MEDICINE_WEIGHT_G) ? 1U : 0U;
    }
#endif
#if VERSION_FEATURE_DHT11
    if (dht11_drv != 0) {
        g_yyyh.temperature = dht11_drv->read_temperature();
        g_yyyh.humidity = dht11_drv->read_humidity();
    }
#endif
    g_yyyh.display_dirty = 1U;
    app_logic_request_telemetry();
}

void alarm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now = sched_tick_get();

    (void)events;
    (void)ctx;

    if ((g_yyyh.alarm_active == 0U)
#if VERSION_FEATURE_WEIGHT
        && (g_yyyh.low_medicine == 0U)
#endif
       ) {
        if (g_yyyh.alarm_output_on != 0U) {
            yyyh_set_outputs(0U);
            g_yyyh.display_dirty = 1U;
        }
        return;
    }

    if ((now - g_yyyh.last_alarm_toggle_tick) >= YYYH_ALARM_TOGGLE_MS) {
        yyyh_set_outputs((g_yyyh.alarm_output_on == 0U) ? 1U : 0U);
        g_yyyh.last_alarm_toggle_tick = now;
        event_set(APP_EVENT_ALARM);
    }

#if VERSION_FEATURE_VOICE
    if ((tts_drv != 0) && ((now - g_yyyh.last_voice_tick) >= YYYH_VOICE_INTERVAL_MS)) {
        if (g_yyyh.alarm_active != 0U) {
            (void)tts_drv->send(yyyh_voice_take_medicine_gb2312,
                                (unsigned short)sizeof(yyyh_voice_take_medicine_gb2312));
        } else if (g_yyyh.low_medicine != 0U) {
            (void)tts_drv->send(yyyh_voice_low_medicine_gb2312,
                                (unsigned short)sizeof(yyyh_voice_low_medicine_gb2312));
        }
        g_yyyh.last_voice_tick = now;
    }
#endif
}

static void yyyh_print_normal_screen(void)
{
    const yyyh_timer_t *timer = &g_yyyh.timers[g_yyyh.selected_timer];

    display_drv->print(0U, 0U, DISPLAY_FONT_SMALL, "YYYH-%02u %02u:%02u:%02u",
                       (unsigned int)APP_VERSION,
                       (unsigned int)g_yyyh.now.hour,
                       (unsigned int)g_yyyh.now.minute,
                       (unsigned int)g_yyyh.now.second);
    display_drv->print(0U, 1U, DISPLAY_FONT_SMALL, "定%u %s %02u:%02u 量%u",
                       (unsigned int)(g_yyyh.selected_timer + 1U),
                       timer->enabled ? "开" : "关",
                       (unsigned int)timer->hour,
                       (unsigned int)timer->minute,
                       (unsigned int)timer->dose);
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "药1:%u 药2:%u 药3:%u",
                       (unsigned int)g_yyyh.medicine_counts[0],
                       (unsigned int)g_yyyh.medicine_counts[1],
                       (unsigned int)g_yyyh.medicine_counts[2]);
#if VERSION_FEATURE_WEIGHT
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "重:%dg %s",
                       (int)g_yyyh.weight_g,
                       g_yyyh.low_medicine ? "不足" : "正常");
#elif VERSION_FEATURE_DHT11
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "温:%dC 湿:%d%%",
                       (int)g_yyyh.temperature,
                       (int)g_yyyh.humidity);
#else
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "盒:%s 警:%s",
                       g_yyyh.box_open ? "开" : "关",
                       g_yyyh.alarm_active ? "开" : "关");
#endif
}

static void yyyh_print_set_screen(void)
{
    const yyyh_timer_t *timer = &g_yyyh.timers[g_yyyh.selected_timer];
    const char *field = "启用";

    if (g_yyyh.selected_field == 1U) {
        field = "小时";
    } else if (g_yyyh.selected_field == 2U) {
        field = "分钟";
    } else if (g_yyyh.selected_field == 3U) {
        field = "剂量";
    } else if (g_yyyh.selected_field == 4U) {
        field = "分类";
    }

    display_drv->print(0U, 0U, DISPLAY_FONT_SMALL, "定时设置");
    display_drv->print(0U, 1U, DISPLAY_FONT_SMALL, "定%u %s %02u:%02u",
                       (unsigned int)(g_yyyh.selected_timer + 1U),
                       timer->enabled ? "开" : "关",
                       (unsigned int)timer->hour,
                       (unsigned int)timer->minute);
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "剂量:%u 分类:%u",
                       (unsigned int)timer->dose,
                       (unsigned int)(timer->category + 1U));
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "编辑:%s", field);
}

static void yyyh_print_medicine_screen(void)
{
    display_drv->print(0U, 0U, DISPLAY_FONT_SMALL, "药量设置");
    display_drv->print(0U, 1U, DISPLAY_FONT_SMALL, "选择药%u",
                       (unsigned int)(g_yyyh.selected_medicine + 1U));
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "药1:%u 药2:%u 药3:%u",
                       (unsigned int)g_yyyh.medicine_counts[0],
                       (unsigned int)g_yyyh.medicine_counts[1],
                       (unsigned int)g_yyyh.medicine_counts[2]);
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "K3加 K4减");
}

static void yyyh_print_manual_screen(void)
{
    display_drv->print(0U, 0U, DISPLAY_FONT_SMALL, "手动模式");
    display_drv->print(0U, 1U, DISPLAY_FONT_SMALL, "K2药盒:%s",
                       g_yyyh.box_open ? "开" : "关");
    display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "K3报警:%s",
                       g_yyyh.alarm_active ? "开" : "关");
    display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "K4停止");
}

void display_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if ((display_drv == 0) || (g_yyyh.display_dirty == 0U)) {
        return;
    }

    g_yyyh.display_dirty = 0U;
    display_drv->clear();
    if (g_yyyh.alarm_active != 0U) {
        display_drv->print(0U, 0U, DISPLAY_FONT_SMALL, "请取药");
        display_drv->print(0U, 1U, DISPLAY_FONT_SMALL, "定时%u 剂量%u",
                           (unsigned int)(g_yyyh.selected_timer + 1U),
                           (unsigned int)g_yyyh.timers[g_yyyh.selected_timer].dose);
        display_drv->print(0U, 2U, DISPLAY_FONT_SMALL, "分类%u",
                           (unsigned int)(g_yyyh.timers[g_yyyh.selected_timer].category + 1U));
        display_drv->print(0U, 3U, DISPLAY_FONT_SMALL, "取药后停报警");
    } else if (g_yyyh.mode == YYYH_MODE_TIMER_SET) {
        yyyh_print_set_screen();
    } else if (g_yyyh.mode == YYYH_MODE_MEDICINE_SET) {
        yyyh_print_medicine_screen();
    } else if (g_yyyh.mode == YYYH_MODE_MANUAL) {
        yyyh_print_manual_screen();
    } else {
        yyyh_print_normal_screen();
    }
    display_drv->update();
}

#if VERSION_FEATURE_REMOTE
static const cJSON *yyyh_json_object_child(const cJSON *parent, const char *name)
{
    const cJSON *child;

    if ((parent == 0) || (name == 0)) {
        return 0;
    }

    child = cJSON_GetObjectItem(parent, name);
    if ((child != 0) && ((child->type & cJSON_Object) != 0)) {
        return child;
    }
    return 0;
}

static const cJSON *yyyh_json_services_properties(const cJSON *root)
{
    const cJSON *services;
    const cJSON *service;
    const cJSON *properties;

    if (root == 0) {
        return 0;
    }

    services = cJSON_GetObjectItem(root, "services");
    if ((services == 0) || ((services->type & cJSON_Array) == 0)) {
        return 0;
    }

    service = services->child;
    while (service != 0) {
        properties = yyyh_json_object_child(service, "properties");
        if (properties != 0) {
            return properties;
        }
        service = service->next;
    }
    return 0;
}

static const cJSON *yyyh_json_params(const cJSON *root)
{
    const cJSON *params = yyyh_json_object_child(root, "params");
    if (params != 0) {
        return params;
    }
    params = yyyh_json_object_child(root, "message");
    if (params != 0) {
        return params;
    }
    params = yyyh_json_object_child(root, "properties");
    if (params != 0) {
        return params;
    }
    params = yyyh_json_services_properties(root);
    if (params != 0) {
        return params;
    }
    return root;
}

static int yyyh_json_number(const cJSON *parent, const char *name, int *value)
{
    const cJSON *item;

    if ((parent == 0) || (name == 0) || (value == 0)) {
        return 0;
    }

    item = cJSON_GetObjectItem(parent, name);
    if ((item != 0) && ((item->type & cJSON_Number) != 0)) {
        *value = item->valueint;
        return 1;
    }
    if ((item != 0) && ((item->type & (cJSON_True | cJSON_False)) != 0)) {
        *value = ((item->type & cJSON_True) != 0) ? 1 : 0;
        return 1;
    }
    return 0;
}

static const char *yyyh_json_text(const cJSON *parent, const char *name)
{
    const cJSON *item;

    if ((parent == 0) || (name == 0)) {
        return 0;
    }

    item = cJSON_GetObjectItem(parent, name);
    if ((item != 0) && cJSON_IsString(item)) {
        return item->valuestring;
    }
    return 0;
}

static void yyyh_build_properties(char *buf, size_t bufsize)
{
    cJSON *root;
    char *json_str;

    if ((buf == 0) || (bufsize == 0U)) {
        return;
    }
    buf[0] = '\0';

    root = cJSON_CreateObject();
    if (root == 0) {
        return;
    }

    cJSON_AddNumberToObject(root, "version_no", APP_VERSION);
    cJSON_AddNumberToObject(root, "alarm", g_yyyh.alarm_active);
    cJSON_AddNumberToObject(root, "box_open", g_yyyh.box_open);
    cJSON_AddNumberToObject(root, "taken", g_yyyh.medicine_taken);
    cJSON_AddNumberToObject(root, "timer_index", g_yyyh.selected_timer + 1U);
    cJSON_AddNumberToObject(root, "timer_enabled", g_yyyh.timers[g_yyyh.selected_timer].enabled);
    cJSON_AddNumberToObject(root, "timer_hour", g_yyyh.timers[g_yyyh.selected_timer].hour);
    cJSON_AddNumberToObject(root, "timer_minute", g_yyyh.timers[g_yyyh.selected_timer].minute);
    cJSON_AddNumberToObject(root, "timer_dose", g_yyyh.timers[g_yyyh.selected_timer].dose);
    cJSON_AddNumberToObject(root, "medicine1", g_yyyh.medicine_counts[0]);
    cJSON_AddNumberToObject(root, "medicine2", g_yyyh.medicine_counts[1]);
    cJSON_AddNumberToObject(root, "medicine3", g_yyyh.medicine_counts[2]);
#if VERSION_FEATURE_WEIGHT
    cJSON_AddNumberToObject(root, "weight_g", g_yyyh.weight_g);
    cJSON_AddNumberToObject(root, "low_medicine", g_yyyh.low_medicine);
#endif
#if VERSION_FEATURE_DHT11
    cJSON_AddNumberToObject(root, "temperature", g_yyyh.temperature);
    cJSON_AddNumberToObject(root, "humidity", g_yyyh.humidity);
#endif

    json_str = cJSON_PrintUnformatted(root);
    if (json_str != 0) {
        strncpy(buf, json_str, bufsize - 1U);
        buf[bufsize - 1U] = '\0';
        cjson_release_string(json_str);
    }
    cjson_release(root);
}

static void yyyh_publish_telemetry(void)
{
    char telemetry[YYYH_TELEMETRY_BUFFER_SIZE];
#if VERSION_FEATURE_CLOUD
    char properties[YYYH_TELEMETRY_BUFFER_SIZE];
#endif

    yyyh_build_properties(telemetry, sizeof(telemetry));

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
    if (esp8266_mqtt_is_ready() != 0) {
#if VERSION_FEATURE_CLOUD
        uint32_t now = sched_tick_get();
        const esp8266_mqtt_config_t *mqtt_cfg;

        yyyh_build_properties(properties, sizeof(properties));
        if ((properties[0] != '\0') &&
            ((app_last_huawei_property_tick == 0U) ||
             ((uint32_t)(now - app_last_huawei_property_tick) >= YYYH_HUAWEI_PROPERTY_INTERVAL_MS))) {
            (void)esp8266_mqtt_publish_huawei_properties(properties);
            app_last_huawei_property_tick = now;
        }
        mqtt_cfg = esp8266_mqtt_active_config();
        if ((mqtt_cfg != 0) &&
            (mqtt_cfg->huawei_custom_pub_topic != 0) &&
            (mqtt_cfg->huawei_custom_pub_topic[0] != '\0')) {
            (void)esp8266_mqtt_publish_json(mqtt_cfg->huawei_custom_pub_topic, telemetry);
        }
#else
        (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, telemetry);
#endif
        return;
    }
#endif

#if VERSION_FEATURE_BLE
    (void)comm_port_send((const unsigned char *)telemetry, (unsigned short)strlen(telemetry));
    (void)comm_port_send((const unsigned char *)"\n", 1U);
#endif
}

void app_logic_request_telemetry(void)
{
    if (g_yyyh.telemetry_pending == 0U) {
        g_yyyh.telemetry_pending = 1U;
        event_set(APP_EVENT_COMM_TX);
    }
}

void app_logic_on_remote_rx(const char *json_data)
{
    cJSON *root;
    const cJSON *params;
    const char *cmd;
    int value;
    int index;

    if (json_data == 0) {
        return;
    }

    root = cjson_parse(json_data, strlen(json_data));
    if (root == 0) {
        return;
    }

    params = yyyh_json_params(root);
    cmd = yyyh_json_text(params, "cmd");

    if ((cmd != 0) && (strcmp(cmd, "get_status") == 0)) {
        app_logic_request_telemetry();
    } else if ((cmd != 0) && (strcmp(cmd, "open_box") == 0)) {
        yyyh_open_box(1U);
    } else if ((cmd != 0) && (strcmp(cmd, "close_box") == 0)) {
        yyyh_open_box(0U);
    } else if ((cmd != 0) && (strcmp(cmd, "ack_take") == 0)) {
        yyyh_stop_alarm();
    } else if ((cmd != 0) && (strcmp(cmd, "set_timer") == 0)) {
        index = 0;
        (void)yyyh_json_number(params, "index", &index);
        if (index > 0) {
            --index;
        }
        if ((index >= 0) && (index < (int)YYYH_TIMER_COUNT)) {
            yyyh_timer_t *timer = &g_yyyh.timers[index];
            if (yyyh_json_number(params, "enabled", &value) != 0) {
                timer->enabled = value ? 1U : 0U;
            }
            if (yyyh_json_number(params, "hour", &value) != 0) {
                timer->hour = yyyh_clamp_u8(value, 0U, 23U);
            }
            if (yyyh_json_number(params, "minute", &value) != 0) {
                timer->minute = yyyh_clamp_u8(value, 0U, 59U);
            }
            if (yyyh_json_number(params, "dose", &value) != 0) {
                timer->dose = yyyh_clamp_u8(value, 1U, 9U);
            }
            if (yyyh_json_number(params, "category", &value) != 0) {
                timer->category = yyyh_clamp_u8(value, 0U, YYYH_MEDICINE_SLOT_COUNT - 1U);
            }
        }
    } else if ((cmd != 0) && (strcmp(cmd, "set_count") == 0)) {
        index = 0;
        (void)yyyh_json_number(params, "index", &index);
        if (index > 0) {
            --index;
        }
        if (((index >= 0) && (index < (int)YYYH_MEDICINE_SLOT_COUNT)) &&
            (yyyh_json_number(params, "value", &value) != 0)) {
            g_yyyh.medicine_counts[index] = yyyh_clamp_u8(value, 0U, 99U);
        }
    } else if ((cmd != 0) && (strcmp(cmd, "set_time") == 0)) {
        rtc_time_t next = g_yyyh.now;
        if (yyyh_json_number(params, "year", &value) != 0) {
            next.year = (uint16_t)value;
        }
        if (yyyh_json_number(params, "month", &value) != 0) {
            next.month = yyyh_clamp_u8(value, 1U, 12U);
        }
        if (yyyh_json_number(params, "day", &value) != 0) {
            next.day = yyyh_clamp_u8(value, 1U, 31U);
        }
        if (yyyh_json_number(params, "hour", &value) != 0) {
            next.hour = yyyh_clamp_u8(value, 0U, 23U);
        }
        if (yyyh_json_number(params, "minute", &value) != 0) {
            next.minute = yyyh_clamp_u8(value, 0U, 59U);
        }
        if (yyyh_json_number(params, "second", &value) != 0) {
            next.second = yyyh_clamp_u8(value, 0U, 59U);
        }
        if (rtc_drv != 0) {
            rtc_drv->set_time(&next);
        }
        g_yyyh.now = next;
    }

    g_yyyh.display_dirty = 1U;
    app_logic_request_telemetry();
    cjson_release(root);
}

void comm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now;
#if VERSION_FEATURE_BLE
    unsigned char rx;
#endif

    (void)ctx;

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
    app_comm_poll(events);
#endif

#if VERSION_FEATURE_BLE
    if ((events & APP_EVENT_COMM_RX) != 0U) {
        while (comm_port_recv(&rx, 1U) > 0) {
        }
    }
#endif

    if (((events & (APP_EVENT_COMM_TX | APP_EVENT_RTC | APP_EVENT_SENSOR | APP_EVENT_TICK)) != 0U) &&
        (g_yyyh.telemetry_pending != 0U)) {
        now = sched_tick_get();
        if ((g_yyyh.last_telemetry_tick != 0U) &&
            ((uint32_t)(now - g_yyyh.last_telemetry_tick) < YYYH_TELEMETRY_MIN_INTERVAL_MS)) {
            return;
        }
        g_yyyh.telemetry_pending = 0U;
        g_yyyh.last_telemetry_tick = now;
        yyyh_publish_telemetry();
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
