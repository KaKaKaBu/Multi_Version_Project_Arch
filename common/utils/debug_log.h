/**
 * @file debug_log.h
 * @brief 编译期门控的 printf 调试宏，按子系统独立开关。
 *
 * 在 board_config 或编译选项中定义 DEBUG_LOG_ENABLE=1 启用总开关；
 * 各 DEBUG_LOG_*_ENABLE 可单独关闭嘈杂模块（如 MQTT_POLL）。
 * 禁用时宏展开为 do{}while(0)，不产生代码体积。
 *
 * 输出依赖 newlib printf，通常重定向到 debug_uart（见 syscalls/retarget）。
 */

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include "board_config.h"

#ifndef DEBUG_LOG_ENABLE
/** @brief 主开关：0 时所有子宏为空操作。 */
#define DEBUG_LOG_ENABLE 0
#endif

#ifndef DEBUG_LOG_APP_ENABLE
#define DEBUG_LOG_APP_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_APP_CB_ENABLE
#define DEBUG_LOG_APP_CB_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_ESP8266_ENABLE
#define DEBUG_LOG_ESP8266_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_ESP8266_DRAIN_ENABLE
#define DEBUG_LOG_ESP8266_DRAIN_ENABLE 0
#endif
#ifndef DEBUG_LOG_MQTT_ENABLE
#define DEBUG_LOG_MQTT_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_MQTT_AT_ENABLE
#define DEBUG_LOG_MQTT_AT_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_MQTT_POLL_ENABLE
#define DEBUG_LOG_MQTT_POLL_ENABLE 0
#endif
#ifndef DEBUG_LOG_IRQ_EVENT_ENABLE
#define DEBUG_LOG_IRQ_EVENT_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_USART_ENABLE
#define DEBUG_LOG_USART_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_RTC_ENABLE
#define DEBUG_LOG_RTC_ENABLE DEBUG_LOG_ENABLE
#endif
#ifndef DEBUG_LOG_SCHED_ENABLE
#define DEBUG_LOG_SCHED_ENABLE DEBUG_LOG_ENABLE
#endif

#if DEBUG_LOG_ENABLE
#include <stdio.h>
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_APP_ENABLE
#define DEBUG_LOG_APP(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_APP(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_APP_CB_ENABLE
#define DEBUG_LOG_APP_CB(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_APP_CB(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_ESP8266_ENABLE
#define DEBUG_LOG_ESP8266(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_ESP8266(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_ESP8266_DRAIN_ENABLE
#define DEBUG_LOG_ESP8266_DRAIN(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_ESP8266_DRAIN(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_MQTT_ENABLE
#define DEBUG_LOG_MQTT(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_MQTT(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_MQTT_AT_ENABLE
#define DEBUG_LOG_MQTT_AT(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_MQTT_AT(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_MQTT_POLL_ENABLE
#define DEBUG_LOG_MQTT_POLL(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_MQTT_POLL(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_IRQ_EVENT_ENABLE
#define DEBUG_LOG_IRQ_EVENT(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_IRQ_EVENT(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_USART_ENABLE
#define DEBUG_LOG_USART(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_USART(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_RTC_ENABLE
#define DEBUG_LOG_RTC(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_RTC(...) do { } while (0)
#endif

#if DEBUG_LOG_ENABLE && DEBUG_LOG_SCHED_ENABLE
#define DEBUG_LOG_SCHED(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG_SCHED(...) do { } while (0)
#endif

#endif
