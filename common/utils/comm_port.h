/**
 * @file comm_port.h
 * @brief 应用层“当前通信口”门面：绑定具名 comm_driver 并转发 send/recv/IRQ。
 *
 * 避免应用直接依赖 devmgr 与具体模块名；board 通过 BOARD_COMM_DEVICE 默认设备。
 * 与 irq_event 配合：comm_port_irq_source() + irq_event_bind 唤醒通信任务。
 */

#ifndef COMM_PORT_H
#define COMM_PORT_H

#include "comm_if.h"
#include "irq_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 按注册名绑定活动驱动；name 为 null 时用 BOARD_COMM_DEVICE。 */
void comm_port_bind(const char *device_name);

/** @brief 当前绑定的驱动指针；未 bind 时为 null。 */
const comm_driver_t *comm_port_driver(void);

/** @brief 转发 send；无驱动或无钩子时返回 -1。 */
int comm_port_send(const unsigned char *data, unsigned short len);

/** @brief 转发 recv；无驱动或无钩子时返回 -1。 */
int comm_port_recv(unsigned char *buf, unsigned short max_len);

/** @brief 转发异步 RX 回调注册。 */
void comm_port_register_rx_callback(comm_rx_callback_t callback);

/**
 * @brief 根据绑定设备名推断 USART RX 的 irq_event_source_t。
 * @return 已知映射；未知返回 IRQ_EVENT_SOURCE_COUNT。
 */
irq_event_source_t comm_port_irq_source(void);

/** @brief comm_port_irq_source() 是否有效。 */
int comm_port_has_irq(void);

#ifdef __cplusplus
}
#endif

#endif
