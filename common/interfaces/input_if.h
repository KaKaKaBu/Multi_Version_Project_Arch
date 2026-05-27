#ifndef INPUT_IF_H
#define INPUT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct input_driver {
    const char *name;
    void (*init)(void);
    unsigned char (*read_key)(void);
} input_driver_t;

#ifdef __cplusplus
}
#endif

#endif
