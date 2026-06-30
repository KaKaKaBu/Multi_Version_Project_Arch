/* FJXT_001 车窗防夹业务逻辑：车窗状态机、按键/远程控制、防夹、显示与提醒。 */
#include "app_logic.h"
#include "devmgr.h"
#include "display_if.h"
#include "stepper_if.h"
#include "misc_if.h"
#include "comm_if.h"
#include "gpio_hal.h"
#include "comm_port.h"
#include "board_config.h"
#include "tiny_printf.h"
#include "version_config.h"

#if VERSION_FEATURE_REMOTE
#include "cJSON.h"
#include "cjson_port.h"
#include <string.h>
#endif

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
#include "esp8266_mqtt.h"
#endif

#define FJXT_ALARM_BLINK_MS 200U
#define FJXT_ALARM_HOLD_MS 1200U
#define FJXT_TELEMETRY_INTERVAL_MS 1000U

static const display_driver_t *display;
static const stepper_driver_t *stepper;
static const misc_driver_t *buzzer_drv;
static const misc_driver_t *led_drv;
#if VERSION_FEATURE_VOICE
static const comm_driver_t *voice_drv;
#endif

fjxt_context_t g_fjxt;

static uint8_t fjxt_read_active_pin(const hal_pin_t *pin, uint8_t active_low)
{
    uint8_t level;

    if (pin == 0) {
        return 0U;
    }

    level = gpio_hal_read(pin->port, pin->pin);
    return active_low ? (uint8_t)(level == 0U) : (uint8_t)(level != 0U);
}

static const char *fjxt_state_text(fjxt_window_state_t state)
{
    switch (state) {
    case FJXT_STATE_OPENING:
        return "Opening";
    case FJXT_STATE_CLOSING:
        return "Closing";
    case FJXT_STATE_OPEN_DONE:
        return "Open Done";
    case FJXT_STATE_CLOSE_DONE:
        return "Close Done";
    case FJXT_STATE_PINCH_REVERSING:
        return "Anti Pinch";
    default:
        return "Stopped";
    }
}

#if VERSION_FEATURE_CAMERA
static void fjxt_copy_text(char *dst, uint16_t dst_size, const char *src)
{
    uint16_t i;

    if ((dst == 0) || (dst_size == 0U)) {
        return;
    }

    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0U; (i < (uint16_t)(dst_size - 1U)) && (src[i] != '\0'); ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}
#endif

static void fjxt_set_alarm_output(uint8_t on)
{
    g_fjxt.alarm_output_on = (on != 0U) ? 1U : 0U;

    if (buzzer_drv != 0) {
        buzzer_drv->set_state(g_fjxt.alarm_output_on);
    }
    if (led_drv != 0) {
        led_drv->set_state(g_fjxt.alarm_output_on);
    }
}

#if VERSION_FEATURE_VOICE
static void fjxt_voice_play(uint8_t cmd_id)
{
    unsigned char payload[2];

    if (voice_drv == 0) {
        return;
    }

    payload[0] = 0x01U;
    payload[1] = cmd_id;
    (void)voice_drv->send(payload, 2U);
}
#else
static void fjxt_voice_play(uint8_t cmd_id)
{
    (void)cmd_id;
}
#endif

static void fjxt_start_alarm(uint8_t voice_cmd)
{
    uint32_t now = sched_tick_get();

    g_fjxt.alarm_active = 1U;
    g_fjxt.alarm_stop_tick = now + FJXT_ALARM_HOLD_MS;
    g_fjxt.last_alarm_tick = now;
    fjxt_set_alarm_output(1U);
    fjxt_voice_play(voice_cmd);
    event_set(APP_EVENT_ALARM);
}

static void fjxt_stop_motor(void)
{
    if (stepper != 0) {
        stepper->stop();
    }
}

static void fjxt_set_state(fjxt_window_state_t state)
{
    if (g_fjxt.state == state) {
        return;
    }

    g_fjxt.state = state;
    g_fjxt.display_dirty = 1U;
    app_logic_request_telemetry();
}

static void fjxt_complete_open(void)
{
    fjxt_stop_motor();
    g_fjxt.nudge_remaining_deg = 0U;
    g_fjxt.reverse_remaining_deg = 0U;
    fjxt_set_state(FJXT_STATE_OPEN_DONE);
    fjxt_start_alarm(BOARD_VOICE_CMD_OPEN_DONE);
}

static void fjxt_complete_close(void)
{
    fjxt_stop_motor();
    g_fjxt.nudge_remaining_deg = 0U;
    g_fjxt.reverse_remaining_deg = 0U;
    fjxt_set_state(FJXT_STATE_CLOSE_DONE);
    fjxt_start_alarm(BOARD_VOICE_CMD_CLOSE_DONE);
}

static void fjxt_start_open(uint8_t nudge)
{
    if (g_fjxt.open_limit != 0U) {
        fjxt_complete_open();
        return;
    }

    g_fjxt.nudge_remaining_deg = nudge ? BOARD_STEPMOTOR_NUDGE_DEGREE : 0U;
    g_fjxt.reverse_remaining_deg = 0U;
    fjxt_set_state(FJXT_STATE_OPENING);
    event_set(APP_EVENT_MOTOR);
}

static void fjxt_start_close(uint8_t nudge)
{
    if (g_fjxt.close_limit != 0U) {
        fjxt_complete_close();
        return;
    }

    g_fjxt.nudge_remaining_deg = nudge ? BOARD_STEPMOTOR_NUDGE_DEGREE : 0U;
    g_fjxt.reverse_remaining_deg = 0U;
    fjxt_set_state(FJXT_STATE_CLOSING);
    event_set(APP_EVENT_MOTOR);
}

static void fjxt_start_pinch_reverse(void)
{
    g_fjxt.nudge_remaining_deg = 0U;
    g_fjxt.reverse_remaining_deg = BOARD_STEPMOTOR_REVERSE_DEGREE;
    fjxt_set_state(FJXT_STATE_PINCH_REVERSING);
    fjxt_start_alarm(BOARD_VOICE_CMD_PINCH);
    event_set(APP_EVENT_MOTOR);
}

static void fjxt_apply_command(fjxt_command_t cmd)
{
    g_fjxt.pending_cmd = FJXT_CMD_NONE;

    switch (cmd) {
    case FJXT_CMD_OPEN_FULL:
        fjxt_start_open(0U);
        break;
    case FJXT_CMD_CLOSE_FULL:
        fjxt_start_close(0U);
        break;
    case FJXT_CMD_OPEN_NUDGE:
        fjxt_start_open(1U);
        break;
    case FJXT_CMD_CLOSE_NUDGE:
        fjxt_start_close(1U);
        break;
    case FJXT_CMD_STOP:
        fjxt_stop_motor();
        fjxt_set_state(FJXT_STATE_STOPPED);
        break;
    default:
        break;
    }
}

static void fjxt_step_motor(uint8_t direction)
{
    if (stepper == 0) {
        return;
    }

    stepper->step_angle(direction, BOARD_STEPMOTOR_STEP_DEGREE, BOARD_STEPMOTOR_STEP_DELAY_MS);
}

static void fjxt_refresh_display(void)
{
    if ((display == 0) || (g_fjxt.display_dirty == 0U)) {
        return;
    }

    display->clear();
    DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "FJXT Window");
    display->print(0U, 2U, DISPLAY_FONT_SMALL, "State:%s", fjxt_state_text(g_fjxt.state));
    display->print(0U, 4U, DISPLAY_FONT_SMALL, "Obj:%s", g_fjxt.pinch_detected ? "YES" : "NO");
    display->print(0U, 5U, DISPLAY_FONT_SMALL, "Open:%s Close:%s",
                   g_fjxt.open_limit ? "Y" : "N",
                   g_fjxt.close_limit ? "Y" : "N");
    display->print(0U, 7U, DISPLAY_FONT_SMALL, g_fjxt.alarm_active ? "ALARM" : "READY");
    display->update();
    g_fjxt.display_dirty = 0U;
}

#if VERSION_FEATURE_REMOTE
static const cJSON *fjxt_json_params(const cJSON *root)
{
    const cJSON *params = cJSON_GetObjectItem(root, "params");

    if ((params != 0) && ((params->type & cJSON_Object) != 0)) {
        return params;
    }

    return root;
}

static fjxt_command_t fjxt_command_from_text(const char *cmd)
{
    if (cmd == 0) {
        return FJXT_CMD_NONE;
    }
    if ((strcmp(cmd, "open") == 0) || (strcmp(cmd, "open_full") == 0)) {
        return FJXT_CMD_OPEN_FULL;
    }
    if ((strcmp(cmd, "close") == 0) || (strcmp(cmd, "close_full") == 0)) {
        return FJXT_CMD_CLOSE_FULL;
    }
    if ((strcmp(cmd, "open_step") == 0) || (strcmp(cmd, "open_nudge") == 0)) {
        return FJXT_CMD_OPEN_NUDGE;
    }
    if ((strcmp(cmd, "close_step") == 0) || (strcmp(cmd, "close_nudge") == 0)) {
        return FJXT_CMD_CLOSE_NUDGE;
    }
    if (strcmp(cmd, "stop") == 0) {
        return FJXT_CMD_STOP;
    }
    return FJXT_CMD_NONE;
}

static void fjxt_publish_telemetry(void)
{
    cJSON *root;
    cJSON *data;
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
        cJSON_AddStringToObject(data, "state", fjxt_state_text(g_fjxt.state));
        cJSON_AddNumberToObject(data, "open_limit", g_fjxt.open_limit);
        cJSON_AddNumberToObject(data, "close_limit", g_fjxt.close_limit);
        cJSON_AddNumberToObject(data, "pinch", g_fjxt.pinch_detected);
        cJSON_AddNumberToObject(data, "alarm", g_fjxt.alarm_active);
        cJSON_AddNumberToObject(data, "camera", VERSION_FEATURE_CAMERA);
        cJSON_AddNumberToObject(data, "cloud", VERSION_FEATURE_CLOUD);
#if VERSION_FEATURE_CAMERA
        cJSON_AddStringToObject(data, "camera_ip", g_fjxt.camera_ip);
        cJSON_AddStringToObject(data, "camera_stream", g_fjxt.camera_stream);
#endif
        cJSON_AddItemToObject(root, "data", data);
    }

    json_text = cJSON_PrintUnformatted(root);
    if (json_text != 0) {
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
        if (esp8266_mqtt_is_ready() != 0) {
            (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, json_text);
        } else
#endif
        {
            (void)comm_port_send((const unsigned char *)json_text, (unsigned short)strlen(json_text));
            (void)comm_port_send((const unsigned char *)"\r\n", 2U);
        }
        cjson_release_string(json_text);
    }

    cjson_release(root);
}
#endif

void app_logic_init(void)
{
    gpio_hal_config_pin(&board_open_limit_pin);
    gpio_hal_config_pin(&board_close_limit_pin);
    gpio_hal_config_pin(&board_pinch_sensor_pin);

    display = devmgr_get_display("oled");
    stepper = devmgr_get_stepper("stepmotor");
    buzzer_drv = devmgr_get_misc("buzzer");
    led_drv = devmgr_get_misc("led");
#if VERSION_FEATURE_VOICE
    voice_drv = devmgr_get_comm("su03t");
#endif

    g_fjxt.state = FJXT_STATE_STOPPED;
    g_fjxt.pending_cmd = FJXT_CMD_NONE;
    g_fjxt.open_limit = 0U;
    g_fjxt.close_limit = 0U;
    g_fjxt.pinch_detected = 0U;
    g_fjxt.alarm_active = 0U;
    g_fjxt.alarm_output_on = 0U;
    g_fjxt.display_dirty = 1U;
    g_fjxt.nudge_remaining_deg = 0U;
    g_fjxt.reverse_remaining_deg = 0U;
    g_fjxt.last_alarm_tick = sched_tick_get();
    g_fjxt.alarm_stop_tick = 0U;
    g_fjxt.last_telemetry_tick = 0U;
    g_fjxt.telemetry_pending = 1U;
#if VERSION_FEATURE_CAMERA
    g_fjxt.camera_ip[0] = '\0';
    g_fjxt.camera_stream[0] = '\0';
#endif

    fjxt_set_alarm_output(0U);
#if VERSION_FEATURE_REMOTE
    cjson_port_init();
#endif
}

void app_logic_handle_key(uint8_t key_index)
{
    switch (key_index) {
    case 1U:
        g_fjxt.pending_cmd = FJXT_CMD_OPEN_FULL;
        break;
    case 2U:
        g_fjxt.pending_cmd = FJXT_CMD_CLOSE_FULL;
        break;
    case 3U:
        g_fjxt.pending_cmd = FJXT_CMD_OPEN_NUDGE;
        break;
    case 4U:
        g_fjxt.pending_cmd = FJXT_CMD_CLOSE_NUDGE;
        break;
    default:
        return;
    }

    g_fjxt.display_dirty = 1U;
    event_set(APP_EVENT_MOTOR);
}

void sensor_loop_run(sched_event_t events, void *ctx)
{
    uint8_t old_pinch;
    uint8_t old_open;
    uint8_t old_close;

    (void)events;
    (void)ctx;

    old_pinch = g_fjxt.pinch_detected;
    old_open = g_fjxt.open_limit;
    old_close = g_fjxt.close_limit;

    g_fjxt.open_limit = fjxt_read_active_pin(&board_open_limit_pin, BOARD_OPEN_LIMIT_ACTIVE_LOW);
    g_fjxt.close_limit = fjxt_read_active_pin(&board_close_limit_pin, BOARD_CLOSE_LIMIT_ACTIVE_LOW);
    g_fjxt.pinch_detected = fjxt_read_active_pin(&board_pinch_sensor_pin, BOARD_PINCH_SENSOR_ACTIVE_LOW);

    if ((old_pinch != g_fjxt.pinch_detected) ||
        (old_open != g_fjxt.open_limit) ||
        (old_close != g_fjxt.close_limit)) {
        g_fjxt.display_dirty = 1U;
        app_logic_request_telemetry();
        event_set(APP_EVENT_MOTOR);
    }
}

void motor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (g_fjxt.pending_cmd != FJXT_CMD_NONE) {
        fjxt_apply_command(g_fjxt.pending_cmd);
    }

    if ((g_fjxt.state == FJXT_STATE_CLOSING) && (g_fjxt.pinch_detected != 0U)) {
        fjxt_start_pinch_reverse();
        return;
    }

    if (g_fjxt.state == FJXT_STATE_OPENING) {
        if (g_fjxt.open_limit != 0U) {
            fjxt_complete_open();
            return;
        }
        fjxt_step_motor(BOARD_STEPMOTOR_OPEN_DIR);
        if (g_fjxt.nudge_remaining_deg != 0U) {
            if (g_fjxt.nudge_remaining_deg <= BOARD_STEPMOTOR_STEP_DEGREE) {
                g_fjxt.nudge_remaining_deg = 0U;
                fjxt_stop_motor();
                fjxt_set_state(FJXT_STATE_STOPPED);
            } else {
                g_fjxt.nudge_remaining_deg = (uint16_t)(g_fjxt.nudge_remaining_deg - BOARD_STEPMOTOR_STEP_DEGREE);
            }
        }
    } else if (g_fjxt.state == FJXT_STATE_CLOSING) {
        if (g_fjxt.close_limit != 0U) {
            fjxt_complete_close();
            return;
        }
        fjxt_step_motor(BOARD_STEPMOTOR_CLOSE_DIR);
        if (g_fjxt.nudge_remaining_deg != 0U) {
            if (g_fjxt.nudge_remaining_deg <= BOARD_STEPMOTOR_STEP_DEGREE) {
                g_fjxt.nudge_remaining_deg = 0U;
                fjxt_stop_motor();
                fjxt_set_state(FJXT_STATE_STOPPED);
            } else {
                g_fjxt.nudge_remaining_deg = (uint16_t)(g_fjxt.nudge_remaining_deg - BOARD_STEPMOTOR_STEP_DEGREE);
            }
        }
    } else if (g_fjxt.state == FJXT_STATE_PINCH_REVERSING) {
        if ((g_fjxt.open_limit != 0U) || (g_fjxt.reverse_remaining_deg == 0U)) {
            fjxt_complete_open();
            return;
        }
        fjxt_step_motor(BOARD_STEPMOTOR_OPEN_DIR);
        if (g_fjxt.reverse_remaining_deg <= BOARD_STEPMOTOR_STEP_DEGREE) {
            g_fjxt.reverse_remaining_deg = 0U;
        } else {
            g_fjxt.reverse_remaining_deg = (uint16_t)(g_fjxt.reverse_remaining_deg - BOARD_STEPMOTOR_STEP_DEGREE);
        }
    }
}

void alarm_loop_run(sched_event_t events, void *ctx)
{
    uint32_t now;

    (void)events;
    (void)ctx;

    if (g_fjxt.alarm_active == 0U) {
        return;
    }

    now = sched_tick_get();
    if ((int32_t)(now - g_fjxt.alarm_stop_tick) >= 0) {
        g_fjxt.alarm_active = 0U;
        fjxt_set_alarm_output(0U);
        g_fjxt.display_dirty = 1U;
        app_logic_request_telemetry();
        return;
    }

    if ((now - g_fjxt.last_alarm_tick) >= FJXT_ALARM_BLINK_MS) {
        fjxt_set_alarm_output((g_fjxt.alarm_output_on == 0U) ? 1U : 0U);
        g_fjxt.last_alarm_tick = now;
    }
}

void display_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    fjxt_refresh_display();
}

void app_logic_request_telemetry(void)
{
#if VERSION_FEATURE_REMOTE
    g_fjxt.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
#endif
}

void comm_loop_run(sched_event_t events, void *ctx)
{
#if VERSION_FEATURE_REMOTE
    uint32_t now;
#endif

    (void)ctx;

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_CLOUD
    if ((events & (APP_EVENT_COMM_RX | SCHED_EVENT_TICK)) != 0U) {
        esp8266_mqtt_poll();
    }
#endif

#if VERSION_FEATURE_REMOTE
    now = sched_tick_get();
    if ((now - g_fjxt.last_telemetry_tick) >= FJXT_TELEMETRY_INTERVAL_MS) {
        g_fjxt.telemetry_pending = 1U;
    }

    if (g_fjxt.telemetry_pending != 0U) {
        g_fjxt.telemetry_pending = 0U;
        g_fjxt.last_telemetry_tick = now;
        fjxt_publish_telemetry();
    }
#else
    (void)events;
#endif
}

void app_logic_on_remote_rx(const char *json_data)
{
#if VERSION_FEATURE_REMOTE
    cJSON *root;
    cJSON *cmd_item;
    const cJSON *params;
    const char *cmd_text;
    fjxt_command_t cmd;

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

    cmd_text = cmd_item->valuestring;
    params = fjxt_json_params(root);
    (void)params;

    if (strcmp(cmd_text, "get_status") == 0) {
        app_logic_request_telemetry();
    } else {
        cmd = fjxt_command_from_text(cmd_text);
        if (cmd != FJXT_CMD_NONE) {
            g_fjxt.pending_cmd = cmd;
            event_set(APP_EVENT_MOTOR);
        }
#if VERSION_FEATURE_CAMERA
        else if (strcmp(cmd_text, "camera_info") == 0) {
            const cJSON *ip_item = cJSON_GetObjectItem(params, "ip");
            const cJSON *stream_item = cJSON_GetObjectItem(params, "stream");

            if ((ip_item != 0) && cJSON_IsString(ip_item)) {
                fjxt_copy_text(g_fjxt.camera_ip, (uint16_t)sizeof(g_fjxt.camera_ip), ip_item->valuestring);
            }
            if ((stream_item != 0) && cJSON_IsString(stream_item)) {
                fjxt_copy_text(g_fjxt.camera_stream, (uint16_t)sizeof(g_fjxt.camera_stream), stream_item->valuestring);
            }
            g_fjxt.display_dirty = 1U;
            app_logic_request_telemetry();
        }
#endif
    }

    cjson_release(root);
#else
    (void)json_data;
#endif
}
