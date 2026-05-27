#ifndef RFID_IF_H
#define RFID_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rfid_driver {
    const char *name;
    void (*init)(void);
    unsigned char (*poll_card)(void);
    void (*get_uid)(unsigned char *uid, unsigned char max_len, unsigned char *uid_len);
    void (*clear_card)(void);
} rfid_driver_t;

#ifdef __cplusplus
}
#endif

#endif
