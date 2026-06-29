#ifndef HAL_STATUS_H
#define HAL_STATUS_H

typedef enum hal_status {
    HAL_OK = 0,
    HAL_ERR_TIMEOUT,
    HAL_ERR_BUSY,
    HAL_ERR_NACK,
    HAL_ERR_PARAM,
    HAL_ERR_IO
} hal_status_t;

#endif
