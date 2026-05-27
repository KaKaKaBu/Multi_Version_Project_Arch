#ifndef SENSOR_IF_H
#define SENSOR_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct temp_hum_sensor {
    const char *name;
    void (*init)(void);
    float (*read_temperature)(void);
    float (*read_humidity)(void);
} temp_hum_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
