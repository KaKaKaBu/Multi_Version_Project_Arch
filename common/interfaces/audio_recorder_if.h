/**
 * @file audio_recorder_if.h
 * @brief Simple record/playback module interface, e.g. ISD1820.
 */

#ifndef AUDIO_RECORDER_IF_H
#define AUDIO_RECORDER_IF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_recorder_driver {
    const char *name;
    void (*init)(const void *config);
    void (*set_record)(unsigned char on);
    void (*set_play)(unsigned char on);
} audio_recorder_driver_t;

#ifdef __cplusplus
}
#endif

#endif
