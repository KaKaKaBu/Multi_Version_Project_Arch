#ifndef HAL_STATUS_H
#define HAL_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hal_status {
    HAL_OK = 0,
    HAL_ERR_TIMEOUT = -1,
    HAL_ERR_BUSY = -2,
    HAL_ERR_NACK = -3,
    HAL_ERR_PARAM = -4,
    HAL_ERR_IO = -5
} hal_status_t;

#ifdef __cplusplus
}
#endif

#endif
