#ifndef GAS_IF_H
#define GAS_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gas_sensor {
    const char *name;
    void (*init)(void);
    unsigned short (*read_ppm)(void);
} gas_sensor_t;

#ifdef __cplusplus
}
#endif

#endif
