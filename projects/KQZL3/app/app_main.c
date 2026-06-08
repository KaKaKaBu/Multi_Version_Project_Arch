#include "devmgr.h"
#include "hal_common.h"
#include "irq_event.h"
#include "scheduler.h"
#include "sched_loop.h"
#include "version_config.h"
#include "app_logic.h"
#include "input_if.h"
#include "gpio_hal.h"
#include "board_config.h"

/* ============================================================================
 * sched_loop task definitions
 * ============================================================================ */

/* Sensor reading loop - every 2 seconds */
static sched_loop_t sensor_loop = SCHED_LOOP_DEF(
    "sensor", sensor_loop_run, 2, 2000,
    SCHED_EVENT_TICK, APP_EVENT_SENSOR_READY);

/* Alarm control loop - every 1 second */
static sched_loop_t alarm_loop = SCHED_LOOP_DEF(
    "alarm", alarm_loop_run, 1, 1000,
    SCHED_EVENT_TICK, APP_EVENT_TICK);

/* Display update loop - every 500ms */
static sched_loop_t display_loop = SCHED_LOOP_DEF(
    "display", display_loop_run, 3, 500,
    SCHED_EVENT_TICK, APP_EVENT_TICK);

/* Communication loop - event driven */
static sched_loop_t comm_loop = SCHED_LOOP_DEF(
    "comm", comm_loop_run, 4, 0,
    APP_EVENT_COMM_RX, 0);

/* Key polling loop - 10ms for debouncing */
static sched_loop_t key_loop;

static void key_loop_run(sched_event_t events, void *ctx)
{
    static uint8_t key_state = 0;
    static uint8_t key_debounce = 0;
    static uint16_t key_press_count = 0;

    const hal_pin_t *key_pins[4] = {
        &board_key1_pin,
        &board_key2_pin,
        &board_key3_pin,
        &board_key4_pin
    };

    (void)events;
    (void)ctx;

    /* Read current key states (active low) */
    uint8_t current_state = 0;
    for (int i = 0; i < 4; i++) {
        if (gpio_hal_read(key_pins[i]->port, key_pins[i]->pin) == 0) {
            current_state |= (1 << i);
        }
    }

    /* Debounce logic */
    if (current_state != key_state) {
        key_debounce++;
        if (key_debounce >= 5) {  /* 50ms debounce */
            /* Key state changed and stable */
            uint8_t pressed = (~key_state) & current_state;

            /* Handle key press events */
            if (pressed & 0x01) app_logic_on_key1_press();
            if (pressed & 0x02) app_logic_on_key2_press();
            if (pressed & 0x04) app_logic_on_key3_press();
            if (pressed & 0x08) app_logic_on_key4_press();

            key_state = current_state;
            key_debounce = 0;
        }
    } else {
        key_debounce = 0;
    }

    key_press_count++;
}

/* ============================================================================
 * Application main
 * ============================================================================ */

void app_main(void)
{
    bsp_init();
    sched_init();
    irq_event_init();
    devmgr_init_all();

    /* Initialize application logic */
    app_logic_init();

    /* Setup sched_loop tasks */
    sched_loop_init();
    sched_loop_register(&sensor_loop);
    sched_loop_register(&alarm_loop);
    sched_loop_register(&display_loop);
    sched_loop_register(&comm_loop);

    /* Setup key polling loop */
    key_loop = (sched_loop_t)SCHED_LOOP_DEF("key", key_loop_run, 0, 10, SCHED_EVENT_TICK, APP_EVENT_KEY);
    sched_loop_register(&key_loop);

    sched_start();
}

int main(void)
{
    app_main();
    for (;;) {
    }
}

