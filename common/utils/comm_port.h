#ifndef COMM_PORT_H
#define COMM_PORT_H

#include "comm_if.h"
#include "irq_event.h"

#ifdef __cplusplus
extern "C" {
#endif

void comm_port_bind(const char *device_name);
const comm_driver_t *comm_port_driver(void);

int comm_port_send(const unsigned char *data, unsigned short len);
int comm_port_recv(unsigned char *buf, unsigned short max_len);
void comm_port_register_rx_callback(comm_rx_callback_t callback);

irq_event_source_t comm_port_irq_source(void);
int comm_port_has_irq(void);

#ifdef __cplusplus
}
#endif

#endif
