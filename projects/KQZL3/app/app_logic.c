/* KQZL3 空气质量检测业务逻辑：传感器采样、报警、显示、按键和远程协议处理。 */
#include "app_logic.h"
#include "devmgr.h"
#include "gas_if.h"
#include "sensor_if.h"
#include "actuator_if.h"
#include "misc_if.h"
#include "display_if.h"
#include "input_if.h"
#include "comm_if.h"
#include "board_config.h"

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
#include "comm_port.h"
#endif
#if VERSION_FEATURE_WIFI
#include "esp8266_mqtt.h"
#endif

#include <string.h>

/* Global context */
app_context_t g_app;

/* Driver instances */
static const gas_sensor_t *pm25_sensor = NULL;
#if VERSION_FEATURE_MQ2
static const gas_sensor_t *mq2_sensor = NULL;
#endif
#if VERSION_FEATURE_MQ7
static const gas_sensor_t *mq7_sensor = NULL;
#endif
#if VERSION_FEATURE_DHT11
static const temp_hum_sensor_t *dht11_sensor = NULL;
#endif
static const relay_driver_t *relay_drv = NULL;
static const misc_driver_t *buzzer_drv = NULL;
static const misc_driver_t *led_drv = NULL;
static const display_driver_t *display = NULL;

/* Default threshold values */
#define DEFAULT_PM25_THRESHOLD      100
#define DEFAULT_TEMP_THRESHOLD      35
#define DEFAULT_HUMIDITY_THRESHOLD  80
#define DEFAULT_SMOKE_THRESHOLD     500
#define DEFAULT_CO_THRESHOLD        50

/* Threshold step sizes */
#define PM25_THRESHOLD_STEP     10
#define TEMP_THRESHOLD_STEP     1
#define HUMIDITY_THRESHOLD_STEP 5
#define SMOKE_THRESHOLD_STEP    50
#define CO_THRESHOLD_STEP       5

/* ============================================================================
 * Initialization
 * ============================================================================ */

void app_logic_init(void)
{
    memset(&g_app, 0, sizeof(g_app));

    /* Initialize default thresholds */
    g_app.pm25_threshold = DEFAULT_PM25_THRESHOLD;
    g_app.temp_threshold = DEFAULT_TEMP_THRESHOLD;
    g_app.humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
    g_app.smoke_threshold = DEFAULT_SMOKE_THRESHOLD;
    g_app.co_threshold = DEFAULT_CO_THRESHOLD;

    /* Start in auto mode */
    g_app.mode = MODE_AUTO;
    g_app.mode_changed = 1;

    /* Get sensor drivers */
    pm25_sensor = devmgr_get_gas_sensor("pm25");

#if VERSION_FEATURE_DHT11
    dht11_sensor = devmgr_get_temp_hum_sensor("dht11");
#endif

#if VERSION_FEATURE_MQ2
    mq2_sensor = devmgr_get_gas_sensor("mq2_smoke");
#endif

#if VERSION_FEATURE_MQ7
    mq7_sensor = devmgr_get_gas_sensor("mq7_co");
#endif

    /* Get actuator drivers */
    relay_drv = devmgr_get_relay("relay");

    buzzer_drv = devmgr_get_misc("buzzer");

    led_drv = devmgr_get_misc("led");

    /* Get display driver */
    display = devmgr_get_display("oled");
    if (display) {
        display->clear();
    }
}

/* ============================================================================
 * Sensor Reading Loop
 * ============================================================================ */

void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    uint8_t updated = 0;

    /* Read PM2.5 */
    if (pm25_sensor) {
        g_app.pm25_ppm = pm25_sensor->read_ppm();
        updated = 1;
    }

#if VERSION_FEATURE_DHT11
    /* Read temperature and humidity */
    if (dht11_sensor) {
        float temp = dht11_sensor->read_temperature();
        float hum = dht11_sensor->read_humidity();
        g_app.temperature = (int8_t)temp;
        g_app.humidity = (uint8_t)hum;
        updated = 1;
    }
#endif

#if VERSION_FEATURE_MQ2
    /* Read smoke */
    if (mq2_sensor) {
        g_app.smoke_ppm = mq2_sensor->read_ppm();
        updated = 1;
    }
#endif

#if VERSION_FEATURE_MQ7
    /* Read CO */
    if (mq7_sensor) {
        g_app.co_ppm = mq7_sensor->read_ppm();
        updated = 1;
    }
#endif

    if (updated) {
        g_app.sensor_updated = 1;
#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE
        app_logic_request_telemetry();
#endif
    }
}

/* ============================================================================
 * Alarm Control Loop
 * ============================================================================ */

void alarm_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    if (g_app.mode != MODE_AUTO) {
        /* In manual/threshold mode, don't override device states */
        return;
    }

    uint8_t alarm = 0;

    /* Check PM2.5 threshold */
    if (g_app.pm25_ppm > g_app.pm25_threshold) {
        alarm = 1;
    }

#if VERSION_FEATURE_DHT11
    /* Check temperature threshold */
    if (g_app.temperature > g_app.temp_threshold) {
        alarm = 1;
    }

    /* Check humidity threshold */
    if (g_app.humidity > g_app.humidity_threshold) {
        alarm = 1;
    }
#endif

#if VERSION_FEATURE_MQ2
    /* Check smoke threshold */
    if (g_app.smoke_ppm > g_app.smoke_threshold) {
        alarm = 1;
    }
#endif

#if VERSION_FEATURE_MQ7
    /* Check CO threshold */
    if (g_app.co_ppm > g_app.co_threshold) {
        alarm = 1;
    }
#endif

    g_app.alarm_active = alarm;

    /* Control devices based on alarm state */
    if (alarm) {
        /* Turn on fan, buzzer, and light when any threshold is exceeded */
        if (!g_app.fan_on) {
            g_app.fan_on = 1;
            if (relay_drv) relay_drv->set_state(1);
        }
        if (!g_app.buzzer_on) {
            g_app.buzzer_on = 1;
            if (buzzer_drv) buzzer_drv->set_state(1);
        }
        if (!g_app.light_on) {
            g_app.light_on = 1;
            if (led_drv) led_drv->set_state(1);
        }
    } else {
        /* Turn off all devices when no alarm */
        if (g_app.fan_on) {
            g_app.fan_on = 0;
            if (relay_drv) relay_drv->set_state(0);
        }
        if (g_app.buzzer_on) {
            g_app.buzzer_on = 0;
            if (buzzer_drv) buzzer_drv->set_state(0);
        }
        if (g_app.light_on) {
            g_app.light_on = 0;
            if (led_drv) led_drv->set_state(0);
        }
    }

    g_app.display_refresh = 1;
}

/* ============================================================================
 * Display Loop
 * ============================================================================ */

static void format_auto_screen(void)
{
    DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "AUTO PM2.5:%d", g_app.pm25_ppm);

#if VERSION_FEATURE_DHT11
    DISPLAY_PRINT(display, 0U, 1U, DISPLAY_FONT_SMALL, "T:%dC H:%d%%", g_app.temperature, g_app.humidity);
#else
    DISPLAY_PRINT(display, 0U, 1U, DISPLAY_FONT_SMALL, "PM2.5 Monitor");
#endif

#if VERSION_FEATURE_MQ2 && VERSION_FEATURE_MQ7
    DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "SM:%d CO:%d", g_app.smoke_ppm, g_app.co_ppm);
#elif VERSION_FEATURE_MQ2
    DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "Smoke:%d ppm", g_app.smoke_ppm);
#elif VERSION_FEATURE_MQ7
    DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "CO:%d ppm", g_app.co_ppm);
#else
    DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "Status:%s", g_app.alarm_active ? "ALARM" : "OK");
#endif

    DISPLAY_PRINT(display, 0U, 3U, DISPLAY_FONT_SMALL, "F:%s B:%s L:%s",
                  g_app.fan_on ? "ON" : "off",
                  g_app.buzzer_on ? "ON" : "off",
                  g_app.light_on ? "ON" : "off");
}

static void format_manual_screen(void)
{
    const char *device_names[] = {"Fan", "Buzzer", "Light"};
    uint8_t state = 0;

    switch (g_app.manual_selected_device) {
        case DEVICE_FAN: state = g_app.fan_on; break;
        case DEVICE_BUZZER: state = g_app.buzzer_on; break;
        case DEVICE_LIGHT: state = g_app.light_on; break;
    }

    DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "Mode:MANUAL");
    DISPLAY_PRINT(display, 0U, 1U, DISPLAY_FONT_SMALL, ">%s", device_names[g_app.manual_selected_device]);
    DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "State: %s", state ? "ON" : "OFF");
    DISPLAY_PRINT(display, 0U, 3U, DISPLAY_FONT_SMALL, "K2:Sel K3:On K4:Off");
}

static void format_threshold_screen(void)
{
    const char *thresh_names[] = {"PM2.5", "Temp", "Humid", "Smoke", "CO"};
    uint8_t max_thresh = THRESH_PM25 + 1;
    uint16_t value = 0;

#if VERSION_FEATURE_DHT11
    max_thresh = THRESH_HUMIDITY + 1;
#endif
#if VERSION_FEATURE_MQ2
    max_thresh = THRESH_SMOKE + 1;
#endif
#if VERSION_FEATURE_MQ7
    max_thresh = THRESH_CO + 1;
#endif

    if (g_app.thresh_selected_item >= max_thresh) {
        g_app.thresh_selected_item = 0;
    }

    switch (g_app.thresh_selected_item) {
        case THRESH_PM25: value = g_app.pm25_threshold; break;
        case THRESH_TEMP: value = (uint16_t)g_app.temp_threshold; break;
        case THRESH_HUMIDITY: value = g_app.humidity_threshold; break;
        case THRESH_SMOKE: value = g_app.smoke_threshold; break;
        case THRESH_CO: value = g_app.co_threshold; break;
    }

    DISPLAY_PRINT(display, 0U, 0U, DISPLAY_FONT_SMALL, "Mode:THRESHOLD");
    DISPLAY_PRINT(display, 0U, 1U, DISPLAY_FONT_SMALL, ">%s", thresh_names[g_app.thresh_selected_item]);
    DISPLAY_PRINT(display, 0U, 2U, DISPLAY_FONT_SMALL, "Value: %d", value);
    DISPLAY_PRINT(display, 0U, 3U, DISPLAY_FONT_SMALL, "K2:Sel K3:+ K4:-");
}

void display_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;

    if (!g_app.display_refresh && !g_app.mode_changed && !g_app.sensor_updated) {
        return;
    }

    if (!display) {
        return;
    }

    g_app.display_refresh = 0;
    g_app.mode_changed = 0;
    g_app.sensor_updated = 0;

    display->clear();
    switch (g_app.mode) {
        case MODE_AUTO:
            format_auto_screen();
            break;
        case MODE_MANUAL:
            format_manual_screen();
            break;
        case MODE_THRESHOLD:
            format_threshold_screen();
            break;
    }
    display->update();
}

/* ============================================================================
 * Key Event Handlers
 * ============================================================================ */

void app_logic_on_key1_press(void)
{
    /* Mode switch: AUTO -> MANUAL -> THRESHOLD -> AUTO */
    switch (g_app.mode) {
        case MODE_AUTO:
            g_app.mode = MODE_MANUAL;
            g_app.manual_selected_device = 0;
            break;
        case MODE_MANUAL:
            g_app.mode = MODE_THRESHOLD;
            g_app.thresh_selected_item = 0;
            break;
        case MODE_THRESHOLD:
            g_app.mode = MODE_AUTO;
            break;
    }
    g_app.mode_changed = 1;
}

void app_logic_on_key2_press(void)
{
    /* Select function based on current mode */
    switch (g_app.mode) {
        case MODE_MANUAL:
            /* Cycle through devices: Fan -> Buzzer -> Light -> Fan */
            g_app.manual_selected_device++;
            if (g_app.manual_selected_device >= DEVICE_COUNT) {
                g_app.manual_selected_device = 0;
            }
            break;
            
        case MODE_THRESHOLD:
            /* Cycle through available thresholds */
            g_app.thresh_selected_item++;
#if !VERSION_FEATURE_DHT11
            if (g_app.thresh_selected_item == THRESH_TEMP || 
                g_app.thresh_selected_item == THRESH_HUMIDITY) {
                g_app.thresh_selected_item = THRESH_PM25;
            }
#endif
#if !VERSION_FEATURE_MQ2
            if (g_app.thresh_selected_item == THRESH_SMOKE) {
                g_app.thresh_selected_item = THRESH_PM25;
            }
#endif
#if !VERSION_FEATURE_MQ7
            if (g_app.thresh_selected_item == THRESH_CO) {
                g_app.thresh_selected_item = THRESH_PM25;
            }
#endif
            if (g_app.thresh_selected_item >= THRESH_COUNT) {
                g_app.thresh_selected_item = 0;
            }
            break;
            
        default:
            break;
    }
    g_app.display_refresh = 1;
}

void app_logic_on_key3_press(void)
{
    /* Add/On function based on current mode */
    switch (g_app.mode) {
        case MODE_MANUAL:
            /* Turn on selected device */
            switch (g_app.manual_selected_device) {
                case DEVICE_FAN:
                    g_app.fan_on = 1;
                    if (relay_drv) relay_drv->set_state(1);
                    break;
                case DEVICE_BUZZER:
                    g_app.buzzer_on = 1;
                    if (buzzer_drv) buzzer_drv->set_state(1);
                    break;
                case DEVICE_LIGHT:
                    g_app.light_on = 1;
                    if (led_drv) led_drv->set_state(1);
                    break;
            }
            break;
            
        case MODE_THRESHOLD:
            /* Increment selected threshold */
            switch (g_app.thresh_selected_item) {
                case THRESH_PM25:
                    g_app.pm25_threshold += PM25_THRESHOLD_STEP;
                    break;
                case THRESH_TEMP:
                    g_app.temp_threshold += TEMP_THRESHOLD_STEP;
                    break;
                case THRESH_HUMIDITY:
                    g_app.humidity_threshold += HUMIDITY_THRESHOLD_STEP;
                    break;
                case THRESH_SMOKE:
                    g_app.smoke_threshold += SMOKE_THRESHOLD_STEP;
                    break;
                case THRESH_CO:
                    g_app.co_threshold += CO_THRESHOLD_STEP;
                    break;
            }
            break;
            
        default:
            break;
    }
    g_app.display_refresh = 1;
}

void app_logic_on_key4_press(void)
{
    /* Sub/Off function based on current mode */
    switch (g_app.mode) {
        case MODE_MANUAL:
            /* Turn off selected device */
            switch (g_app.manual_selected_device) {
                case DEVICE_FAN:
                    g_app.fan_on = 0;
                    if (relay_drv) relay_drv->set_state(0);
                    break;
                case DEVICE_BUZZER:
                    g_app.buzzer_on = 0;
                    if (buzzer_drv) buzzer_drv->set_state(0);
                    break;
                case DEVICE_LIGHT:
                    g_app.light_on = 0;
                    if (led_drv) led_drv->set_state(0);
                    break;
            }
            break;
            
        case MODE_THRESHOLD:
            /* Decrement selected threshold */
            switch (g_app.thresh_selected_item) {
                case THRESH_PM25:
                    if (g_app.pm25_threshold >= PM25_THRESHOLD_STEP)
                        g_app.pm25_threshold -= PM25_THRESHOLD_STEP;
                    break;
                case THRESH_TEMP:
                    if (g_app.temp_threshold > -40)
                        g_app.temp_threshold -= TEMP_THRESHOLD_STEP;
                    break;
                case THRESH_HUMIDITY:
                    if (g_app.humidity_threshold >= HUMIDITY_THRESHOLD_STEP)
                        g_app.humidity_threshold -= HUMIDITY_THRESHOLD_STEP;
                    break;
                case THRESH_SMOKE:
                    if (g_app.smoke_threshold >= SMOKE_THRESHOLD_STEP)
                        g_app.smoke_threshold -= SMOKE_THRESHOLD_STEP;
                    break;
                case THRESH_CO:
                    if (g_app.co_threshold >= CO_THRESHOLD_STEP)
                        g_app.co_threshold -= CO_THRESHOLD_STEP;
                    break;
            }
            break;
            
        default:
            break;
    }
    g_app.display_refresh = 1;
}

/* ============================================================================
 * Communication Loop (WiFi/BLE versions only)
 * ============================================================================ */

#if VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE

#include "cJSON.h"
#include "cjson_port.h"

#define APP_TELEMETRY_BUFFER_SIZE 384U

static void app_logic_publish_telemetry(void)
{
    char telemetry[APP_TELEMETRY_BUFFER_SIZE];
    app_logic_build_telemetry(telemetry, sizeof(telemetry));

#if VERSION_FEATURE_WIFI
    if (esp8266_mqtt_is_ready() != 0) {
        (void)esp8266_mqtt_publish_json(BOARD_ESP8266_MQTT_PUB_TOPIC, telemetry);
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
    g_app.telemetry_pending = 1U;
    event_set(APP_EVENT_COMM_TX);
}

void comm_loop_run(sched_event_t events, void *ctx)
{
#if VERSION_FEATURE_BLE
    unsigned char rx;
#endif

    (void)ctx;

#if VERSION_FEATURE_WIFI
    if ((events & (APP_EVENT_COMM_RX | APP_EVENT_TICK)) != 0U) {
        esp8266_mqtt_poll();
    }
#endif

#if VERSION_FEATURE_BLE
    if ((events & APP_EVENT_COMM_RX) != 0U) {
        while (comm_port_recv(&rx, 1U) > 0) {
        }
    }
#endif

    if (((events & (APP_EVENT_COMM_TX | APP_EVENT_TICK)) != 0U) && (g_app.telemetry_pending != 0U)) {
        g_app.telemetry_pending = 0U;
        app_logic_publish_telemetry();
    }
}

void app_logic_build_telemetry(char *buf, size_t bufsize)
{
    cJSON *root;
    cJSON *thresholds;
    char *json_str;
    const char *mode_str;

    if ((buf == 0) || (bufsize == 0U)) {
        return;
    }
    buf[0] = '\0';

    root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "version", VERSION_NAME);
    cJSON_AddNumberToObject(root, "version_no", APP_VERSION);
    cJSON_AddNumberToObject(root, "pm25", g_app.pm25_ppm);

#if VERSION_FEATURE_DHT11
    cJSON_AddNumberToObject(root, "temp", g_app.temperature);
    cJSON_AddNumberToObject(root, "humidity", g_app.humidity);
#endif

#if VERSION_FEATURE_MQ2
    cJSON_AddNumberToObject(root, "smoke", g_app.smoke_ppm);
#endif

#if VERSION_FEATURE_MQ7
    cJSON_AddNumberToObject(root, "co", g_app.co_ppm);
#endif

    cJSON_AddNumberToObject(root, "fan", g_app.fan_on);
    cJSON_AddNumberToObject(root, "buzzer", g_app.buzzer_on);
    cJSON_AddNumberToObject(root, "light", g_app.light_on);
    cJSON_AddNumberToObject(root, "alarm", g_app.alarm_active);

    mode_str = (g_app.mode == MODE_AUTO) ? "auto" :
               (g_app.mode == MODE_MANUAL) ? "manual" : "threshold";
    cJSON_AddStringToObject(root, "mode", mode_str);

    thresholds = cJSON_CreateObject();
    if (thresholds != 0) {
        cJSON_AddNumberToObject(thresholds, "pm25", g_app.pm25_threshold);
#if VERSION_FEATURE_DHT11
        cJSON_AddNumberToObject(thresholds, "temp", g_app.temp_threshold);
        cJSON_AddNumberToObject(thresholds, "humidity", g_app.humidity_threshold);
#endif
#if VERSION_FEATURE_MQ2
        cJSON_AddNumberToObject(thresholds, "smoke", g_app.smoke_threshold);
#endif
#if VERSION_FEATURE_MQ7
        cJSON_AddNumberToObject(thresholds, "co", g_app.co_threshold);
#endif
        cJSON_AddItemToObject(root, "thresholds", thresholds);
    }

    json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        strncpy(buf, json_str, bufsize - 1U);
        buf[bufsize - 1U] = '\0';
        cJSON_free(json_str);
    }

    cJSON_Delete(root);
}

void app_logic_on_mqtt_rx(const char *json_data)
{
    cJSON *root = cjson_parse(json_data, strlen(json_data));
    if (!root) return;

    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (!cmd_item || !cJSON_IsString(cmd_item)) {
        cjson_release(root);
        return;
    }

    const char *cmd = cmd_item->valuestring;

    if (strcmp(cmd, "set_mode") == 0) {
        cJSON *mode_item = cJSON_GetObjectItem(root, "mode");
        if (mode_item && cJSON_IsString(mode_item)) {
            const char *mode = mode_item->valuestring;
            if (strcmp(mode, "auto") == 0) g_app.mode = MODE_AUTO;
            else if (strcmp(mode, "manual") == 0) g_app.mode = MODE_MANUAL;
            else if (strcmp(mode, "threshold") == 0) g_app.mode = MODE_THRESHOLD;
            g_app.mode_changed = 1;
            g_app.display_refresh = 1;
        }
    }
    else if (strcmp(cmd, "set_device") == 0) {
        cJSON *dev_item = cJSON_GetObjectItem(root, "device");
        cJSON *state_item = cJSON_GetObjectItem(root, "state");
        if (dev_item && cJSON_IsString(dev_item) && state_item && ((state_item->type & cJSON_Number) != 0)) {
            uint8_t state = state_item->valueint ? 1 : 0;
            const char *device = dev_item->valuestring;

            if (strcmp(device, "fan") == 0) {
                g_app.fan_on = state;
                if (relay_drv) relay_drv->set_state(state);
            }
            else if (strcmp(device, "buzzer") == 0) {
                g_app.buzzer_on = state;
                if (buzzer_drv) buzzer_drv->set_state(state);
            }
            else if (strcmp(device, "light") == 0) {
                g_app.light_on = state;
                if (led_drv) led_drv->set_state(state);
            }
            g_app.display_refresh = 1;
        }
    }
    else if (strcmp(cmd, "set_threshold") == 0) {
        cJSON *sensor_item = cJSON_GetObjectItem(root, "sensor");
        cJSON *value_item = cJSON_GetObjectItem(root, "value");
        if (sensor_item && cJSON_IsString(sensor_item) && value_item && ((value_item->type & cJSON_Number) != 0)) {
            const char *sensor = sensor_item->valuestring;
            int value = value_item->valueint;

            if (strcmp(sensor, "pm25") == 0) g_app.pm25_threshold = (uint16_t)value;
            else if (strcmp(sensor, "temp") == 0) g_app.temp_threshold = (int8_t)value;
            else if (strcmp(sensor, "humidity") == 0) g_app.humidity_threshold = (uint8_t)value;
            else if (strcmp(sensor, "smoke") == 0) g_app.smoke_threshold = (uint16_t)value;
            else if (strcmp(sensor, "co") == 0) g_app.co_threshold = (uint16_t)value;
            g_app.display_refresh = 1;
        }
    }
    else if (strcmp(cmd, "get_status") == 0) {
        g_app.display_refresh = 1;
    }

    app_logic_request_telemetry();
    cjson_release(root);
}

#else

void comm_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
}

void app_logic_build_telemetry(char *buf, size_t bufsize)
{
    (void)buf;
    (void)bufsize;
}

void app_logic_on_mqtt_rx(const char *json_data)
{
    (void)json_data;
}

void app_logic_request_telemetry(void)
{
}

#endif /* VERSION_FEATURE_WIFI || VERSION_FEATURE_BLE */
