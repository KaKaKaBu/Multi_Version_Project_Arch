/**
 * @file gnss_if.h
 * @brief GNSS/GPS 接收机驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(gnss, 实例)；devmgr_get_gnss。
 * - 应用循环中调用 poll 解析 NMEA，get_fix 取 gnss_fix_t。
 */

#ifndef GNSS_IF_H
#define GNSS_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 接收机解析后的 GNSS 定位数据。 */
typedef struct gnss_fix {
    unsigned char valid;     ///< 非零表示经纬度有效。
    float latitude;        ///< 纬度（十进制度）。
    float longitude;       ///< 经度（十进制度）。
    float speed_kmh;       ///< 地速（km/h）。
    float course;          ///< 地面航向（度）。
    float altitude;        ///< 海拔（米）。
} gnss_fix_t;

/** @brief GNSS 模块驱动的虚函数表。 */
typedef struct gnss_driver {
    const char *name;              ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);            ///< 一次性硬件初始化。
    void (*poll)(void);            ///< 处理待处理的 NMEA 或二进制语句。
    /**
     * @brief 将最新解析的定位数据复制到调用方结构体。
     * @param[out] fix 输出定位缓冲；不可为 null。
     */
    void (*get_fix)(gnss_fix_t *fix);
} gnss_driver_t;

#ifdef __cplusplus
}
#endif

#endif
