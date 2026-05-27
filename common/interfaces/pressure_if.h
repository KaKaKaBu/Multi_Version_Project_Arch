#ifndef PRESSURE_IF_H
#define PRESSURE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pressure_sensor {
    const char *name;
    void (*init)(void);
    float (*read_pressure_pa)(void);
    float (*read_temperature)(void);
} pressure_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
