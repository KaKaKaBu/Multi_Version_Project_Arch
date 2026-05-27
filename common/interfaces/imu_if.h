#ifndef IMU_IF_H
#define IMU_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct imu_sample {
    short ax;
    short ay;
    short az;
    short gx;
    short gy;
    short gz;
} imu_sample_t;

typedef struct imu_sensor {
    const char *name;
    void (*init)(void);
    void (*read_sample)(imu_sample_t *sample);
} imu_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
