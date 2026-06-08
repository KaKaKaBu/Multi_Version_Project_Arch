/**
 * @file esp8266_port.h
 * @brief Low-level USART transport and RX stream helpers for ESP8266 AT commands.
 */

#ifndef ESP8266_PORT_H
#define ESP8266_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Clears the software RX stream buffer and USART hardware FIFO. */
void esp8266_port_rx_clear(void);

/**
 * @brief Appends bytes to the bounded software RX stream buffer.
 *
 * @param data Pointer to incoming bytes; ignored when null or len is zero.
 * @param len Number of bytes to append.
 */
void esp8266_port_rx_push(const uint8_t *data, uint16_t len);

/**
 * @brief Returns the number of bytes currently stored in the RX stream buffer.
 *
 * @return RX stream length in bytes.
 */
uint16_t esp8266_port_rx_length(void);

/**
 * @brief Returns a pointer to the RX stream buffer contents.
 *
 * @return Read-only pointer valid until the next discard or clear.
 */
const uint8_t *esp8266_port_rx_data(void);

/**
 * @brief Removes len bytes from the front of the RX stream buffer.
 *
 * @param len Number of bytes to discard; clears all when len exceeds length.
 */
void esp8266_port_rx_discard(uint16_t len);

/**
 * @brief Sends a raw byte buffer over the ESP8266 USART.
 *
 * @param data Pointer to bytes to transmit.
 * @param len Number of bytes to send.
 * @return Number of bytes sent on success, or -1 on error.
 */
int esp8266_port_send(const uint8_t *data, uint16_t len);

/**
 * @brief Sends a null-terminated string over the ESP8266 USART.
 *
 * @param text C string to transmit.
 * @return Number of bytes sent on success, or -1 on error.
 */
int esp8266_port_send_str(const char *text);

/**
 * @brief Reads one byte from the USART hardware FIFO if available.
 *
 * @param byte Output byte storage; ignored when null.
 * @return 1 if a byte was read, 0 if the FIFO is empty.
 */
int esp8266_port_recv_byte(uint8_t *byte);

/**
 * @brief Drains the USART FIFO into the RX stream and invokes any raw RX callback.
 *
 * Call before parsing AT responses or waiting for tokens.
 */
void esp8266_port_drain_rx(void);

/**
 * @brief Polls the RX stream until token appears or timeout expires.
 *
 * @param token Substring to search for in the RX stream.
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return 1 if token was found, 0 on timeout.
 */
int esp8266_port_wait_token(const char *token, uint32_t timeout_ms);

/**
 * @brief Waits for the ESP8266 '>' prompt used before raw payload uploads.
 *
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return 1 if the prompt appeared, 0 on timeout.
 */
int esp8266_port_wait_prompt(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
