/**
 * @file rtc_if.h
 * @brief 实时时钟驱动接口。
 *
 * ## 驱动模型
 * - REGISTER_DRIVER(rtc, 实例)；devmgr_get_rtc。
 * - rtc_time_t 为驱动与应用的日历时间交换结构。
 */

#ifndef RTC_IF_H
#define RTC_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief RTC 提供的日历日期与时间快照。 */
typedef struct rtc_time {
    unsigned short year;    ///< 四位年份（如 2026）。
    unsigned char month;  ///< 月份 1–12。
    unsigned char day;    ///< 日 1–31。
    unsigned char hour;   ///< 时 0–23。
    unsigned char minute; ///< 分 0–59。
    unsigned char second; ///< 秒 0–59。
    unsigned char week;   ///< 星期 0–6（由驱动约定）。
} rtc_time_t;

/** @brief 实时时钟芯片驱动的虚函数表。 */
typedef struct rtc_driver {
    const char *name;                       ///< 驱动实例名称，供 devmgr 查找。
    void (*init)(const void *config);                     ///< 一次性硬件初始化。
    /**
     * @brief 读取当前日历时间。
     * @param[out] time 输出时间；为 null 时忽略。
     */
    void (*read_time)(rtc_time_t *time);
    /**
     * @brief 设置 RTC 时间。
     * @param[in] time 待写入时间；为 null 时忽略。
     */
    void (*set_time)(const rtc_time_t *time);
} rtc_driver_t;

#ifdef __cplusplus
}
#endif

#endif
