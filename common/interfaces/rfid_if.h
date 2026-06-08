/**
 * @file rfid_if.h
 * @brief RFID 读卡器（如 RC522）驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(rfid, 实例)；devmgr_get_rfid("name")。
 * - 典型：poll_card → get_uid → clear_card。
 */

#ifndef RFID_IF_H
#define RFID_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief RFID 读卡器驱动的虚函数表。 */
typedef struct rfid_driver {
    const char *name;              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(void);            ///< 一次性硬件初始化。
    /**
     * @brief 轮询场内是否有卡。
     * @return 有卡时返回非零。
     */
    unsigned char (*poll_card)(void);
    /**
     * @brief 将检测到的卡 UID 复制到调用方缓冲区。
     * @param[out] uid UID 输出缓冲。
     * @param[in] max_len uid 缓冲区容量（字节）。
     * @param[out] uid_len 实际写入的 UID 长度。
     */
    void (*get_uid)(unsigned char *uid, unsigned char max_len, unsigned char *uid_len);
    void (*clear_card)(void);      ///< 读取后清除内部卡状态。
} rfid_driver_t;

#ifdef __cplusplus
}
#endif

#endif
