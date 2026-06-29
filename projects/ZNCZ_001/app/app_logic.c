/* ZNCZ_001 智能插座业务逻辑：继电器、RTC 定时、WiFi 协议、显示和状态机处理。 */
#include "app_logic.h"
#include "app_fsm.h"
#include "board_config.h"
#include "cjson_port.h"
#include "devmgr.h"
#include "debug_log.h"
#include "version_config.h"
#include <string.h>

#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif

#define APP_WIFI_PAGE_SSID 0U
#define APP_WIFI_PAGE_PASS 1U
#define APP_WIFI_PAGE_STATUS 2U

#define APP_TIMER_FIELD_CURRENT 0U
#define APP_TIMER_FIELD_ON 1U
#define APP_TIMER_FIELD_OFF 2U

#define APP_TIMER_DIGIT_HOUR 0U
#define APP_TIMER_DIGIT_MINUTE 1U
#define APP_TIMER_DIGIT_SECOND 2U

typedef enum {
    APP_FSM_STATE_MANUAL_MAIN = 0,
    APP_FSM_STATE_MANUAL_WIFI_SSID,
    APP_FSM_STATE_MANUAL_WIFI_PASS,
    APP_FSM_STATE_MANUAL_WIFI_STATUS,
    APP_FSM_STATE_TIMER
} app_fsm_state_t;

#define APP_FSM_EVT_KEY1 1U
#define APP_FSM_EVT_KEY2 2U
#define APP_FSM_EVT_KEY3 3U
#define APP_FSM_EVT_KEY4 4U
#define APP_FSM_EVT_KEY5 5U
#define APP_FSM_EVT_COMM_RELAY 6U

typedef struct app_hms {
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} app_hms_t;

typedef struct app_context {
    rtc_time_t now;
    app_hms_t on_time;
    app_hms_t off_time;
    unsigned char relay_state;
    unsigned char timer_field;
    unsigned char timer_digit;
    unsigned char display_dirty;
    signed char comm_pending_relay;
} app_context_t;

static app_context_t app_ctx;
static app_fsm_t app_ui_fsm;
static const rtc_driver_t *app_rtc;
static const relay_driver_t *app_relay;
static const display_driver_t *app_display;

extern const unsigned char *app_get_last_rx(unsigned short *len);
extern void app_clear_last_rx(void);

static unsigned char app_fsm_is_timer(unsigned short state)
{
    return (state == (unsigned short)APP_FSM_STATE_TIMER) ? 1U : 0U;
}

static void app_mark_dirty(void *context)
{
    app_context_t *ctx = (app_context_t *)context;

    if (ctx != 0) {
        ctx->display_dirty = 1U;
    }
}

static unsigned char app_clamp_hour(unsigned char value)
{
    return (unsigned char)(value % 24U);
}

static unsigned char app_clamp_minute_second(unsigned char value)
{
    return (unsigned char)(value % 60U);
}

static app_hms_t *app_timer_field_hms(unsigned char field)
{
    if (field == APP_TIMER_FIELD_ON) {
        return &app_ctx.on_time;
    }
    if (field == APP_TIMER_FIELD_OFF) {
        return &app_ctx.off_time;
    }

    return 0;
}

static void app_adjust_digit(app_hms_t *time, unsigned char digit, int delta);

static void app_adjust_current_digit(unsigned char digit, int delta)
{
    app_hms_t tmp;

    tmp.hour = app_ctx.now.hour;
    tmp.minute = app_ctx.now.minute;
    tmp.second = app_ctx.now.second;
    app_adjust_digit(&tmp, digit, delta);
    app_ctx.now.hour = tmp.hour;
    app_ctx.now.minute = tmp.minute;
    app_ctx.now.second = tmp.second;
}

static void app_rtc_read_now(void)
{
    if (app_rtc != 0) {
        app_rtc->read_time(&app_ctx.now);
    }
}

static void app_rtc_write_now(void)
{
    if (app_rtc != 0) {
        app_rtc->set_time(&app_ctx.now);
    }
}

static void app_relay_apply(unsigned char on)
{
    app_ctx.relay_state = (on != 0U) ? 1U : 0U;
    if (app_relay != 0) {
        app_relay->set_state(app_ctx.relay_state);
    }
    app_ctx.display_dirty = 1U;
}

static unsigned char app_timer_should_relay_on(void)
{
    unsigned long now_sec;
    unsigned long on_sec;
    unsigned long off_sec;

    now_sec = (unsigned long)app_ctx.now.hour * 3600UL
            + (unsigned long)app_ctx.now.minute * 60UL
            + (unsigned long)app_ctx.now.second;
    on_sec = (unsigned long)app_ctx.on_time.hour * 3600UL
           + (unsigned long)app_ctx.on_time.minute * 60UL
           + (unsigned long)app_ctx.on_time.second;
    off_sec = (unsigned long)app_ctx.off_time.hour * 3600UL
            + (unsigned long)app_ctx.off_time.minute * 60UL
            + (unsigned long)app_ctx.off_time.second;

    if (on_sec <= off_sec) {
        return (now_sec >= on_sec && now_sec < off_sec) ? 1U : 0U;
    }

    return (now_sec >= on_sec || now_sec < off_sec) ? 1U : 0U;
}

static void app_timer_apply_relay(void)
{
    if (app_fsm_is_timer(app_ui_fsm.state) == 0U) {
        return;
    }

    app_relay_apply(app_timer_should_relay_on());
}

static void app_format_hms(char *buf, unsigned short buf_len, const app_hms_t *time)
{
    if ((buf == 0) || (buf_len < 9U) || (time == 0)) {
        return;
    }

    buf[0] = (char)('0' + (time->hour / 10U));
    buf[1] = (char)('0' + (time->hour % 10U));
    buf[2] = ':';
    buf[3] = (char)('0' + (time->minute / 10U));
    buf[4] = (char)('0' + (time->minute % 10U));
    buf[5] = ':';
    buf[6] = (char)('0' + (time->second / 10U));
    buf[7] = (char)('0' + (time->second % 10U));
    buf[8] = '\0';
}

static void app_print_time_line(unsigned char row,
                                const char *prefix,
                                const app_hms_t *time,
                                unsigned char line_field_id)
{
    char line[20];
    unsigned char pos = 0U;
    unsigned char active_field = app_ctx.timer_field;
    unsigned char active_digit = app_ctx.timer_digit;

    if ((app_display == 0) || (prefix == 0) || (time == 0)) {
        return;
    }

    line[pos++] = (line_field_id == active_field) ? '*' : ' ';
    line[pos++] = prefix[0];
    line[pos++] = prefix[1];
    line[pos++] = prefix[2];
    line[pos++] = ':';

    line[pos++] = (line_field_id == active_field && active_digit == APP_TIMER_DIGIT_HOUR) ? '>' : ' ';
    line[pos++] = (char)('0' + (time->hour / 10U));
    line[pos++] = (char)('0' + (time->hour % 10U));
    line[pos++] = ':';

    line[pos++] = (line_field_id == active_field && active_digit == APP_TIMER_DIGIT_MINUTE) ? '>' : ' ';
    line[pos++] = (char)('0' + (time->minute / 10U));
    line[pos++] = (char)('0' + (time->minute % 10U));
    line[pos++] = ':';

    line[pos++] = (line_field_id == active_field && active_digit == APP_TIMER_DIGIT_SECOND) ? '>' : ' ';
    line[pos++] = (char)('0' + (time->second / 10U));
    line[pos++] = (char)('0' + (time->second % 10U));
    line[pos] = '\0';

    app_display->print(0U, row, DISPLAY_FONT_SMALL, "%s", line);
}

static const char *app_wifi_status_text(void)
{
#if VERSION_FEATURE_WIFI
    if (esp8266_mqtt_is_ready() != 0) {
        return "connected";
    }
#endif
    return "offline";
}

static void app_display_manual_main(void)
{
    char time_text[12];

    if (app_display == 0) {
        return;
    }

    app_display->clear();
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, "Mode:MANUAL");
    app_display->print(0U, 2U, DISPLAY_FONT_SMALL, "Relay:%s",
                       app_ctx.relay_state != 0U ? "ON " : "OFF");
    app_hms_t cur_time;

    cur_time.hour = app_ctx.now.hour;
    cur_time.minute = app_ctx.now.minute;
    cur_time.second = app_ctx.now.second;
    app_format_hms(time_text, (unsigned short)sizeof(time_text), &cur_time);
    app_display->print(0U, 4U, DISPLAY_FONT_SMALL, "Time:%s", time_text);
    app_display->update();
}

static void app_display_manual_wifi(void)
{
    const char *title;
    const char *value;
    unsigned char wifi_page = APP_WIFI_PAGE_SSID;

    if (app_display == 0) {
        return;
    }

    if (app_ui_fsm.state == (unsigned short)APP_FSM_STATE_MANUAL_WIFI_PASS) {
        wifi_page = APP_WIFI_PAGE_PASS;
    } else if (app_ui_fsm.state == (unsigned short)APP_FSM_STATE_MANUAL_WIFI_STATUS) {
        wifi_page = APP_WIFI_PAGE_STATUS;
    }

    if (wifi_page == APP_WIFI_PAGE_PASS) {
        title = "WiFi PASS";
        value = BOARD_ESP8266_WIFI_PASS;
    } else if (wifi_page == APP_WIFI_PAGE_STATUS) {
        title = "WiFi STAT";
        value = app_wifi_status_text();
    } else {
        title = "WiFi SSID";
        value = BOARD_ESP8266_WIFI_SSID;
    }

    app_display->clear();
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, "%s", title);
    app_display->print(0U, 2U, DISPLAY_FONT_SMALL, "%s", value);
    app_display->print(0U, 5U, DISPLAY_FONT_SMALL, "K2:Back");
    app_display->update();
}

static void app_display_timer(void)
{
    if (app_display == 0) {
        return;
    }

    app_display->clear();
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, "Mode:TIMER");
    {
        app_hms_t cur_time;

        cur_time.hour = app_ctx.now.hour;
        cur_time.minute = app_ctx.now.minute;
        cur_time.second = app_ctx.now.second;
        app_print_time_line(2U, "Cur", &cur_time, APP_TIMER_FIELD_CURRENT);
    }
    app_print_time_line(3U, "On ", &app_ctx.on_time, APP_TIMER_FIELD_ON);
    app_print_time_line(4U, "Off", &app_ctx.off_time, APP_TIMER_FIELD_OFF);
    app_display->print(0U, 6U, DISPLAY_FONT_SMALL, "Relay:%s",
                       app_ctx.relay_state != 0U ? "ON" : "OFF");
    app_display->update();
}

static void app_display_refresh(void)
{
    if (app_ui_fsm.state == (unsigned short)APP_FSM_STATE_TIMER) {
        app_display_timer();
        return;
    }

    if (app_ui_fsm.state == (unsigned short)APP_FSM_STATE_MANUAL_MAIN) {
        app_display_manual_main();
        return;
    }

    app_display_manual_wifi();
}

static void app_adjust_digit(app_hms_t *time, unsigned char digit, int delta)
{
    int value;

    if (time == 0) {
        return;
    }

    if (digit == APP_TIMER_DIGIT_HOUR) {
        value = (int)time->hour + delta;
        while (value < 0) {
            value += 24;
        }
        time->hour = app_clamp_hour((unsigned char)value);
        return;
    }

    if (digit == APP_TIMER_DIGIT_MINUTE) {
        value = (int)time->minute + delta;
        while (value < 0) {
            value += 60;
        }
        time->minute = app_clamp_minute_second((unsigned char)value);
        return;
    }

    value = (int)time->second + delta;
    while (value < 0) {
        value += 60;
    }
    time->second = app_clamp_minute_second((unsigned char)value);
}

static void app_fsm_action_enter_timer(void *context)
{
    app_context_t *ctx = (app_context_t *)context;

    if (ctx == 0) {
        return;
    }

    ctx->timer_field = APP_TIMER_FIELD_CURRENT;
    ctx->timer_digit = APP_TIMER_DIGIT_HOUR;
    app_timer_apply_relay();
    app_mark_dirty(context);
}

static void app_fsm_action_enter_manual_main(void *context)
{
    app_mark_dirty(context);
}

static void app_fsm_action_toggle_relay(void *context)
{
    app_context_t *ctx = (app_context_t *)context;

    if (ctx == 0) {
        return;
    }

    app_relay_apply(ctx->relay_state == 0U ? 1U : 0U);
}

static void app_fsm_action_timer_next_field(void *context)
{
    app_context_t *ctx = (app_context_t *)context;

    if (ctx == 0) {
        return;
    }

    ctx->timer_field = (unsigned char)((ctx->timer_field + 1U) % 3U);
    app_mark_dirty(context);
}

static void app_fsm_action_timer_next_digit(void *context)
{
    app_context_t *ctx = (app_context_t *)context;

    if (ctx == 0) {
        return;
    }

    ctx->timer_digit = (unsigned char)((ctx->timer_digit + 1U) % 3U);
    app_mark_dirty(context);
}

static void app_fsm_action_timer_digit_up(void *context)
{
    app_context_t *ctx = (app_context_t *)context;
    app_hms_t *field_time;

    if (ctx == 0) {
        return;
    }

    if (ctx->timer_field == APP_TIMER_FIELD_CURRENT) {
        app_adjust_current_digit(ctx->timer_digit, 1);
        app_rtc_write_now();
        app_timer_apply_relay();
        app_mark_dirty(context);
        return;
    }

    field_time = app_timer_field_hms(ctx->timer_field);
    app_adjust_digit(field_time, ctx->timer_digit, 1);
    app_timer_apply_relay();
    app_mark_dirty(context);
}

static void app_fsm_action_timer_digit_down(void *context)
{
    app_context_t *ctx = (app_context_t *)context;
    app_hms_t *field_time;

    if (ctx == 0) {
        return;
    }

    if (ctx->timer_field == APP_TIMER_FIELD_CURRENT) {
        app_adjust_current_digit(ctx->timer_digit, -1);
        app_rtc_write_now();
        app_timer_apply_relay();
        app_mark_dirty(context);
        return;
    }

    field_time = app_timer_field_hms(ctx->timer_field);
    app_adjust_digit(field_time, ctx->timer_digit, -1);
    app_timer_apply_relay();
    app_mark_dirty(context);
}

static void app_fsm_action_comm_relay(void *context)
{
    app_context_t *ctx = (app_context_t *)context;

    if ((ctx == 0) || (ctx->comm_pending_relay < 0)) {
        return;
    }

    app_relay_apply(ctx->comm_pending_relay != 0 ? 1U : 0U);
    ctx->comm_pending_relay = -1;
    app_mark_dirty(context);
}

static const app_fsm_transition_t app_ui_transitions[] = {
    {APP_FSM_STATE_MANUAL_MAIN, APP_FSM_EVT_KEY1, APP_FSM_STATE_TIMER, app_fsm_action_enter_timer},
    {APP_FSM_STATE_MANUAL_WIFI_SSID, APP_FSM_EVT_KEY1, APP_FSM_STATE_TIMER, app_fsm_action_enter_timer},
    {APP_FSM_STATE_MANUAL_WIFI_PASS, APP_FSM_EVT_KEY1, APP_FSM_STATE_TIMER, app_fsm_action_enter_timer},
    {APP_FSM_STATE_MANUAL_WIFI_STATUS, APP_FSM_EVT_KEY1, APP_FSM_STATE_TIMER, app_fsm_action_enter_timer},
    {APP_FSM_STATE_TIMER, APP_FSM_EVT_KEY1, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_enter_manual_main},

    {APP_FSM_STATE_MANUAL_MAIN, APP_FSM_EVT_KEY2, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_toggle_relay},
    {APP_FSM_STATE_MANUAL_WIFI_SSID, APP_FSM_EVT_KEY2, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_enter_manual_main},
    {APP_FSM_STATE_MANUAL_WIFI_PASS, APP_FSM_EVT_KEY2, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_enter_manual_main},
    {APP_FSM_STATE_MANUAL_WIFI_STATUS, APP_FSM_EVT_KEY2, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_enter_manual_main},
    {APP_FSM_STATE_TIMER, APP_FSM_EVT_KEY2, APP_FSM_STATE_TIMER, app_fsm_action_timer_next_field},

    {APP_FSM_STATE_MANUAL_MAIN, APP_FSM_EVT_KEY3, APP_FSM_STATE_MANUAL_WIFI_SSID, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_SSID, APP_FSM_EVT_KEY3, APP_FSM_STATE_MANUAL_WIFI_SSID, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_PASS, APP_FSM_EVT_KEY3, APP_FSM_STATE_MANUAL_WIFI_SSID, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_STATUS, APP_FSM_EVT_KEY3, APP_FSM_STATE_MANUAL_WIFI_SSID, app_mark_dirty},
    {APP_FSM_STATE_TIMER, APP_FSM_EVT_KEY3, APP_FSM_STATE_TIMER, app_fsm_action_timer_next_digit},

    {APP_FSM_STATE_MANUAL_MAIN, APP_FSM_EVT_KEY4, APP_FSM_STATE_MANUAL_WIFI_PASS, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_SSID, APP_FSM_EVT_KEY4, APP_FSM_STATE_MANUAL_WIFI_PASS, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_PASS, APP_FSM_EVT_KEY4, APP_FSM_STATE_MANUAL_WIFI_PASS, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_STATUS, APP_FSM_EVT_KEY4, APP_FSM_STATE_MANUAL_WIFI_PASS, app_mark_dirty},
    {APP_FSM_STATE_TIMER, APP_FSM_EVT_KEY4, APP_FSM_STATE_TIMER, app_fsm_action_timer_digit_up},

    {APP_FSM_STATE_MANUAL_MAIN, APP_FSM_EVT_KEY5, APP_FSM_STATE_MANUAL_WIFI_STATUS, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_SSID, APP_FSM_EVT_KEY5, APP_FSM_STATE_MANUAL_WIFI_STATUS, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_PASS, APP_FSM_EVT_KEY5, APP_FSM_STATE_MANUAL_WIFI_STATUS, app_mark_dirty},
    {APP_FSM_STATE_MANUAL_WIFI_STATUS, APP_FSM_EVT_KEY5, APP_FSM_STATE_MANUAL_WIFI_STATUS, app_mark_dirty},
    {APP_FSM_STATE_TIMER, APP_FSM_EVT_KEY5, APP_FSM_STATE_TIMER, app_fsm_action_timer_digit_down},

    {APP_FSM_STATE_MANUAL_MAIN, APP_FSM_EVT_COMM_RELAY, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_comm_relay},
    {APP_FSM_STATE_MANUAL_WIFI_SSID, APP_FSM_EVT_COMM_RELAY, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_comm_relay},
    {APP_FSM_STATE_MANUAL_WIFI_PASS, APP_FSM_EVT_COMM_RELAY, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_comm_relay},
    {APP_FSM_STATE_MANUAL_WIFI_STATUS, APP_FSM_EVT_COMM_RELAY, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_comm_relay},
    {APP_FSM_STATE_TIMER, APP_FSM_EVT_COMM_RELAY, APP_FSM_STATE_MANUAL_MAIN, app_fsm_action_comm_relay},
};

void app_logic_on_key(unsigned char key_id)
{
    if ((key_id == 0U) || (key_id > 5U)) {
        return;
    }

    (void)app_fsm_dispatch(&app_ui_fsm, key_id);
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

static void app_set_hms_from_json(cJSON *root, app_hms_t *time)
{
    int value;

    if ((root == 0) || (time == 0)) {
        return;
    }

    if (app_json_get_int(root, "hour", &value) == 0) {
        time->hour = app_clamp_hour((unsigned char)value);
    }
    if (app_json_get_int(root, "minute", &value) == 0) {
        time->minute = app_clamp_minute_second((unsigned char)value);
    }
    if (app_json_get_int(root, "second", &value) == 0) {
        time->second = app_clamp_minute_second((unsigned char)value);
    }
}

static void app_publish_status(void)
{
#if VERSION_FEATURE_WIFI
    cJSON *root;
    char time_text[12];
    char on_text[12];
    char off_text[12];
    char *json_text;
    const char *mode_text;

    if (esp8266_mqtt_is_ready() == 0) {
        return;
    }

    root = cJSON_CreateObject();
    if (root == 0) {
        return;
    }

    app_hms_t cur_time;

    cur_time.hour = app_ctx.now.hour;
    cur_time.minute = app_ctx.now.minute;
    cur_time.second = app_ctx.now.second;
    app_format_hms(time_text, (unsigned short)sizeof(time_text), &cur_time);
    app_format_hms(on_text, (unsigned short)sizeof(on_text), &app_ctx.on_time);
    app_format_hms(off_text, (unsigned short)sizeof(off_text), &app_ctx.off_time);
    mode_text = app_fsm_is_timer(app_ui_fsm.state) ? "timer" : "manual";

    (void)cJSON_AddNumberToObject(root, "relay", app_ctx.relay_state);
    (void)cJSON_AddStringToObject(root, "mode", mode_text);
    (void)cJSON_AddStringToObject(root, "time", time_text);
    (void)cJSON_AddStringToObject(root, "on_time", on_text);
    (void)cJSON_AddStringToObject(root, "off_time", off_text);
    (void)cJSON_AddStringToObject(root, "wifi", app_wifi_status_text());

    json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_text != 0) {
        (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, json_text);
        cjson_release_string(json_text);
    }
#endif
}

static void app_handle_comm_json(const char *text, unsigned short len)
{
    cJSON *root;
    cJSON *cmd;
    int state;

    root = cjson_parse(text, len);
    if (root == 0) {
        return;
    }

    cmd = cJSON_GetObjectItem(root, "cmd");
    if ((cmd == 0) || ((cmd->type & cJSON_String) == 0) || (cmd->valuestring == 0)) {
        cjson_release(root);
        return;
    }

    if (strcmp(cmd->valuestring, "set_time") == 0) {
        int year = (int)app_ctx.now.year;
        int month = (int)app_ctx.now.month;
        int day = (int)app_ctx.now.day;

        app_set_hms_from_json(root, (app_hms_t *)&app_ctx.now.hour);
        if (app_json_get_int(root, "year", &year) == 0) {
            app_ctx.now.year = (unsigned short)year;
        }
        if (app_json_get_int(root, "month", &month) == 0) {
            app_ctx.now.month = (unsigned char)month;
        }
        if (app_json_get_int(root, "day", &day) == 0) {
            app_ctx.now.day = (unsigned char)day;
        }
        app_rtc_write_now();
        app_ctx.display_dirty = 1U;
        app_publish_status();
    } else if (strcmp(cmd->valuestring, "set_on_time") == 0) {
        app_set_hms_from_json(root, &app_ctx.on_time);
        app_timer_apply_relay();
        app_ctx.display_dirty = 1U;
    } else if (strcmp(cmd->valuestring, "set_off_time") == 0) {
        app_set_hms_from_json(root, &app_ctx.off_time);
        app_timer_apply_relay();
        app_ctx.display_dirty = 1U;
    } else if (strcmp(cmd->valuestring, "relay") == 0) {
        if (app_json_get_int(root, "state", &state) == 0) {
            app_ctx.comm_pending_relay = (state != 0) ? 1 : 0;
            (void)app_fsm_dispatch(&app_ui_fsm, APP_FSM_EVT_COMM_RELAY);
        }
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

void app_logic_on_second_tick(void)
{
    app_rtc_read_now();
    DEBUG_LOG_RTC("[RTC] app now=%04u-%02u-%02u %02u:%02u:%02u fsm=%u dirty=1\r\n",
                  (unsigned int)app_ctx.now.year,
                  (unsigned int)app_ctx.now.month,
                  (unsigned int)app_ctx.now.day,
                  (unsigned int)app_ctx.now.hour,
                  (unsigned int)app_ctx.now.minute,
                  (unsigned int)app_ctx.now.second,
                  (unsigned int)app_ui_fsm.state);

    if (app_fsm_is_timer(app_ui_fsm.state) != 0U) {
        app_timer_apply_relay();
    }

    app_ctx.display_dirty = 1U;
}

void app_logic_init(void)
{
    app_rtc = devmgr_get_rtc("ds1302");
    app_relay = devmgr_get_relay("relay");
    app_display = devmgr_get_display("oled");
    DEBUG_LOG_RTC("[RTC] init driver=%s display=%s\r\n",
                  (app_rtc != 0) ? app_rtc->name : "null",
                  (app_display != 0) ? app_display->name : "null");

    app_ctx.timer_field = APP_TIMER_FIELD_CURRENT;
    app_ctx.timer_digit = APP_TIMER_DIGIT_HOUR;
    app_ctx.relay_state = 0U;
    app_ctx.comm_pending_relay = -1;
    app_ctx.on_time.hour = 8U;
    app_ctx.on_time.minute = 0U;
    app_ctx.on_time.second = 0U;
    app_ctx.off_time.hour = 22U;
    app_ctx.off_time.minute = 0U;
    app_ctx.off_time.second = 0U;
    app_ctx.display_dirty = 1U;

    app_fsm_init(&app_ui_fsm,
                 (unsigned short)APP_FSM_STATE_MANUAL_MAIN,
                 app_ui_transitions,
                 (unsigned short)(sizeof(app_ui_transitions) / sizeof(app_ui_transitions[0])),
                 &app_ctx);

    app_rtc_read_now();
    app_relay_apply(0U);
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
