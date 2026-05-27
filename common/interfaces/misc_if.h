#ifndef MISC_IF_H
#define MISC_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct misc_driver {
    const char *name;
    void (*init)(void);
    void (*set_state)(unsigned char on);
} misc_driver_t;

#ifdef __cplusplus
}
#endif

#endif
