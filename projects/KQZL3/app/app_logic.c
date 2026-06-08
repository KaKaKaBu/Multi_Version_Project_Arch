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
#include "tiny_printf.h"

#include <string.h>

/* Version feature flags based on KQZL3_VERSION */
#if KQZL3_VERSION >= 2
#define KQZL3_HAS_DHT11     1
#else
#define KQZL3_HAS_DHT11     0
#endif

#if KQZL3_VERSION >= 5
#define KQZL3_HAS_MQ2       1
#else
#define KQZL3_HAS_MQ2       0
#endif

#if KQZL3_VERSION >= 11
#define KQZL3_HAS_MQ7       1
#else
#define KQZL3_HAS_MQ7       0
#endif

#if (KQZL3_VERSION == 3) || (KQZL3_VERSION == 6) || (KQZL3_VERSION == 8) || \
    (KQZL3_VERSION == 9) || (KQZL3_VERSION == 10) || (KQZL3_VERSION == 12) || (KQZL3_VERSION == 14)
#define KQZL3_HAS_WIFI      1
#else
#define KQZL3_HAS_WIFI      0
#endif

#if (KQZL3_VERSION == 4) || (KQZL3_VERSION == 7) || (KQZL3_VERSION == 13)
#define KQZL3_HAS_BLE       1
#else
#define KQZL3_HAS_BLE       0
#endif

/* Global context */
app_context_t g_app;

/* Driver instances */
static const gas_sensor_t *pm25_sensor = NULL;
static const gas_sensor_t *mq2_sensor = NULL;
static const gas_sensor_t *mq7_sensor = NULL;
static const temp_hum_sensor_t *dht11_sensor = NULL;
static const relay_driver_t *relay_drv = NULL;
static const misc_driver_t *buzzer_drv = NULL;
static const misc_driver_t *led_drv = NULL;
static const display_driver_t *display = NULL;

/* Display line buffer */
static char line_buf[4][17];  /* 4 lines x 16 chars + null */

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
    if (pm25_sensor) {
        pm25_sensor->init();
    }

#if KQZL3_HAS_DHT11
    dht11_sensor = devmgr_get_temp_hum_sensor("dht11");
    if (dht11_sensor) {
        dht11_sensor->init();
    }
#endif

#if KQZL3_HAS_MQ2
    mq2_sensor = devmgr_get_gas_sensor("mq2_smoke");
    if (mq2_sensor) {
        mq2_sensor->init();
    }
#endif

#if KQZL3_HAS_MQ7
    mq7_sensor = devmgr_get_gas_sensor("mq7_co");
    if (mq7_sensor) {
        mq7_sensor->init();
    }
#endif

    /* Get actuator drivers */
    relay_drv = devmgr_get_relay("relay");
    if (relay_drv) {
        relay_drv->init();
    }

    buzzer_drv = devmgr_get_misc("buzzer");
    if (buzzer_drv) {
        buzzer_drv->init();
    }

    led_drv = devmgr_get_misc("led");
    if (led_drv) {
        led_drv->init();
    }

    /* Get display driver */
    display = devmgr_get_display("oled");
    if (display) {
        display->init();
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

#if KQZL3_HAS_DHT11
    /* Read temperature and humidity */
    if (dht11_sensor) {
        float temp = dht11_sensor->read_temperature();
        float hum = dht11_sensor->read_humidity();
        g_app.temperature = (int8_t)temp;
        g_app.humidity = (uint8_t)hum;
        updated = 1;
    }
#endif

#if KQZL3_HAS_MQ2
    /* Read smoke */
    if (mq2_sensor) {
        g_app.smoke_ppm = mq2_sensor->read_ppm();
        updated = 1;
    }
#endif

#if KQZL3_HAS_MQ7
    /* Read CO */
    if (mq7_sensor) {
        g_app.co_ppm = mq7_sensor->read_ppm();
        updated = 1;
    }
#endif

    if (updated) {
        g_app.sensor_updated = 1;
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

#if KQZL3_HAS_DHT11
    /* Check temperature threshold */
    if (g_app.temperature > g_app.temp_threshold) {
        alarm = 1;
    }

    /* Check humidity threshold */
    if (g_app.humidity > g_app.humidity_threshold) {
        alarm = 1;
    }
#endif

#if KQZL3_HAS_MQ2
    /* Check smoke threshold */
    if (g_app.smoke_ppm > g_app.smoke_threshold) {
        alarm = 1;
    }
#endif

#if KQZL3_HAS_MQ7
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
    /* Line 1: Mode and PM2.5 */
    tiny_snprintf(line_buf[0], 17, "Mode:AUTO PM2.5:%d", g_app.pm25_ppm);

#if KQZL3_HAS_DHT11
    /* Line 2: Temperature and Humidity */
    tiny_snprintf(line_buf[1], 17, "T:%dC H:%d%%", 
                  g_app.temperature, g_app.humidity);
#else
    tiny_snprintf(line_buf[1], 17, "PM2.5 Monitor");
#endif

#if KQZL3_HAS_MQ2 && KQZL3_HAS_MQ7
    /* Line 3: Smoke and CO */
    tiny_snprintf(line_buf[2], 17, "SM:%d CO:%d", 
                  g_app.smoke_ppm, g_app.co_ppm);
#elif KQZL3_HAS_MQ2
    tiny_snprintf(line_buf[2], 17, "Smoke:%d ppm", g_app.smoke_ppm);
#elif KQZL3_HAS_MQ7
    tiny_snprintf(line_buf[2], 17, "CO:%d ppm", g_app.co_ppm);
#else
    tiny_snprintf(line_buf[2], 17, "Status:%s", 
                  g_app.alarm_active ? "ALARM" : "OK");
#endif

    /* Line 4: Device states */
    tiny_snprintf(line_buf[3], 17, "F:%s B:%s L:%s",
                  g_app.fan_on ? "ON" : "off",
                  g_app.buzzer_on ? "ON" : "off",
                  g_app.light_on ? "ON" : "off");
}

static void format_manual_screen(void)
{
    const char *device_names[] = {"Fan", "Buzzer", "Light"};

    /* Line 1: Mode and selected device */
    tiny_snprintf(line_buf[0], 17, "Mode:MANUAL");
    
    /* Line 2: Selected device indicator */
    tiny_snprintf(line_buf[1], 17, ">%s", device_names[g_app.manual_selected_device]);
    
    /* Line 3: Current device state */
    uint8_t state = 0;
    switch (g_app.manual_selected_device) {
        case DEVICE_FAN: state = g_app.fan_on; break;
        case DEVICE_BUZZER: state = g_app.buzzer_on; break;
        case DEVICE_LIGHT: state = g_app.light_on; break;
    }
    tiny_snprintf(line_buf[2], 17, "State: %s", state ? "ON" : "OFF");
    
    /* Line 4: Key hints */
    tiny_snprintf(line_buf[3], 17, "K2:Sel K3:On K4:Off");
}

static void format_threshold_screen(void)
{
    const char *thresh_names[] = {"PM2.5", "Temp", "Humid", "Smoke", "CO"};
    
    /* Determine which thresholds are available */
    uint8_t max_thresh = THRESH_PM25 + 1;
#if KQZL3_HAS_DHT11
    max_thresh = THRESH_HUMIDITY + 1;
#endif
#if KQZL3_HAS_MQ2
    max_thresh = THRESH_SMOKE + 1;
#endif
#if KQZL3_HAS_MQ7
    max_thresh = THRESH_CO + 1;
#endif

    /* Clamp selection to available thresholds */
    if (g_app.thresh_selected_item >= max_thresh) {
        g_app.thresh_selected_item = 0;
    }

    /* Line 1: Mode */
    tiny_snprintf(line_buf[0], 17, "Mode:THRESHOLD");
    
    /* Line 2: Selected threshold */
    tiny_snprintf(line_buf[1], 17, ">%s", thresh_names[g_app.thresh_selected_item]);
    
    /* Line 3: Current threshold value */
    uint16_t value = 0;
    switch (g_app.thresh_selected_item) {
        case THRESH_PM25: value = g_app.pm25_threshold; break;
        case THRESH_TEMP: value = (uint16_t)g_app.temp_threshold; break;
        case THRESH_HUMIDITY: value = g_app.humidity_threshold; break;
        case THRESH_SMOKE: value = g_app.smoke_threshold; break;
        case THRESH_CO: value = g_app.co_threshold; break;
    }
    tiny_snprintf(line_buf[2], 17, "Value: %d", value);
    
    /* Line 4: Key hints */
    tiny_snprintf(line_buf[3], 17, "K2:Sel K3:+ K4:-");
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

    /* Clear flags */
    g_app.display_refresh = 0;
    g_app.mode_changed = 0;
    g_app.sensor_updated = 0;

    /* Format screen based on mode */
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

    /* Update display */
    display->clear();
    display->print(0, 0, 1, line_buf[0]);
    display->print(0, 1, 1, line_buf[1]);
    display->print(0, 2, 1, line_buf[2]);
    display->print(0, 3, 1, line_buf[3]);
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
#if !KQZL3_HAS_DHT11
            if (g_app.thresh_selected_item == THRESH_TEMP || 
                g_app.thresh_selected_item == THRESH_HUMIDITY) {
                g_app.thresh_selected_item = THRESH_PM25;
            }
#endif
#if !KQZL3_HAS_MQ2
            if (g_app.thresh_selected_item == THRESH_SMOKE) {
                g_app.thresh_selected_item = THRESH_PM25;
            }
#endif
#if !KQZL3_HAS_MQ7
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

#if KQZL3_HAS_WIFI || KQZL3_HAS_BLE

#include "cJSON.h"
#include "cjson_port.h"

void comm_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    /* Poll for incoming MQTT/BLE data */
    /* This would integrate with esp8266_mqtt_poll or jdy31_ble_poll */
}

void app_logic_build_telemetry(char *buf, size_t bufsize)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddNumberToObject(root, "pm25", g_app.pm25_ppm);
    
#if KQZL3_HAS_DHT11
    cJSON_AddNumberToObject(root, "temp", g_app.temperature);
    cJSON_AddNumberToObject(root, "humidity", g_app.humidity);
#endif

#if KQZL3_HAS_MQ2
    cJSON_AddNumberToObject(root, "smoke", g_app.smoke_ppm);
#endif

#if KQZL3_HAS_MQ7
    cJSON_AddNumberToObject(root, "co", g_app.co_ppm);
#endif

    cJSON_AddNumberToObject(root, "fan", g_app.fan_on);
    cJSON_AddNumberToObject(root, "buzzer", g_app.buzzer_on);
    cJSON_AddNumberToObject(root, "light", g_app.light_on);
    
    const char *mode_str = (g_app.mode == MODE_AUTO) ? "auto" : 
                           (g_app.mode == MODE_MANUAL) ? "manual" : "threshold";
    cJSON_AddStringToObject(root, "mode", mode_str);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        strncpy(buf, json_str, bufsize - 1);
        buf[bufsize - 1] = '\0';
        cJSON_free(json_str);
    }

    cJSON_Delete(root);
}

void app_logic_on_mqtt_rx(const char *json_data)
{
    cJSON *root = cJSON_Parse(json_data);
    if (!root) return;

    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (!cmd_item || !cJSON_IsString(cmd_item)) {
        cJSON_Delete(root);
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
        }
    }
    else if (strcmp(cmd, "set_device") == 0) {
        cJSON *dev_item = cJSON_GetObjectItem(root, "device");
        cJSON *state_item = cJSON_GetObjectItem(root, "state");
        if (dev_item && cJSON_IsString(dev_item) && state_item && cJSON_IsNumber(state_item)) {
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
        if (sensor_item && cJSON_IsString(sensor_item) && value_item && cJSON_IsNumber(value_item)) {
            const char *sensor = sensor_item->valuestring;
            int value = value_item->valueint;
            
            if (strcmp(sensor, "pm25") == 0) g_app.pm25_threshold = value;
            else if (strcmp(sensor, "temp") == 0) g_app.temp_threshold = value;
            else if (strcmp(sensor, "humidity") == 0) g_app.humidity_threshold = value;
            else if (strcmp(sensor, "smoke") == 0) g_app.smoke_threshold = value;
            else if (strcmp(sensor, "co") == 0) g_app.co_threshold = value;
            g_app.display_refresh = 1;
        }
    }
    else if (strcmp(cmd, "get_status") == 0) {
        /* Request to send telemetry - handled by comm_loop */
        g_app.display_refresh = 1;
    }

    cJSON_Delete(root);
}

#else

void app_logic_build_telemetry(char *buf, size_t bufsize)
{
    (void)buf;
    (void)bufsize;
}

void app_logic_on_mqtt_rx(const char *json_data)
{
    (void)json_data;
}

#endif /* KQZL3_HAS_WIFI || KQZL3_HAS_BLE */
