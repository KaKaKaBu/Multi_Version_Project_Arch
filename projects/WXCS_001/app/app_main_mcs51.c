#include "devmgr.h"
#include "hal_common.h"
#include "app_logic.h"
#include "board_config.h"

#define APP_MCS51_KEY_DEBOUNCE_LOOPS 5U
#define APP_MCS51_LOOP_DELAY_US 10000UL
#define APP_MCS51_SENSOR_PERIOD_LOOPS 50U

static const input_driver_t *app_keys;

static void app_mcs51_key_poll(void)
{
    static unsigned char last_stable_key;
    static unsigned char pending_key;
    static unsigned char stable_count;
    unsigned char raw_key;

    if (app_keys == 0) {
        app_keys = devmgr_get_input("key");
        if (app_keys == 0) {
            return;
        }
    }

    raw_key = app_keys->read_key();
    if (raw_key != pending_key) {
        pending_key = raw_key;
        stable_count = 0U;
        return;
    }

    if (stable_count < APP_MCS51_KEY_DEBOUNCE_LOOPS) {
        ++stable_count;
        return;
    }

    if ((raw_key != 0U) && (raw_key != last_stable_key)) {
        last_stable_key = raw_key;
        app_logic_on_key(raw_key);
        logic_loop_run(APP_EVENT_KEY, 0);
    } else if (raw_key == 0U) {
        last_stable_key = 0U;
    }
}

void app_main(void)
{
    unsigned char sensor_counter = 0U;

    bsp_init();
    devmgr_init_all();
    app_logic_init();

    for (;;) {
        app_mcs51_key_poll();

        ++sensor_counter;
        if (sensor_counter >= APP_MCS51_SENSOR_PERIOD_LOOPS) {
            sensor_counter = 0U;
            sensor_loop_run(APP_EVENT_TICK, 0);
            logic_loop_run(APP_EVENT_SENSOR, 0);
        } else {
            logic_loop_run(APP_EVENT_TICK, 0);
        }

        hal_delay_us(APP_MCS51_LOOP_DELAY_US);
    }
}

int main(void)
{
    app_main();
    return 0;
}
