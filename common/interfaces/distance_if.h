#ifndef DISTANCE_IF_H
#define DISTANCE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct distance_sensor {
    const char *name;
    void (*init)(void);
    unsigned short (*read_distance_cm)(void);
} distance_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
