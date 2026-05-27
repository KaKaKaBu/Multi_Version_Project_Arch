#ifndef CJSON_PORT_H
#define CJSON_PORT_H

#include "cJSON.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void cjson_port_init(void);

cJSON *cjson_parse(const char *text, size_t length);
void cjson_release(cJSON *item);

void cjson_release_string(char *text);

char *cjson_build_telemetry(const char *device, float temperature, float humidity);

#ifdef __cplusplus
}
#endif

#endif
