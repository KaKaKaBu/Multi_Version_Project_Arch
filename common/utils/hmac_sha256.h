/**
 * @file hmac_sha256.h
 * @brief Small SHA-256/HMAC-SHA256 helper used by cloud MQTT authentication.
 */

#ifndef HMAC_SHA256_H
#define HMAC_SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculates HMAC-SHA256 and writes lowercase hexadecimal text.
 *
 * @param key HMAC key bytes.
 * @param key_len Key length in bytes.
 * @param message Message bytes.
 * @param message_len Message length in bytes.
 * @param out_hex Output buffer; must hold at least 65 bytes.
 * @return 0 on success, -1 on invalid arguments.
 */
int hmac_sha256_hex(const uint8_t *key,
                    size_t key_len,
                    const uint8_t *message,
                    size_t message_len,
                    char out_hex[65]);

#ifdef __cplusplus
}
#endif

#endif
