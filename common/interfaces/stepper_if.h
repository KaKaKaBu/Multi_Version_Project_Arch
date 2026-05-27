#ifndef STEPPER_IF_H
#define STEPPER_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stepper_driver {
    const char *name;
    void (*init)(void);
    void (*step_angle)(unsigned char direction, unsigned short angle_deg, unsigned short step_delay_ms);
    void (*stop)(void);
} stepper_driver_t;

#ifdef __cplusplus
}
#endif

#endif
