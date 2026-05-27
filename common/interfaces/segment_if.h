#ifndef SEGMENT_IF_H
#define SEGMENT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct segment_display {
    const char *name;
    void (*init)(void);
    void (*show_digits)(unsigned char ten, unsigned char one);
} segment_display_t;

#ifdef __cplusplus
}
#endif

#endif
