#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "version_config.h"

#if defined(PLATFORM_MCS51)
#include "mcs51_memory.h"
typedef unsigned long sched_event_t;
#else
#include "scheduler.h"
#endif

/* 页面/控制模式：自动判定、手动控制、阈值设置三种界面共用一套按键。 */
typedef enum {
    APP_MODE_AUTO = 0,
    APP_MODE_MANUAL = 1,
    APP_MODE_THRESHOLD = 2
} app_mode_t;

/* 阈值设置项：温度、空气质量、CO 按顺序循环选择。 */
typedef enum {
    APP_THRESHOLD_TEMP = 0,
    APP_THRESHOLD_AQ = 1,
    APP_THRESHOLD_CO = 2
} app_threshold_t;

/* 应用运行态：传感器值、阈值、输出状态和远程遥测节流都集中保存在这里。 */
typedef struct {
    app_mode_t mode;
    unsigned char selected_device;
    app_threshold_t selected_threshold;
    unsigned char fan_on;
    unsigned char alarm_on;
    #if defined(PLATFORM_MCS51)
    int temperature;
    #else
    float temperature;
    #endif
    unsigned short air_quality_ppm;
    unsigned short co_ppm;
    unsigned char temp_threshold;
    unsigned short aq_threshold_ppm;
    unsigned short co_threshold_ppm;
    unsigned char display_dirty;
#if VERSION_FEATURE_REMOTE
    unsigned char telemetry_pending;
    unsigned long last_telemetry_tick;
#endif
} app_context_t;

#if defined(PLATFORM_MCS51)
extern MCS51_XDATA app_context_t g_ctx;
#else
extern app_context_t g_ctx;
#endif

/* 业务入口：初始化、主逻辑循环、按键处理和周期采样。 */
void app_logic_init(void);
void app_logic_loop(sched_event_t events);
void app_logic_on_key(unsigned char key_id);
void app_logic_on_sensor_tick(void);

#if VERSION_FEATURE_REMOTE
/* 远程入口：WiFi/MQTT 与蓝牙透传最终都进入同一套 JSON 解析逻辑。 */
void app_logic_on_mqtt_rx(const char *json_data);
void app_logic_request_telemetry(void);
void app_publish_telemetry(void);
#endif

/* sched_loop 回调：由 app_main.c 注册到协作调度器。 */
void sensor_loop_run(sched_event_t events, void *ctx);
void logic_loop_run(sched_event_t events, void *ctx);
void key_loop_run(sched_event_t events, void *ctx);
#if VERSION_FEATURE_REMOTE
void comm_loop_run(sched_event_t events, void *ctx);
#endif

#endif
