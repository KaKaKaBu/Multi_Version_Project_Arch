#ifndef WEIGHT_IF_H
#define WEIGHT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct weight_sensor {
    const char *name;
    void (*init)(void);
    float (*read_grams)(void);
} weight_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
