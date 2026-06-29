/* ZHJS_001 智慧照明业务逻辑：光照采样、人体检测、灯路控制、显示和按键处理。 */
#include "app_logic.h"
#include "devmgr.h"
#include "version_config.h"
#include "analog_probe_if.h"
#include "actuator_if.h"
#include "display_if.h"

#define APP_MODE_AUTO 0U
#define APP_MODE_MANUAL 1U

typedef struct {
    unsigned char mode;
    unsigned char selected;
    unsigned char light_on[APP_LIGHT_CHANNEL_COUNT];
    unsigned char light_gear[APP_LIGHT_CHANNEL_COUNT];
    float light_percent;
    unsigned char person_present;
    unsigned char display_dirty;
    unsigned long leave_deadline_tick;
} app_context_t;

static app_context_t app_ctx;
static const analog_probe_t *app_gl5506;
static const analog_probe_t *app_presence;
static const relay_driver_t *app_lights[APP_LIGHT_CHANNEL_COUNT];
static const display_driver_t *app_display;

static const char *app_light_names[APP_LIGHT_CHANNEL_COUNT] = {
    "light1",
    "light2",
    "light3"
};

static unsigned char app_read_person(void)
{
    float v;

    if (app_presence == 0) {
        return 0U;
    }

    v = app_presence->read_value();
    return (v >= 0.5f) ? 1U : 0U;
}

static void app_read_light_percent(void)
{
    if (app_gl5506 != 0) {
        app_ctx.light_percent = app_gl5506->read_value();
    }
}

static unsigned char app_auto_gear_from_light(float percent)
{
    if (percent >= 85.0f) {
        return 0U;
    }
    if (percent > 55.0f) {
        return 1U;
    }
    if (percent > 25.0f) {
        return 2U;
    }

    return 3U;
}

static void app_apply_channel_hw(unsigned char index)
{
    unsigned char on;
    unsigned char gear;

    if ((index >= APP_LIGHT_CHANNEL_COUNT) || (app_lights[index] == 0)) {
        return;
    }

    on = app_ctx.light_on[index];
    gear = app_ctx.light_gear[index];
    if ((on == 0U) || (gear == 0U)) {
        app_lights[index]->set_state(0U);
    } else {
        app_lights[index]->set_state(1U);
    }
}

static void app_apply_all_hw(void)
{
    unsigned char i;

    for (i = 0U; i < APP_LIGHT_CHANNEL_COUNT; i++) {
        app_apply_channel_hw(i);
    }
}

static void app_set_channel(unsigned char index, unsigned char on, unsigned char gear)
{
    if (index >= APP_LIGHT_CHANNEL_COUNT) {
        return;
    }

    app_ctx.light_on[index] = on;
    app_ctx.light_gear[index] = gear;
    app_apply_channel_hw(index);
    app_ctx.display_dirty = 1U;
}

static void app_auto_apply_all(unsigned char gear)
{
    unsigned char i;
    unsigned char on;

    on = (gear > 0U) ? 1U : 0U;
    for (i = 0U; i < APP_LIGHT_CHANNEL_COUNT; i++) {
        app_ctx.light_on[i] = on;
        app_ctx.light_gear[i] = gear;
        app_apply_channel_hw(i);
    }
    app_ctx.display_dirty = 1U;
}

static void app_auto_all_off(void)
{
    app_auto_apply_all(0U);
}

static void app_auto_tick(void)
{
    unsigned char person;
    unsigned char gear;
    uint32_t now;

    app_read_light_percent();
    person = app_read_person();
    app_ctx.person_present = person;
    now = sched_tick_get();

    if (person != 0U) {
        app_ctx.leave_deadline_tick = 0U;
        gear = app_auto_gear_from_light(app_ctx.light_percent);
        app_auto_apply_all(gear);
        return;
    }

    if (app_ctx.leave_deadline_tick == 0U) {
        app_ctx.leave_deadline_tick = now + APP_AUTO_LEAVE_DELAY_MS;
    }

    if (now >= app_ctx.leave_deadline_tick) {
        app_auto_all_off();
    }

    app_ctx.display_dirty = 1U;
}

static void app_display_refresh(void)
{
    unsigned char i;
    unsigned char sel;
    const char *mode_text;
    const char *person_text;

    if (app_display == 0) {
        return;
    }

    mode_text = (app_ctx.mode == APP_MODE_AUTO) ? "AUTO" : "MAN";
    person_text = (app_ctx.person_present != 0U) ? "YES" : "NO ";

    app_display->clear();
    app_display->print(0U, 0U, DISPLAY_FONT_SMALL, "Mode:%s", mode_text);
    app_display->print(0U, 2U, DISPLAY_FONT_SMALL, "Lux:%3u%%",
                       (unsigned int)(app_ctx.light_percent + 0.5f));
    app_display->print(0U, 4U, DISPLAY_FONT_SMALL, "Person:%s", person_text);

    if (app_ctx.mode == APP_MODE_MANUAL) {
        sel = app_ctx.selected;
        if (sel < APP_LIGHT_CHANNEL_COUNT) {
            app_display->print(0U, 6U, DISPLAY_FONT_SMALL, "L%u %s G%u",
                               (unsigned int)(sel + 1U),
                               app_ctx.light_on[sel] != 0U ? "ON " : "OFF",
                               (unsigned int)app_ctx.light_gear[sel]);
        }
    } else {
        for (i = 0U; i < APP_LIGHT_CHANNEL_COUNT; i++) {
            if (app_ctx.light_on[i] != 0U) {
                break;
            }
        }
        if (i >= APP_LIGHT_CHANNEL_COUNT) {
            app_display->print(0U, 6U, DISPLAY_FONT_SMALL, "Light:OFF");
        } else {
            app_display->print(0U, 6U, DISPLAY_FONT_SMALL, "Gear:%u",
                               (unsigned int)app_ctx.light_gear[0]);
        }
    }

    app_display->update();
}

void app_logic_on_key(unsigned char key_id)
{
    unsigned char sel;

    if (key_id == 1U) {
        app_ctx.mode = (app_ctx.mode == APP_MODE_AUTO) ? APP_MODE_MANUAL : APP_MODE_AUTO;
        app_ctx.leave_deadline_tick = 0U;
        if (app_ctx.mode == APP_MODE_AUTO) {
            app_auto_tick();
        } else {
            app_apply_all_hw();
        }
        app_ctx.display_dirty = 1U;
        return;
    }

    if (app_ctx.mode != APP_MODE_MANUAL) {
        return;
    }

    sel = app_ctx.selected;

    if (key_id == 2U) {
        app_ctx.selected = (unsigned char)((sel + 1U) % APP_LIGHT_CHANNEL_COUNT);
        app_ctx.display_dirty = 1U;
        return;
    }

    if (key_id == 3U) {
        if (sel < APP_LIGHT_CHANNEL_COUNT) {
            unsigned char on = (app_ctx.light_on[sel] == 0U) ? 1U : 0U;
            unsigned char gear = app_ctx.light_gear[sel];

            if ((on != 0U) && (gear == 0U)) {
                gear = 1U;
            }
            if (on == 0U) {
                gear = 0U;
            }
            app_set_channel(sel, on, gear);
        }
        return;
    }

    if (key_id == 4U) {
        if (sel < APP_LIGHT_CHANNEL_COUNT) {
            unsigned char gear = app_ctx.light_gear[sel];
            unsigned char on = app_ctx.light_on[sel];

            gear = (unsigned char)((gear + 1U) % 4U);
            if (gear == 0U) {
                on = 0U;
            } else {
                on = 1U;
            }
            app_set_channel(sel, on, gear);
        }
    }
}

void app_logic_on_sensor_tick(void)
{
    app_read_light_percent();
    app_ctx.person_present = app_read_person();

    if (app_ctx.mode == APP_MODE_AUTO) {
        app_auto_tick();
    } else {
        app_ctx.display_dirty = 1U;
    }
}

void app_logic_init(void)
{
    unsigned char i;

    app_gl5506 = devmgr_get_analog_probe("gl5506");
    app_presence = devmgr_get_analog_probe("presence");
    app_display = devmgr_get_display("oled");

    for (i = 0U; i < APP_LIGHT_CHANNEL_COUNT; i++) {
        app_lights[i] = devmgr_get_relay(app_light_names[i]);
    }

    app_ctx.mode = APP_MODE_AUTO;
    app_ctx.selected = 0U;
    app_ctx.leave_deadline_tick = 0U;
    app_ctx.display_dirty = 1U;

    for (i = 0U; i < APP_LIGHT_CHANNEL_COUNT; i++) {
        app_ctx.light_on[i] = 0U;
        app_ctx.light_gear[i] = 0U;
    }

    app_read_light_percent();
    app_ctx.person_present = app_read_person();
    app_auto_all_off();
    app_display_refresh();
}

void app_logic_loop(sched_event_t events)
{
    if ((events & APP_EVENT_KEY) != 0U) {
        app_ctx.display_dirty = 1U;
    }

    if ((events & APP_EVENT_SENSOR) != 0U) {
        app_ctx.display_dirty = 1U;
    }

    if (app_ctx.display_dirty != 0U) {
        app_display_refresh();
        app_ctx.display_dirty = 0U;
    }
}
