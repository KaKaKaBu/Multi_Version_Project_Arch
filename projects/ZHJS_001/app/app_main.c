#include "devmgr.h"
#include "hal_common.h"
#include "input_if.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"

#define APP_KEY_DEBOUNCE_MS 50U

static void app_logic_run(sched_event_t events, void *ctx)
{
    (void)ctx;
    app_logic_loop(events);
}

static void key_loop_run(sched_event_t events, void *ctx)
{
    const input_driver_t *keys = devmgr_get_input("key");
    static unsigned char last_stable_key;
    static unsigned char pending_key;
    static uint32_t debounce_start_tick;
    unsigned char raw_key;

    (void)events;
    (void)ctx;

    if (keys == 0) {
        return;
    }

    raw_key = keys->read_key();

    if (raw_key != pending_key) {
        pending_key = raw_key;
        debounce_start_tick = sched_tick_get();
    } else if ((raw_key != 0U) &&
               ((sched_tick_get() - debounce_start_tick) >= APP_KEY_DEBOUNCE_MS) &&
               (raw_key != last_stable_key)) {
        last_stable_key = raw_key;
        app_logic_on_key(raw_key);
        event_set(APP_EVENT_KEY);
    }

    if (raw_key == 0U) {
        last_stable_key = 0U;
        pending_key = 0U;
    }
}

static void sensor_loop_run(sched_event_t events, void *ctx)
{
    (void)events;
    (void)ctx;
    app_logic_on_sensor_tick();
    event_set(APP_EVENT_SENSOR);
}

static sched_loop_t logic_loop = SCHED_LOOP_DEF(
    "app_logic",
    app_logic_run,
    2U,
    0U,
    APP_EVENT_KEY | APP_EVENT_SENSOR,
    0U
);

static sched_loop_t key_loop = SCHED_LOOP_DEF(
    "key",
    key_loop_run,
    1U,
    0U,
    APP_EVENT_TICK,
    0U
);

static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor",
    sensor_loop_run,
    0U,
    APP_SENSOR_POLL_MS,
    0U,
    APP_EVENT_TICK
);

void app_main(void)
{
    bsp_init();
    sched_init();
    sched_loop_init();
    irq_event_init();
    devmgr_init_all();

    app_logic_init();

    (void)sched_loop_register(&logic_loop);
    (void)sched_loop_register(&key_loop);
    (void)sched_loop_register(&sensor_loop);
    sched_start();
}

int main(void)
{
    app_main();

    for (;;) {
    }
}
