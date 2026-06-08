/**
 * @file hal_status.h
 * @brief Common HAL return codes used across BSP drivers.
 */

#ifndef HAL_STATUS_H
#define HAL_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HAL operation result codes (negative values indicate errors). */
typedef enum hal_status {
    HAL_OK = 0,           /**< @brief Operation completed successfully. */
    HAL_ERR_TIMEOUT = -1, /**< @brief Operation timed out. */
    HAL_ERR_BUSY = -2,    /**< @brief Peripheral or resource is busy. */
    HAL_ERR_NACK = -3,    /**< @brief No acknowledge (e.g. I2C address NACK). */
    HAL_ERR_PARAM = -4,   /**< @brief Invalid parameter or NULL pointer. */
    HAL_ERR_IO = -5       /**< @brief I/O error (e.g. no data available). */
} hal_status_t;

#ifdef __cplusplus
}
#endif

#endif
