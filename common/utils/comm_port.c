#include "comm_port.h"
#include "devmgr.h"
#include "board_config.h"

#ifndef BOARD_COMM_DEVICE
#define BOARD_COMM_DEVICE "esp8266"
#endif

static const comm_driver_t *comm_port_active;

static irq_event_source_t comm_port_irq_for_device(const char *name)
{
    if (name == 0) {
        return IRQ_EVENT_SOURCE_COUNT;
    }

    if ((name[0] == 'e') && (name[1] == 's') && (name[2] == 'p')) {
#if defined(BOARD_ESP8266_USART_ID)
#if (BOARD_ESP8266_USART_ID == 1U)
        return IRQ_EVENT_SOURCE_USART1_RX;
#elif (BOARD_ESP8266_USART_ID == 2U)
        return IRQ_EVENT_SOURCE_USART2_RX;
#else
        return IRQ_EVENT_SOURCE_USART3_RX;
#endif
#else
        return IRQ_EVENT_SOURCE_USART1_RX;
#endif
    }

    if ((name[0] == 'j') && (name[1] == 'd') && (name[2] == 'y')) {
        return IRQ_EVENT_SOURCE_USART2_RX;
    }

    return IRQ_EVENT_SOURCE_COUNT;
}

void comm_port_bind(const char *device_name)
{
    const char *name = device_name;

    if (name == 0) {
        name = BOARD_COMM_DEVICE;
    }

    comm_port_active = devmgr_get_comm(name);
}

const comm_driver_t *comm_port_driver(void)
{
    return comm_port_active;
}

int comm_port_send(const unsigned char *data, unsigned short len)
{
    if ((comm_port_active == 0) || (comm_port_active->send == 0)) {
        return -1;
    }

    return comm_port_active->send(data, len);
}

int comm_port_recv(unsigned char *buf, unsigned short max_len)
{
    if ((comm_port_active == 0) || (comm_port_active->recv == 0)) {
        return -1;
    }

    return comm_port_active->recv(buf, max_len);
}

void comm_port_register_rx_callback(comm_rx_callback_t callback)
{
    if ((comm_port_active == 0) || (comm_port_active->register_rx_callback == 0)) {
        return;
    }

    comm_port_active->register_rx_callback(callback);
}

irq_event_source_t comm_port_irq_source(void)
{
    if (comm_port_active == 0) {
        return comm_port_irq_for_device(BOARD_COMM_DEVICE);
    }

    return comm_port_irq_for_device(comm_port_active->name);
}

int comm_port_has_irq(void)
{
    return (comm_port_irq_source() < IRQ_EVENT_SOURCE_COUNT) ? 1 : 0;
}
