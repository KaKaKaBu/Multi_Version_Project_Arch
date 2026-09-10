/**
 * @file ir_remote.c
 * @brief NEC/HX1838 infrared remote input driver.
 */

#include "input_if.h"
#include "driver_configs.h"
#include "driver_core.h"
#include "exti_hal.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "timer_hal.h"

#define IR_KEY_UP      0x52U
#define IR_KEY_DOWN    0x18U
#define IR_KEY_LEFT    0x5AU
#define IR_KEY_RIGHT   0x08U
#define IR_KEY_OK      0x1CU

typedef enum ir_state {
    IR_RX_IDLE = 0,
    IR_RX_WAIT_LEAD_RISE,
    IR_RX_WAIT_LEAD_FALL,
    IR_RX_WAIT_BIT_RISE,
    IR_RX_WAIT_BIT_FALL
} ir_state_t;

static const ir_remote_driver_config_t *ir_config;
static volatile ir_state_t ir_state;
static volatile uint16_t ir_last_edge_us;
static volatile uint32_t ir_raw_data;
static volatile uint8_t ir_bit_idx;
static volatile uint8_t ir_pending_key;
static volatile uint8_t ir_last_cmd;
static volatile uint8_t ir_fail_stage;

static uint8_t in_range_u16(uint16_t value, uint16_t lo, uint16_t hi)
{
    return ((value >= lo) && (value <= hi)) ? 1U : 0U;
}

static uint8_t bit_reverse8(uint8_t value)
{
    value = (uint8_t)(((value & 0xF0U) >> 4) | ((value & 0x0FU) << 4));
    value = (uint8_t)(((value & 0xCCU) >> 2) | ((value & 0x33U) << 2));
    value = (uint8_t)(((value & 0xAAU) >> 1) | ((value & 0x55U) << 1));
    return value;
}

static void ir_reset(void)
{
    ir_state = IR_RX_IDLE;
    ir_raw_data = 0U;
    ir_bit_idx = 0U;
}

static uint8_t ir_map_key(uint8_t code)
{
    switch (code) {
    case IR_KEY_UP:
        return 1U;
    case IR_KEY_DOWN:
        return 2U;
    case IR_KEY_LEFT:
        return 3U;
    case IR_KEY_RIGHT:
        return 4U;
    case IR_KEY_OK:
        return 5U;
    default:
        return 0U;
    }
}

static void ir_accept_key(uint8_t code)
{
    uint8_t key = ir_map_key(code);
    if (key != 0U) {
        ir_last_cmd = code;
        ir_pending_key = key;
        ir_fail_stage = 0U;
    }
    ir_reset();
}

static uint8_t ir_pin_active_level(void)
{
    uint8_t level;

    if (ir_config == 0) {
        return 1U;
    }
    level = gpio_hal_read(ir_config->pin.port, ir_config->pin.pin);
    return (ir_config->active_low != 0U) ? (uint8_t)(level == 0U) : level;
}

static void ir_decode_edge(void)
{
    uint16_t now_us;
    uint16_t delta_us;
    uint8_t active_now;

    if (ir_config == 0) {
        return;
    }

    now_us = timer_hal_get_counter_us(ir_config->timer);
    delta_us = (uint16_t)(now_us - ir_last_edge_us);
    ir_last_edge_us = now_us;
    active_now = ir_pin_active_level();

    if (delta_us > 15000U) {
        ir_reset();
    }

    if (active_now != 0U) {
        if (ir_state == IR_RX_IDLE) {
            if (delta_us > 3000U) {
                ir_state = IR_RX_WAIT_LEAD_RISE;
            }
            return;
        }

        if (ir_state == IR_RX_WAIT_BIT_FALL) {
            if (in_range_u16(delta_us, 100U, 1000U) != 0U) {
                /* bit 0 */
            } else if (in_range_u16(delta_us, 1100U, 2500U) != 0U) {
                ir_raw_data |= ((uint32_t)1U << ir_bit_idx);
            } else {
                ir_fail_stage = 9U;
                ir_reset();
                return;
            }

            ++ir_bit_idx;
            if (ir_bit_idx >= 32U) {
                uint8_t addr = (uint8_t)(ir_raw_data & 0xFFU);
                uint8_t addr_inv = (uint8_t)((ir_raw_data >> 8) & 0xFFU);
                uint8_t cmd = (uint8_t)((ir_raw_data >> 16) & 0xFFU);
                uint8_t cmd_inv = (uint8_t)((ir_raw_data >> 24) & 0xFFU);

                if (ir_map_key(cmd) != 0U) {
                    ir_accept_key(cmd);
                } else if (ir_map_key(bit_reverse8(cmd)) != 0U) {
                    ir_accept_key(bit_reverse8(cmd));
                } else if (((uint8_t)(addr + addr_inv) == 0xFFU) &&
                           ((uint8_t)(cmd + cmd_inv) == 0xFFU) &&
                           (ir_map_key(cmd) != 0U)) {
                    ir_accept_key(cmd);
                } else {
                    ir_fail_stage = 13U;
                    ir_reset();
                }
                return;
            }
            ir_state = IR_RX_WAIT_BIT_RISE;
            return;
        }

        if (ir_state == IR_RX_WAIT_LEAD_FALL) {
            ir_fail_stage = 1U;
            ir_reset();
        }
        return;
    }

    if (ir_state == IR_RX_WAIT_LEAD_RISE) {
        if (in_range_u16(delta_us, 6800U, 11200U) == 0U) {
            ir_fail_stage = 4U;
            ir_reset();
            return;
        }
        ir_state = IR_RX_WAIT_LEAD_FALL;
        return;
    }

    if (ir_state == IR_RX_WAIT_LEAD_FALL) {
        if (in_range_u16(delta_us, 1200U, 3200U) != 0U) {
            if (ir_last_cmd != 0U) {
                ir_accept_key(ir_last_cmd);
            } else {
                ir_reset();
            }
            return;
        }
        if (in_range_u16(delta_us, 3200U, 6200U) == 0U) {
            ir_fail_stage = 5U;
            ir_reset();
            return;
        }
        ir_raw_data = 0U;
        ir_bit_idx = 0U;
        ir_state = IR_RX_WAIT_BIT_RISE;
        return;
    }

    if (ir_state == IR_RX_WAIT_BIT_RISE) {
        if (in_range_u16(delta_us, 150U, 1200U) == 0U) {
            ir_fail_stage = 8U;
            ir_reset();
            return;
        }
        ir_state = IR_RX_WAIT_BIT_FALL;
    }
}

static void ir_remote_init(const void *config)
{
    uint32_t line_mask;
    exti_hal_irq_channel_t irq_channel;

    ir_config = (const ir_remote_driver_config_t *)config;
    if (ir_config == 0) {
        return;
    }

    gpio_hal_config_pin(&ir_config->pin);
    timer_hal_init_us(ir_config->timer, 0xFFFFU);
    line_mask = exti_hal_line_mask_from_pin(&ir_config->pin);
    if (line_mask == 0U) {
        return;
    }

    (void)exti_hal_configure_gpio_pin(&ir_config->pin, EXTI_HAL_TRIGGER_BOTH);
    irq_channel = exti_hal_irq_channel_from_line((uint8_t)ir_config->pin.pin);
    exti_hal_enable_irq(irq_channel, (ir_config->irq_priority == 0U) ? 5U : ir_config->irq_priority);
    ir_last_edge_us = timer_hal_get_counter_us(ir_config->timer);
    ir_reset();
}

static unsigned char ir_remote_read_key(void)
{
    uint8_t key;
    uint32_t state = hal_irq_lock();

    key = ir_pending_key;
    ir_pending_key = 0U;
    hal_irq_unlock(state);
    return key;
}

void ir_remote_exti_irq_handler(uint32_t pending_mask)
{
    if (ir_config == 0) {
        return;
    }
    if ((pending_mask & exti_hal_line_mask_from_pin(&ir_config->pin)) == 0U) {
        return;
    }
    ir_decode_edge();
}

uint8_t ir_remote_last_code(void)
{
    return ir_last_cmd;
}

uint8_t ir_remote_fail_stage(void)
{
    return ir_fail_stage;
}

static const input_driver_t ir_remote_drv = {
    "ir_remote",
    ir_remote_init,
    ir_remote_read_key
};

REGISTER_DRIVER(INPUT, ir_remote_drv);
