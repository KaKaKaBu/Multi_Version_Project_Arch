/**
 * @file comm_port.c
 * @brief 通信端口门面：devmgr 查找、send/recv 转发、设备名→IRQ 源映射。
 */

#include "comm_port.h"
#include "devmgr.h"
#include "board_config.h"

#ifndef BOARD_COMM_DEVICE
/** @brief 未在 board_config 指定时的默认 Wi-Fi/串口模块注册名。 */
#define BOARD_COMM_DEVICE "esp8266"
#endif

/** 当前会话绑定的 comm 驱动；由 comm_port_bind 设置。 */
static const comm_driver_t *comm_port_active;

/**
 * @brief 根据设备名前缀推断 USART RX IRQ 槽（与板级 USART 分配约定一致）。
 *
 * "esp*" → BOARD_ESP8266_USART_ID 或默认 USART1；
 * "jdy*" → USART2（JDY 蓝牙模块常见接线）。
 */
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
