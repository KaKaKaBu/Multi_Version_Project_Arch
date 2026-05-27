#ifndef RADIO_IF_H
#define RADIO_IF_H

/*
 * 兼容旧接口：无线收发已统一为 comm_if.h / comm_driver_t。
 * 新代码请使用 devmgr_get_comm() 或 comm_port。
 */
#include "comm_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef comm_driver_t radio_driver_t;

#ifdef __cplusplus
}
#endif

#endif
