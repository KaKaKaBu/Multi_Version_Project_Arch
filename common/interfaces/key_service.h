
/**
 * @file key_service.h
 * @brief Shared key event interface exposed to applications (interrupt-driven driver).
 */

#ifndef KEY_SERVICE_H
#define KEY_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief High-level key events produced by the state machine. */
typedef enum key_event_type {
    KEY_EVENT_SINGLE_CLICK = 0, /**< Key pressed and released once within the double-click window. */
    KEY_EVENT_DOUBLE_CLICK,     /**< Two short presses within the configured double-click window. */
    KEY_EVENT_LONG_PRESS        /**< Press held longer than the configured long-press threshold. */
} key_event_type_t;

/**
 * @brief Callback signature for key events.
 *
 * @param key_index 1-based key index defined by the board pin table.
 * @param event     Event type (single/double/long).
 * @param user_data Pointer provided during registration (may be NULL).
 */
typedef void (*key_event_callback_t)(uint8_t key_index,
                                     key_event_type_t event,
                                     void *user_data);

/**
 * @brief Registers (or replaces) the callback invoked when key events are dispatched.
 *
 * The callback executes in the cooperative key service task context (not inside
 * hardware ISRs). Pass NULL to disable notifications.
 */
void key_register_callback(key_event_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* KEY_SERVICE_H */
