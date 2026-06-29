#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "scheduler.h"
#include "version_config.h"

/* 页面/控制模式：自动晾晒、手动开合、阈值设置。 */
typedef enum {
    ZNLYJ_MODE_AUTO = 0,
    ZNLYJ_MODE_MANUAL = 1,
    ZNLYJ_MODE_THRESHOLD = 2
} znlyj_mode_t;

/* 阈值设置项：v2+ 增加光照和重量阈值。 */
typedef enum {
    ZNLYJ_THR_TEMP = 0,
    ZNLYJ_THR_HUMIDITY = 1,
#if VERSION_FEATURE_LIGHT
    ZNLYJ_THR_LIGHT = 2,
    ZNLYJ_THR_WEIGHT = 3
#else
    ZNLYJ_THR_COUNT_END = 2
#endif
} znlyj_threshold_item_t;

/* 应用运行态：保存传感器采样、晾衣架状态、阈值、报警和远程遥测状态。 */
typedef struct {
    znlyj_mode_t mode;
#if VERSION_FEATURE_LIGHT
    znlyj_threshold_item_t selected_threshold;
#else
    unsigned char selected_threshold;
#endif
    unsigned char rack_open;
    float temperature;
    float humidity;
    unsigned char human_present;
    unsigned char clothes_present;
#if VERSION_FEATURE_LIGHT
    float light_percent;
#endif
#if VERSION_FEATURE_WEIGHT
    float weight_kg;
    unsigned char weight_overload;
#endif
    float temp_threshold;
    float humidity_threshold;
#if VERSION_FEATURE_LIGHT
    float light_threshold;
    float weight_threshold;
#endif
    unsigned char display_dirty;
    unsigned char alarm_active;
    unsigned char alarm_on;
    unsigned long last_alarm_toggle_tick;
#if VERSION_FEATURE_REMOTE
    unsigned char telemetry_pending;
    unsigned long last_telemetry_tick;
#endif
} znlyj_context_t;

extern znlyj_context_t g_znlyj;

/* 业务入口：初始化与按键处理，其余周期任务由 sched_loop 调用。 */
void znlyj_logic_init(void);
void znlyj_logic_on_key(unsigned char key_id);

/* sched_loop 回调：传感器、报警闪烁、显示刷新和按键扫描。 */
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void logic_loop_run(sched_event_t events, void *ctx);
void key_loop_run(sched_event_t events, void *ctx);

#if VERSION_FEATURE_REMOTE
/* 远程入口：WiFi/MQTT 与蓝牙透传最终都进入同一套 JSON 解析逻辑。 */
void comm_loop_run(sched_event_t events, void *ctx);
void znlyj_on_remote_rx(const char *json_data);
void znlyj_request_telemetry(void);
void znlyj_publish_telemetry(void);
#endif

#endif
