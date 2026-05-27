#ifndef ACTUATOR_IF_H
#define ACTUATOR_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct servo_driver {
    const char *name;
    void (*init)(void);
    void (*set_angle)(unsigned short angle);
} servo_driver_t;

typedef struct relay_driver {
    const char *name;
    void (*init)(void);
    void (*set_state)(unsigned char on);
} relay_driver_t;

#ifdef __cplusplus
}
#endif

#endif
