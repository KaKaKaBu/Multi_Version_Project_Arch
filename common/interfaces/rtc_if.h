#ifndef RTC_IF_H
#define RTC_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtc_time {
    unsigned short year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char week;
} rtc_time_t;

typedef struct rtc_driver {
    const char *name;
    void (*init)(void);
    void (*read_time)(rtc_time_t *time);
    void (*set_time)(const rtc_time_t *time);
} rtc_driver_t;

#ifdef __cplusplus
}
#endif

#endif
