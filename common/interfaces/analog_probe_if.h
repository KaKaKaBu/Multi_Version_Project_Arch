#ifndef ANALOG_PROBE_IF_H
#define ANALOG_PROBE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct analog_probe {
    const char *name;
    void (*init)(void);
    float (*read_value)(void);
} analog_probe_t;

#ifdef __cplusplus
}
#endif

#endif
