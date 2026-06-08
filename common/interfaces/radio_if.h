/**
 * @file radio_if.h
 * @brief 无线驱动的遗留别名；新代码请使用 comm_if.h 与 comm_port。
 *
 * radio_driver_t 即 comm_driver_t；PACKET 类型 comm 可由 devmgr_get_radio 返回。
 */

#ifndef RADIO_IF_H
#define RADIO_IF_H

#include "comm_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 分组/流式无线驱动的类型别名（与 comm_driver_t 相同）。 */
typedef comm_driver_t radio_driver_t;

#ifdef __cplusplus
}
#endif

#endif
