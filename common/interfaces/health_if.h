#ifndef HEALTH_IF_H
#define HEALTH_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pulse_oximeter {
    const char *name;
    void (*init)(void);
    unsigned char (*read_heart_rate)(void);
    unsigned char (*read_spo2)(void);
} pulse_oximeter_t;

#ifdef __cplusplus
}
#endif

#endif
