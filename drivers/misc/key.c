/**
 * @file key.c
 * @brief Interrupt-driven key driver with single/double/long press callbacks.
 */

#include "key_service.h"
#include "input_if.h"
#include "gpio_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "scheduler.h"
#include "irq_event.h"
#include "exti_hal.h"
#include "hal_common.h"

/* -------------------------------------------------------------------------- */
/* Configuration defaults                                                     */
/* -------------------------------------------------------------------------- */

#if !defined(KEY_DRIVER_PIN_TABLE)
static const hal_pin_t *const key_default_pins[] = {
    &board_key1_pin,
    &board_key2_pin,
    &board_key3_pin,
    &board_key4_pin,
    &board_key5_pin
};
#define KEY_DRIVER_PIN_TABLE key_default_pins
#define KEY_DRIVER_HAS_DEFAULT_PIN_TABLE 1
#endif

#if !defined(KEY_DRIVER_BUTTON_COUNT)
#if defined(KEY_DRIVER_HAS_DEFAULT_PIN_TABLE)
#define KEY_DRIVER_BUTTON_COUNT ((uint8_t)(sizeof(key_default_pins) / sizeof(key_default_pins[0])))
#else
#error "KEY_DRIVER_BUTTON_COUNT must be defined when overriding KEY_DRIVER_PIN_TABLE"
#endif
#endif

typedef char key_driver_requires_at_least_one_button[(KEY_DRIVER_BUTTON_COUNT > 0) ? 1 : -1];

#ifndef KEY_DRIVER_DEBOUNCE_MS
#define KEY_DRIVER_DEBOUNCE_MS 30U
#endif

#ifndef KEY_DRIVER_DOUBLE_CLICK_MS
#define KEY_DRIVER_DOUBLE_CLICK_MS 400U
#endif

#ifndef KEY_DRIVER_LONG_PRESS_MS
#define KEY_DRIVER_LONG_PRESS_MS 800U
#endif

#ifndef KEY_DRIVER_IRQ_PRIORITY
#ifdef KEY_DRIVER_NVIC_PRIORITY
#define KEY_DRIVER_IRQ_PRIORITY KEY_DRIVER_NVIC_PRIORITY
#else
#define KEY_DRIVER_IRQ_PRIORITY 5U
#endif
#endif

#ifndef KEY_DRIVER_TASK_PRIORITY
#define KEY_DRIVER_TASK_PRIORITY 2U
#endif

#define KEY_DRIVER_MAX_IRQ_CHANNELS 7U
#define KEY_EVENT_FLAG_SINGLE (1U << 0)
#define KEY_EVENT_FLAG_DOUBLE (1U << 1)
#define KEY_EVENT_FLAG_LONG (1U << 2)
#define KEY_DRIVER_MAX_EVENT_BATCH ((KEY_DRIVER_BUTTON_COUNT * 3U) + 1U)
#define KEY_DRIVER_EVENT_IRQ 0x20000000UL
#define KEY_DRIVER_EVENT_WAIT_MASK (SCHED_EVENT_TICK | KEY_DRIVER_EVENT_IRQ)

/* -------------------------------------------------------------------------- */
/* State definitions                                                          */
/* -------------------------------------------------------------------------- */

typedef struct key_button_state {
    const hal_pin_t *pin;
    uint32_t exti_line_mask;
    exti_hal_irq_channel_t irq_channel;
    uint8_t index;
    uint8_t edge_valid;
    volatile uint8_t pressed;
    volatile uint8_t long_reported;
    volatile uint8_t pending_single;
    volatile uint8_t double_candidate;
    volatile uint8_t emit_flags;
    volatile uint32_t press_tick;
    volatile uint32_t last_edge_tick;
    volatile uint32_t single_deadline_tick;
} key_button_state_t;

typedef struct key_pending_event {
    uint8_t key_index;
    key_event_type_t type;
} key_pending_event_t;

static key_button_state_t key_buttons[KEY_DRIVER_BUTTON_COUNT];
static key_event_callback_t key_callback;
static void *key_callback_ctx;
static exti_hal_irq_channel_t key_enabled_irqs[KEY_DRIVER_MAX_IRQ_CHANNELS];
static uint8_t key_enabled_irq_count;
static uint8_t key_button_active_count;
static uint8_t key_service_task_registered;
static uint8_t key_irq_event_bound;

static void key_service_task_entry(struct driver_task *task);

static driver_task_t key_service_task = {
    .name = "key_fsm",
    .entry = key_service_task_entry,
    .priority = KEY_DRIVER_TASK_PRIORITY
};

/* -------------------------------------------------------------------------- */
/* Utility helpers                                                            */
/* -------------------------------------------------------------------------- */

static uint32_t key_enter_critical(void)
{
    return hal_irq_lock();
}

static void key_exit_critical(uint32_t irq_state)
{
    hal_irq_unlock(irq_state);
}

static uint8_t key_line_from_mask(uint32_t line_mask)
{
    uint8_t line;

    for (line = 0U; line < 16U; ++line) {
        if (line_mask == (uint32_t)(1UL << line)) {
            return line;
        }
    }
    return 0xFFU;
}

static void key_enable_irq_channel(exti_hal_irq_channel_t channel)
{
    uint8_t i;

    for (i = 0U; i < key_enabled_irq_count; ++i) {
        if (key_enabled_irqs[i] == channel) {
            return;
        }
    }

    if (key_enabled_irq_count < KEY_DRIVER_MAX_IRQ_CHANNELS) {
        key_enabled_irqs[key_enabled_irq_count++] = channel;
    }

    exti_hal_enable_irq(channel, KEY_DRIVER_IRQ_PRIORITY);
}

static uint8_t key_deadline_reached(uint32_t now, uint32_t deadline)
{
    return ((int32_t)(now - deadline) >= 0);
}

static uint8_t key_transition_allowed(key_button_state_t *btn, uint32_t now)
{
    if ((btn->edge_valid != 0U) && ((uint32_t)(now - btn->last_edge_tick) < KEY_DRIVER_DEBOUNCE_MS)) {
        return 0U;
    }

    btn->edge_valid = 1U;
    btn->last_edge_tick = now;
    return 1U;
}

/* -------------------------------------------------------------------------- */
/* Runtime state machine                                                      */
/* -------------------------------------------------------------------------- */

static void key_schedule_single(key_button_state_t *btn, uint32_t now)
{
    btn->pending_single = 1U;
    btn->single_deadline_tick = now + KEY_DRIVER_DOUBLE_CLICK_MS;
}

static void key_handle_press(key_button_state_t *btn, uint32_t now)
{
    if (key_transition_allowed(btn, now) == 0U) {
        return;
    }

    if ((btn->pending_single != 0U) && (key_deadline_reached(now, btn->single_deadline_tick) == 0U)) {
        btn->double_candidate = 1U;
        btn->pending_single = 0U;
    } else {
        btn->double_candidate = 0U;
    }

    btn->pressed = 1U;
    btn->press_tick = now;
    btn->long_reported = 0U;
}

static void key_handle_release(key_button_state_t *btn, uint32_t now)
{
    uint32_t held_ms;

    if ((btn->pressed == 0U) || (key_transition_allowed(btn, now) == 0U)) {
        return;
    }

    btn->pressed = 0U;
    held_ms = now - btn->press_tick;

    if (btn->long_reported != 0U) {
        btn->double_candidate = 0U;
        btn->pending_single = 0U;
        return;
    }

    if (held_ms >= KEY_DRIVER_LONG_PRESS_MS) {
        btn->emit_flags |= KEY_EVENT_FLAG_LONG;
        btn->long_reported = 1U;
        btn->pending_single = 0U;
        btn->double_candidate = 0U;
        return;
    }

    if ((btn->double_candidate != 0U) && (key_deadline_reached(now, btn->single_deadline_tick) == 0U)) {
        btn->emit_flags |= KEY_EVENT_FLAG_DOUBLE;
        btn->double_candidate = 0U;
        return;
    }

    btn->double_candidate = 0U;
    key_schedule_single(btn, now);
}

static void key_emit_event(uint8_t key_index, key_event_type_t type)
{
    key_event_callback_t cb = key_callback;

    if (cb != 0) {
        cb(key_index, type, key_callback_ctx);
    }
}

static void key_collect_and_dispatch_events(void)
{
    key_pending_event_t pending[KEY_DRIVER_MAX_EVENT_BATCH];
    uint8_t pending_count = 0U;
    uint32_t primask = key_enter_critical();
    uint32_t now = sched_tick_get();
    uint8_t i;

    for (i = 0U; i < KEY_DRIVER_BUTTON_COUNT; ++i) {
        key_button_state_t *btn = &key_buttons[i];

        if (btn->pin == 0) {
            continue;
        }

        if ((btn->pressed != 0U) && (btn->long_reported == 0U) &&
            ((uint32_t)(now - btn->press_tick) >= KEY_DRIVER_LONG_PRESS_MS)) {
            btn->long_reported = 1U;
            btn->pending_single = 0U;
            btn->double_candidate = 0U;
            btn->emit_flags |= KEY_EVENT_FLAG_LONG;
        }

        if ((btn->pending_single != 0U) && key_deadline_reached(now, btn->single_deadline_tick)) {
            btn->pending_single = 0U;
            btn->emit_flags |= KEY_EVENT_FLAG_SINGLE;
        }

        if (btn->emit_flags != 0U) {
            uint8_t flags = btn->emit_flags;

            if (((flags & KEY_EVENT_FLAG_LONG) != 0U) && (pending_count < KEY_DRIVER_MAX_EVENT_BATCH)) {
                pending[pending_count].key_index = btn->index;
                pending[pending_count].type = KEY_EVENT_LONG_PRESS;
                ++pending_count;
            }
            if (((flags & KEY_EVENT_FLAG_DOUBLE) != 0U) && (pending_count < KEY_DRIVER_MAX_EVENT_BATCH)) {
                pending[pending_count].key_index = btn->index;
                pending[pending_count].type = KEY_EVENT_DOUBLE_CLICK;
                ++pending_count;
            }
            if (((flags & KEY_EVENT_FLAG_SINGLE) != 0U) && (pending_count < KEY_DRIVER_MAX_EVENT_BATCH)) {
                pending[pending_count].key_index = btn->index;
                pending[pending_count].type = KEY_EVENT_SINGLE_CLICK;
                ++pending_count;
            }

            btn->emit_flags = 0U;
        }
    }

    key_exit_critical(primask);

    for (i = 0U; i < pending_count; ++i) {
        key_emit_event(pending[i].key_index, pending[i].type);
    }
}

static void key_process_exti(void)
{
    uint32_t now = sched_tick_get();
    uint8_t i;

    for (i = 0U; i < KEY_DRIVER_BUTTON_COUNT; ++i) {
        key_button_state_t *btn = &key_buttons[i];

        if ((btn->pin == 0) || (exti_hal_get_it_status(btn->exti_line_mask) == 0U)) {
            continue;
        }

        if (gpio_hal_read(btn->pin->port, btn->pin->pin) == 0U) {
            key_handle_press(btn, now);
        } else {
            key_handle_release(btn, now);
        }

        exti_hal_clear_pending(btn->exti_line_mask);
    }
}

static void key_service_task_entry(driver_task_t *task)
{
    (void)task_events_get(task);
    key_collect_and_dispatch_events();
    task_block(KEY_DRIVER_EVENT_WAIT_MASK);
}

/* -------------------------------------------------------------------------- */
/* Hardware init                                                               */
/* -------------------------------------------------------------------------- */

static uint8_t key_configure_buttons(void)
{
    uint8_t configured = 0U;
    uint8_t i;

    for (i = 0U; i < KEY_DRIVER_BUTTON_COUNT; ++i) {
        key_button_state_t *btn = &key_buttons[i];
        const hal_pin_t *pin = KEY_DRIVER_PIN_TABLE[i];
        uint8_t line;

        btn->pin = 0;
        if ((pin == 0) || (pin->port == 0)) {
            continue;
        }

        btn->exti_line_mask = exti_hal_line_mask_from_pin(pin);
        line = key_line_from_mask(btn->exti_line_mask);
        if (line == 0xFFU) {
            continue;
        }

        btn->pin = pin;
        btn->index = (uint8_t)(i + 1U);
        btn->irq_channel = exti_hal_irq_channel_from_line(line);
        btn->edge_valid = 0U;
        btn->pressed = 0U;
        btn->long_reported = 0U;
        btn->pending_single = 0U;
        btn->double_candidate = 0U;
        btn->emit_flags = 0U;
        btn->press_tick = 0U;
        btn->last_edge_tick = 0U;
        btn->single_deadline_tick = 0U;

        {
            hal_pin_t cfg = *pin;
            cfg.mode = GPIO_HAL_MODE_IN_PULLUP;
            gpio_hal_config_pin(&cfg);
        }

        if (exti_hal_configure_gpio_pin(pin, EXTI_HAL_TRIGGER_BOTH) == 0U) {
            btn->pin = 0;
            continue;
        }

        key_enable_irq_channel(btn->irq_channel);
        ++configured;
    }

    return configured;
}

static void key_register_service_task_once(void)
{
    if ((key_service_task_registered == 0U) && (key_button_active_count > 0U)) {
        (void)sched_add_task(&key_service_task);
        key_service_task_registered = 1U;
    }
}

static void key_init(void)
{
    key_button_active_count = key_configure_buttons();
    if ((key_button_active_count > 0U) && (key_irq_event_bound == 0U)) {
        (void)irq_event_bind(IRQ_EVENT_SOURCE_KEY_EXTI, KEY_DRIVER_EVENT_IRQ);
        key_irq_event_bound = 1U;
    }
    key_register_service_task_once();
}

/* -------------------------------------------------------------------------- */
/* Compatibility readout                                                      */
/* -------------------------------------------------------------------------- */

static unsigned char key_read_key(void)
{
    uint8_t i;

    for (i = 0U; i < KEY_DRIVER_BUTTON_COUNT; ++i) {
        const key_button_state_t *btn = &key_buttons[i];

        if ((btn->pin != 0) && (gpio_hal_read(btn->pin->port, btn->pin->pin) == 0U)) {
            return btn->index;
        }
    }

    return 0U;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void key_register_callback(key_event_callback_t callback, void *user_data)
{
    uint32_t primask = key_enter_critical();

    key_callback = callback;
    key_callback_ctx = user_data;

    key_exit_critical(primask);
}

static const input_driver_t key_drv = {
    "key",
    key_init,
    key_read_key
};

REGISTER_DRIVER(INPUT, key_drv);

void key_exti_irq_handler(uint32_t pending_mask)
{
    (void)pending_mask;
    key_process_exti();
}
