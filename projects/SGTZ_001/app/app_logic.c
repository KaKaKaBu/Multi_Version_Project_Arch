#include "app_logic.h"
#include "board_config.h"
#include "devmgr.h"
#include "gpio_hal.h"
#if VERSION_FEATURE_VOICE
#include "usart_hal.h"
#endif
#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif

#if defined(PLATFORM_MCS51)
#include "mcs51_memory.h"
MCS51_XDATA app_context_t g_ctx;
#else
app_context_t g_ctx;
#endif

static const display_driver_t *app_display;
static const weight_sensor_t *app_weight;
static const distance_sensor_t *app_distance;
static const relay_driver_t *app_relay;
static const misc_driver_t *app_buzzer;
static unsigned char app_display_divider;
#if VERSION_FEATURE_VOICE
static unsigned char app_voice_ready;
#endif
#if VERSION_FEATURE_WIFI
static unsigned char app_mqtt_ready;
static unsigned char app_mqtt_publish_divider;
static MCS51_XDATA char app_mqtt_payload[160];
#endif

static char *app_puts(char *dst, const char *src)
{
    while (*src != '\0') {
        *dst = *src;
        ++dst;
        ++src;
    }
    *dst = '\0';
    return dst;
}

static char *app_put_u32(char *dst, unsigned long value)
{
    char buf[10];
    unsigned char i = 0U;

    if (value == 0UL) {
        *dst = '0';
        ++dst;
        *dst = '\0';
        return dst;
    }

    while ((value > 0UL) && (i < sizeof(buf))) {
        buf[i] = (char)('0' + (value % 10UL));
        value /= 10UL;
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

static char *app_put_2d(char *dst, unsigned char value)
{
    *dst = (char)('0' + (value / 10U));
    ++dst;
    *dst = (char)('0' + (value % 10U));
    ++dst;
    *dst = '\0';
    return dst;
}

static signed long app_float_to_long(float value)
{
    if (value >= 0.0f) {
        return (signed long)(value + 0.5f);
    }
    return (signed long)(value - 0.5f);
}

static unsigned long app_net_weight_g(signed long raw_g)
{
    signed long net = raw_g - g_ctx.tare_g;

    if (net <= 0L) {
        return 0UL;
    }
    if ((unsigned long)net <= BOARD_WEIGHT_ZERO_DEADBAND_G) {
        return 0UL;
    }
    return (unsigned long)net;
}

static void app_set_fan(unsigned char on)
{
    g_ctx.fan_on = (on != 0U) ? 1U : 0U;
    if (app_relay != 0) {
        app_relay->set_state(g_ctx.fan_on);
    }
}

static void app_set_buzzer(unsigned char on)
{
    g_ctx.alarm_on = (on != 0U) ? 1U : 0U;
    if (app_buzzer != 0) {
        app_buzzer->set_state(g_ctx.alarm_on);
    }
}

static unsigned char app_read_manual_mode(void)
{
#if VERSION_FEATURE_FAN_CONTROL
    unsigned char level = gpio_hal_read(board_mode_switch_pin.port, board_mode_switch_pin.pin);
    return (level == BOARD_MODE_SWITCH_MANUAL_LEVEL) ? 1U : 0U;
#else
    return 0U;
#endif
}

static app_bmi_state_t app_eval_bmi_state(unsigned short bmi_x10)
{
    if (bmi_x10 == 0U) {
        return APP_BMI_UNKNOWN;
    }
    if (bmi_x10 <= BOARD_BMI_LIGHT_MAX_X10) {
        return APP_BMI_LIGHT;
    }
    if (bmi_x10 <= BOARD_BMI_NORMAL_MAX_X10) {
        return APP_BMI_NORMAL;
    }
    return APP_BMI_HEAVY;
}

static void app_compute_bmi(void)
{
#if VERSION_FEATURE_BMI
    unsigned long height_sq;

    if (g_ctx.height_cm < BOARD_HUMAN_MIN_HEIGHT_CM) {
        g_ctx.bmi_x10 = 0U;
        g_ctx.bmi_state = APP_BMI_UNKNOWN;
        return;
    }

    height_sq = (unsigned long)g_ctx.height_cm * (unsigned long)g_ctx.height_cm;
    if (height_sq == 0UL) {
        g_ctx.bmi_x10 = 0U;
        g_ctx.bmi_state = APP_BMI_UNKNOWN;
        return;
    }

    g_ctx.bmi_x10 = (unsigned short)(((g_ctx.weight_g * 100UL) + (height_sq / 2UL)) / height_sq);
    g_ctx.bmi_state = app_eval_bmi_state(g_ctx.bmi_x10);
#endif
}

static const char *app_bmi_text(void)
{
    switch (g_ctx.bmi_state) {
    case APP_BMI_LIGHT:
        return "LIGHT";
    case APP_BMI_NORMAL:
        return "NORMAL";
    case APP_BMI_HEAVY:
        return "HEAVY";
    default:
        return "WAIT";
    }
}

static const char *app_bmi_display_text(void)
{
    switch (g_ctx.bmi_state) {
    case APP_BMI_LIGHT:
        return "偏轻";
    case APP_BMI_NORMAL:
        return "正常";
    case APP_BMI_HEAVY:
        return "偏重";
    default:
        return "等待";
    }
}

static void app_voice_cue(void)
{
#if VERSION_FEATURE_VOICE
    static const uint8_t voice_light_gb2312[] = {
        0xCCU, 0xE5U, 0xD6U, 0xD8U, 0xB9U, 0xFDU, 0xC7U, 0xE1U, 0x20U,
        0xBDU, 0xA8U, 0xD2U, 0xE9U, 0xC4U, 0xFAU, 0xD4U, 0xF6U, 0xB7U, 0xCAU
    };
    static const uint8_t voice_normal_gb2312[] = {
        0xCCU, 0xE5U, 0xD6U, 0xD8U, 0xD5U, 0xFDU, 0xB3U, 0xA3U, 0x20U,
        0xBDU, 0xA8U, 0xD2U, 0xE9U, 0xC4U, 0xFAU, 0xB1U, 0xA3U, 0xB3U, 0xD6U
    };
    static const uint8_t voice_heavy_gb2312[] = {
        0xCCU, 0xE5U, 0xD6U, 0xD8U, 0xB9U, 0xFDU, 0xD6U, 0xD8U, 0x20U,
        0xBDU, 0xA8U, 0xD2U, 0xE9U, 0xC4U, 0xFAU, 0xBCU, 0xF5U, 0xB7U, 0xCAU
    };
    static const uint8_t voice_newline[] = { 0x0DU, 0x0AU };
    const uint8_t *payload = 0;
    uint16_t payload_len = 0U;

    if (g_ctx.voice_paused == 0U) {
        g_ctx.voice_ticks = 8U;
        if (app_voice_ready != 0U) {
            if (g_ctx.bmi_state == APP_BMI_LIGHT) {
                payload = voice_light_gb2312;
                payload_len = (uint16_t)sizeof(voice_light_gb2312);
            } else if (g_ctx.bmi_state == APP_BMI_NORMAL) {
                payload = voice_normal_gb2312;
                payload_len = (uint16_t)sizeof(voice_normal_gb2312);
            } else if (g_ctx.bmi_state == APP_BMI_HEAVY) {
                payload = voice_heavy_gb2312;
                payload_len = (uint16_t)sizeof(voice_heavy_gb2312);
            }

            if (payload != 0) {
                (void)usart_hal_send_buffer(BOARD_VOICE_USART, payload, payload_len);
                (void)usart_hal_send_buffer(BOARD_VOICE_USART, voice_newline, (uint16_t)sizeof(voice_newline));
            }
        }
    }
#endif
}

static void app_voice_loop(void)
{
#if VERSION_FEATURE_VOICE
    if (g_ctx.voice_ticks > 0U) {
        --g_ctx.voice_ticks;
        app_set_buzzer((g_ctx.voice_ticks & 0x01U) != 0U);
    } else if (VERSION_FEATURE_WEIGHT_ALARM == 0) {
        app_set_buzzer(0U);
    }
#endif
}

static char *app_put_weight_kg(char *dst)
{
    dst = app_put_u32(dst, g_ctx.weight_g / 1000UL);
    *dst = '.';
    ++dst;
    *dst = (char)('0' + (unsigned char)((g_ctx.weight_g % 1000UL) / 100UL));
    ++dst;
    *dst = '\0';
    return app_puts(dst, "kg");
}

static char *app_put_height_m(char *dst)
{
    dst = app_put_u32(dst, (unsigned long)(g_ctx.height_cm / 100U));
    *dst = '.';
    ++dst;
    dst = app_put_2d(dst, (unsigned char)(g_ctx.height_cm % 100U));
    return app_puts(dst, "m");
}

static char *app_put_bmi(char *dst)
{
    dst = app_put_u32(dst, (unsigned long)(g_ctx.bmi_x10 / 10U));
    *dst = '.';
    ++dst;
    *dst = (char)('0' + (g_ctx.bmi_x10 % 10U));
    ++dst;
    *dst = '\0';
    return dst;
}

#if VERSION_FEATURE_WIFI
static unsigned char app_payload_contains(const unsigned char *payload, unsigned short len, const char *token)
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

static void app_mqtt_rx_callback(const char *topic, const unsigned char *payload, unsigned short len)
{
    signed long raw_g;

    (void)topic;
    if (app_payload_contains(payload, len, "unlock") != 0U) {
        g_ctx.locked = 0U;
        g_ctx.bmi_x10 = 0U;
        g_ctx.bmi_state = APP_BMI_UNKNOWN;
    } else if (app_payload_contains(payload, len, "lock") != 0U) {
        app_compute_bmi();
        g_ctx.locked = 1U;
    } else if ((app_weight != 0) && (app_payload_contains(payload, len, "tare") != 0U)) {
        raw_g = app_float_to_long(app_weight->read_grams());
        g_ctx.tare_g = raw_g;
        g_ctx.weight_g = 0UL;
    }
    g_ctx.display_dirty = 1U;
}

static void app_mqtt_publish_status(void)
{
    char *p;

    if ((app_mqtt_ready == 0U) || (esp8266_mqtt_is_ready() == 0)) {
        return;
    }

    p = app_mqtt_payload;
#if BOARD_ESP8266_MQTT_BACKEND == 1U
    p = app_puts(p, "{\"weight_g\":");
    p = app_put_u32(p, g_ctx.weight_g);
    p = app_puts(p, ",\"height_cm\":");
    p = app_put_u32(p, g_ctx.height_cm);
    p = app_puts(p, ",\"bmi_x10\":");
    p = app_put_u32(p, g_ctx.bmi_x10);
    p = app_puts(p, ",\"state\":\"");
    p = app_puts(p, app_bmi_text());
    p = app_puts(p, "\",\"locked\":");
    p = app_puts(p, (g_ctx.locked != 0U) ? "1" : "0");
    p = app_puts(p, ",\"fan_on\":");
    p = app_puts(p, (g_ctx.fan_on != 0U) ? "1" : "0");
    (void)app_puts(p, "}");

    (void)esp8266_mqtt_publish_huawei_properties(app_mqtt_payload);
#else
    p = app_puts(p, "{\"type\":\"telemetry\",\"device\":\"");
    p = app_puts(p, VERSION_NAME);
    p = app_puts(p, "\",\"weight_g\":");
    p = app_put_u32(p, g_ctx.weight_g);
    p = app_puts(p, ",\"height_cm\":");
    p = app_put_u32(p, g_ctx.height_cm);
    p = app_puts(p, ",\"bmi_x10\":");
    p = app_put_u32(p, g_ctx.bmi_x10);
    p = app_puts(p, ",\"state\":\"");
    p = app_puts(p, app_bmi_text());
    p = app_puts(p, "\",\"locked\":");
    p = app_puts(p, (g_ctx.locked != 0U) ? "1" : "0");
    (void)app_puts(p, "}");

    (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, app_mqtt_payload);
#endif
}
#endif

static void app_display_status(void)
{
    char line[DISPLAY_SMALL_MAX_CHARS + 1U];
    char *p;

    if (app_display == 0) {
        return;
    }

    app_display->clear();

    p = app_puts(line, VERSION_LABEL);
    p = app_puts(p, " ");
    (void)app_puts(p, VERSION_TITLE);
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, line);

#if VERSION_FEATURE_FAN_CONTROL
    p = app_puts(line, "模式:");
    p = app_puts(p, (g_ctx.manual_mode != 0U) ? "手动 " : "自动 ");
    p = app_puts(p, "风:");
    (void)app_puts(p, (g_ctx.fan_on != 0U) ? "开" : "关");
    app_display->print(0U, 1U, DISPLAY_FONT_SMALL, line);

    p = app_puts(line, "压力:");
    p = app_put_u32(p, g_ctx.weight_g);
    p = app_puts(p, "g 阈:");
    (void)app_put_u32(p, BOARD_PRESSURE_TRIGGER_G);
    app_display->print(0U, 2U, DISPLAY_FONT_SMALL, line);
    app_display->print(0U, 3U, DISPLAY_FONT_SMALL, "K1切换风扇");
#else
    p = app_puts(line, "体重:");
    (void)app_put_weight_kg(p);
    app_display->print(0U, 1U, DISPLAY_FONT_SMALL, line);

    p = app_puts(line, "身高:");
#if VERSION_FEATURE_HEIGHT
    if (g_ctx.height_cm > 0U) {
        (void)app_put_height_m(p);
    } else {
        (void)app_puts(p, "--");
    }
#else
    p = app_put_u32(p, g_ctx.threshold_g);
    (void)app_puts(p, "g");
#endif
    app_display->print(0U, 2U, DISPLAY_FONT_SMALL, line);

    p = app_puts(line, "");
#if VERSION_FEATURE_BMI
    p = app_puts(p, (g_ctx.locked != 0U) ? "锁定 " : "体指:");
    if (g_ctx.bmi_x10 > 0U) {
        p = app_put_bmi(p);
        p = app_puts(p, " ");
    }
    (void)app_puts(p, app_bmi_display_text());
#elif VERSION_FEATURE_WEIGHT_ALARM
    p = app_puts(p, "阈:");
    p = app_put_u32(p, g_ctx.threshold_g);
    p = app_puts(p, "g ");
    (void)app_puts(p, (g_ctx.alarm_on != 0U) ? "警:开" : "警:关");
#else
    (void)app_puts(p, "就绪");
#endif
    app_display->print(0U, 3U, DISPLAY_FONT_SMALL, line);
#endif

    app_display->update();
    g_ctx.display_dirty = 0U;
}

void app_logic_init(void)
{
#if VERSION_FEATURE_VOICE
    usart_hal_config_t voice_uart;
#endif
#if VERSION_FEATURE_WIFI
    esp8266_mqtt_config_t mqtt_cfg;
#endif

    app_display = devmgr_get_display("oled");
    app_weight = devmgr_get_weight_sensor("hx711");
    app_distance = devmgr_get_distance_sensor("hcsr04");
    app_relay = devmgr_get_relay("relay");
    app_buzzer = devmgr_get_misc("buzzer");

    g_ctx.weight_g = 0UL;
    g_ctx.tare_g = 0L;
    g_ctx.distance_cm = 0U;
    g_ctx.height_cm = 0U;
    g_ctx.bmi_x10 = 0U;
    g_ctx.threshold_g = BOARD_WEIGHT_ALARM_DEFAULT_G;
    g_ctx.bmi_state = APP_BMI_UNKNOWN;
    g_ctx.fan_on = 0U;
    g_ctx.alarm_on = 0U;
    g_ctx.manual_mode = 0U;
    g_ctx.locked = 0U;
    g_ctx.voice_paused = 0U;
    g_ctx.voice_ticks = 0U;
    g_ctx.display_dirty = 1U;

#if VERSION_FEATURE_FAN_CONTROL
    gpio_hal_config_pin(&board_mode_switch_pin);
#endif

#if VERSION_FEATURE_VOICE
    voice_uart.instance = BOARD_VOICE_USART;
    voice_uart.baudrate = BOARD_VOICE_BAUDRATE;
    voice_uart.tx = board_voice_tx_pin;
    voice_uart.rx = board_voice_rx_pin;
    voice_uart.remap = GPIO_HAL_REMAP_NONE;
    voice_uart.rx_buf_size = 0U;
    voice_uart.tx_timeout_us = BOARD_VOICE_TX_TIMEOUT_US;
    voice_uart.tx_mode = USART_HAL_TX_MODE_IRQ;
    app_voice_ready = (usart_hal_init(&voice_uart) == HAL_OK) ? 1U : 0U;
#endif

#if VERSION_FEATURE_WIFI
    mqtt_cfg.wifi_ssid = BOARD_ESP8266_WIFI_SSID;
    mqtt_cfg.wifi_password = BOARD_ESP8266_WIFI_PASS;
    mqtt_cfg.broker = BOARD_ESP8266_MQTT_BROKER;
    mqtt_cfg.port = BOARD_ESP8266_MQTT_PORT;
    mqtt_cfg.client_id = BOARD_ESP8266_MQTT_CLIENT_ID;
    mqtt_cfg.mqtt_user = BOARD_ESP8266_MQTT_USER;
    mqtt_cfg.mqtt_password = BOARD_ESP8266_MQTT_PASS;
    mqtt_cfg.sub_topic = BOARD_ESP8266_MQTT_SUB_TOPIC;
    mqtt_cfg.pub_topic = BOARD_ESP8266_MQTT_PUB_TOPIC;
    mqtt_cfg.backend = BOARD_ESP8266_MQTT_BACKEND;
    mqtt_cfg.scheme = BOARD_ESP8266_MQTT_SCHEME;
    mqtt_cfg.huawei_device_id = BOARD_ESP8266_HUAWEI_DEVICE_ID;
    mqtt_cfg.huawei_service_id = BOARD_ESP8266_HUAWEI_SERVICE_ID;
    mqtt_cfg.huawei_property_report_topic = BOARD_ESP8266_MQTT_PUB_TOPIC;
    mqtt_cfg.huawei_property_set_topic = BOARD_ESP8266_MQTT_SUB_TOPIC;
    mqtt_cfg.huawei_custom_sub_topic = BOARD_ESP8266_HUAWEI_CUSTOM_SUB_TOPIC;
    app_mqtt_ready = (esp8266_mqtt_connect(&mqtt_cfg) == 0) ? 1U : 0U;
    if (app_mqtt_ready != 0U) {
        esp8266_mqtt_register_rx_callback(app_mqtt_rx_callback);
    }
#endif

    if (app_weight != 0) {
        g_ctx.tare_g = app_float_to_long(app_weight->read_grams());
    }

    app_set_fan(0U);
    app_set_buzzer(0U);
    app_display_status();
}

void app_logic_on_sensor_tick(void)
{
    signed long raw_g;

    if ((app_weight != 0) && (g_ctx.locked == 0U)) {
        raw_g = app_float_to_long(app_weight->read_grams());
        g_ctx.weight_g = app_net_weight_g(raw_g);
    }

#if VERSION_FEATURE_HEIGHT
    if ((app_distance != 0) && (g_ctx.locked == 0U)) {
        g_ctx.distance_cm = app_distance->read_distance_cm();
        if ((g_ctx.distance_cm > 0U) && (g_ctx.distance_cm < BOARD_HEIGHT_SENSOR_MOUNT_CM)) {
            g_ctx.height_cm = (unsigned short)(BOARD_HEIGHT_SENSOR_MOUNT_CM - g_ctx.distance_cm);
            if (g_ctx.height_cm > BOARD_HUMAN_MAX_HEIGHT_CM) {
                g_ctx.height_cm = 0U;
            }
        } else {
            g_ctx.height_cm = 0U;
        }
    }
#endif

#if VERSION_FEATURE_BMI_REALTIME
    if (g_ctx.locked == 0U) {
        app_compute_bmi();
    }
#endif

#if VERSION_FEATURE_FAN_CONTROL
    g_ctx.manual_mode = app_read_manual_mode();
    if (g_ctx.manual_mode == 0U) {
        app_set_fan((g_ctx.weight_g >= BOARD_PRESSURE_TRIGGER_G) ? 1U : 0U);
    }
#endif

#if VERSION_FEATURE_WEIGHT_ALARM
    app_set_buzzer((g_ctx.weight_g > g_ctx.threshold_g) ? 1U : 0U);
#endif

#if VERSION_FEATURE_WIFI
    ++app_mqtt_publish_divider;
    if (app_mqtt_publish_divider >= 20U) {
        app_mqtt_publish_divider = 0U;
        app_mqtt_publish_status();
    }
#endif

    g_ctx.display_dirty = 1U;
}

void app_logic_on_key(unsigned char key_id)
{
#if VERSION_FEATURE_FAN_CONTROL
    g_ctx.manual_mode = app_read_manual_mode();
    if ((g_ctx.manual_mode != 0U) && (key_id == 1U)) {
        app_set_fan((g_ctx.fan_on == 0U) ? 1U : 0U);
    }
#elif VERSION_FEATURE_WEIGHT_ALARM
    if (key_id == 1U) {
        if (g_ctx.threshold_g <= (5000UL - BOARD_WEIGHT_ALARM_STEP_G)) {
            g_ctx.threshold_g += BOARD_WEIGHT_ALARM_STEP_G;
        }
    } else if (key_id == 2U) {
        if (g_ctx.threshold_g >= BOARD_WEIGHT_ALARM_STEP_G) {
            g_ctx.threshold_g -= BOARD_WEIGHT_ALARM_STEP_G;
        }
    } else if (key_id == 3U) {
        if (g_ctx.threshold_g <= (5000UL - BOARD_WEIGHT_ALARM_FAST_STEP_G)) {
            g_ctx.threshold_g += BOARD_WEIGHT_ALARM_FAST_STEP_G;
        }
    } else if (key_id == 4U) {
        g_ctx.threshold_g = BOARD_WEIGHT_ALARM_DEFAULT_G;
    }
#elif VERSION_FEATURE_BMI
    if (key_id == 1U) {
        app_compute_bmi();
        g_ctx.locked = 1U;
        app_voice_cue();
    } else if (key_id == 2U) {
        g_ctx.locked = 0U;
        g_ctx.bmi_x10 = 0U;
        g_ctx.bmi_state = APP_BMI_UNKNOWN;
    } else if (key_id == 3U) {
        g_ctx.voice_paused = (g_ctx.voice_paused == 0U) ? 1U : 0U;
        if (g_ctx.voice_paused != 0U) {
            g_ctx.voice_ticks = 0U;
            app_set_buzzer(0U);
        } else if (g_ctx.locked != 0U) {
            app_voice_cue();
        }
    }
#else
    (void)key_id;
#endif

    g_ctx.display_dirty = 1U;
}

void app_logic_loop(sched_event_t events)
{
    (void)events;
    app_voice_loop();
#if VERSION_FEATURE_WIFI
    esp8266_mqtt_poll();
#endif

    ++app_display_divider;
    if ((g_ctx.display_dirty != 0U) || (app_display_divider >= 20U)) {
        app_display_divider = 0U;
        app_display_status();
    }
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    app_logic_on_sensor_tick();
}

void logic_loop_run(sched_event_t events, void *ctx)
{
    (void)ctx;
    app_logic_loop(events);
}

void key_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
}
