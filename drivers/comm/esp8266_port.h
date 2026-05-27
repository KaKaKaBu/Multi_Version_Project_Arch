#ifndef ESP8266_PORT_H
#define ESP8266_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void esp8266_port_rx_clear(void);
void esp8266_port_rx_push(const uint8_t *data, uint16_t len);
uint16_t esp8266_port_rx_length(void);
const uint8_t *esp8266_port_rx_data(void);
void esp8266_port_rx_discard(uint16_t len);

int esp8266_port_send(const uint8_t *data, uint16_t len);
int esp8266_port_send_str(const char *text);
int esp8266_port_recv_byte(uint8_t *byte);
void esp8266_port_drain_rx(void);

int esp8266_port_wait_token(const char *token, uint32_t timeout_ms);
int esp8266_port_wait_prompt(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
