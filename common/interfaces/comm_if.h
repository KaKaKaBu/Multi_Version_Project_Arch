/**
 * @file comm_if.h
 * @brief 流式与分组通信驱动接口。
 *
 * ## 驱动模型
 * - kind：COMM_KIND_STREAM（AT/透明串口）或 COMM_KIND_PACKET（Wi-Fi 分组模块）。
 * - REGISTER_DRIVER(comm, 实例)；devmgr_get_comm / comm_port_bind 按 name 查找。
 * - 可与 irq_event + sched_loop 配合实现 RX 唤醒。
 */

#ifndef COMM_IF_H
#define COMM_IF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 通信驱动的传输方式。 */
typedef enum comm_kind {
    COMM_KIND_STREAM = 0U,  ///< 字节流（AT 指令、透明 UART）。
    COMM_KIND_PACKET = 1U   ///< 分组无线或 Wi-Fi 模块。
} comm_kind_t;

/**
 * @brief 收到数据时的回调。
 * @param[in] data 接收到的字节。
 * @param[in] len 数据长度。
 */
typedef void (*comm_rx_callback_t)(const unsigned char *data, unsigned short len);

/** @brief UART、Wi-Fi 或分组无线通信的虚函数表。 */
typedef struct comm_driver {
    const char *name;                                      ///< 驱动实例名称，供 devmgr 查找。
    comm_kind_t kind;                                      ///< 流式或分组语义。
    void (*init)(void);                                    ///< 一次性硬件初始化。
    /**
     * @brief 发送数据。
     * @param[in] data 待发送字节。
     * @param[in] len 数据长度。
     * @return 已发送字节数，失败时为负值。
     */
    int (*send)(const unsigned char *data, unsigned short len);
    /**
     * @brief 接收数据。
     * @param[out] buf 接收缓冲区。
     * @param[in] max_len 缓冲区最大长度。
     * @return 实际接收字节数，失败时为负值。
     */
    int (*recv)(unsigned char *buf, unsigned short max_len);
    /**
     * @brief 注册异步接收回调。
     * @param[in] callback 回调函数指针。
     */
    void (*register_rx_callback)(comm_rx_callback_t callback);
} comm_driver_t;

#ifdef __cplusplus
}
#endif

#endif
