#include "cjson_port.h"
#include <stdlib.h>

void cjson_port_init(void)
{
    cJSON_Hooks hooks;

    hooks.malloc_fn = malloc;
    hooks.free_fn = free;
    cJSON_InitHooks(&hooks);
}

cJSON *cjson_parse(const char *text, size_t length)
{
    if ((text == NULL) || (length == 0U)) {
        return NULL;
    }

    return cJSON_ParseWithLength(text, length);
}

void cjson_release(cJSON *item)
{
    cJSON_Delete(item);
}

void cjson_release_string(char *text)
{
    cJSON_free(text);
}

char *cjson_build_telemetry(const char *device, float temperature, float humidity)
{
    cJSON *root;
    char *json_text;

    root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    if ((device != NULL) && (cJSON_AddStringToObject(root, "device", device) == NULL)) {
        cJSON_Delete(root);
        return NULL;
    }

    if (cJSON_AddNumberToObject(root, "temperature", temperature) == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    if (cJSON_AddNumberToObject(root, "humidity", humidity) == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_text;
}
