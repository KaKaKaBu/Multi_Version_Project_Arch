#ifndef COMM_IF_H
#define COMM_IF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum comm_kind {
    COMM_KIND_STREAM = 0U,
    COMM_KIND_PACKET = 1U
} comm_kind_t;

typedef void (*comm_rx_callback_t)(const unsigned char *data, unsigned short len);

typedef struct comm_driver {
    const char *name;
    comm_kind_t kind;
    void (*init)(void);
    int (*send)(const unsigned char *data, unsigned short len);
    int (*recv)(unsigned char *buf, unsigned short max_len);
    void (*register_rx_callback)(comm_rx_callback_t callback);
} comm_driver_t;

#ifdef __cplusplus
}
#endif

#endif
