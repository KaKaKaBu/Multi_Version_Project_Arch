#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "scheduler.h"
#include "version_config.h"

/* 页面模式：监测主界面与阈值设置界面。 */
typedef enum {
    SMZL_MODE_MONITOR = 0,
    SMZL_MODE_THRESHOLD = 1
} smzl_mode_t;

/* 阈值设置项：心率、血氧、体温分别有上下限。 */
typedef enum {
    SMZL_THR_HR_UPPER = 0,
    SMZL_THR_HR_LOWER = 1,
    SMZL_THR_SPO2_UPPER = 2,
    SMZL_THR_SPO2_LOWER = 3,
    SMZL_THR_TEMP_UPPER = 4,
    SMZL_THR_TEMP_LOWER = 5
} smzl_threshold_item_t;

/* 应用运行态：采样值、报警状态、翻身统计、阈值和远程遥测节流。 */
typedef struct {
    smzl_mode_t mode;
    smzl_threshold_item_t selected_threshold;
    unsigned char heart_rate;
    unsigned char spo2;
    float temperature;
    unsigned short turnover_count;
    unsigned char display_dirty;
    unsigned char alarm_active;
    unsigned char alarm_output_on;
    unsigned long last_alarm_toggle_tick;
    float last_tilt_degree;
    float stable_tilt_degree;
    unsigned long turnover_cooldown_tick;
    unsigned char hr_upper;
    unsigned char hr_lower;
    unsigned char spo2_upper;
    unsigned char spo2_lower;
    float temp_upper;
    float temp_lower;
#if VERSION_FEATURE_REMOTE
    unsigned char telemetry_pending;
    unsigned long last_telemetry_tick;
#endif
} smzl_context_t;

extern smzl_context_t g_smzl;

/* 业务入口：初始化、逻辑循环和按键处理。 */
void smzl_logic_init(void);
void smzl_logic_loop(sched_event_t events);
void smzl_logic_on_key(unsigned char key_id);

/* sched_loop 回调：传感器、报警闪烁、显示刷新和按键扫描。 */
void sensor_loop_run(sched_event_t events, void *ctx);
void alarm_loop_run(sched_event_t events, void *ctx);
void logic_loop_run(sched_event_t events, void *ctx);
void key_loop_run(sched_event_t events, void *ctx);

#if VERSION_FEATURE_REMOTE
/* 远程入口：WiFi/MQTT 与蓝牙透传最终都进入同一套 JSON 解析逻辑。 */
void comm_loop_run(sched_event_t events, void *ctx);
void smzl_on_remote_rx(const char *json_data);
void smzl_request_telemetry(void);
void smzl_publish_telemetry(void);
#endif

#endif
