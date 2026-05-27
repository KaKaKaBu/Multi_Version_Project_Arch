#ifndef GNSS_IF_H
#define GNSS_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gnss_fix {
    unsigned char valid;
    float latitude;
    float longitude;
    float speed_kmh;
    float course;
    float altitude;
} gnss_fix_t;

typedef struct gnss_driver {
    const char *name;
    void (*init)(void);
    void (*poll)(void);
    void (*get_fix)(gnss_fix_t *fix);
} gnss_driver_t;

#ifdef __cplusplus
}
#endif

#endif
