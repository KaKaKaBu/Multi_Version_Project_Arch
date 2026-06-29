#include "driver_core.h"

#if defined(DRIVER_REGISTRY_STATIC)

extern const driver_registry_entry_t driver_registry_static[];
extern const unsigned short driver_registry_static_count;

const driver_registry_entry_t *driver_registry_begin(void)
{
    return driver_registry_static;
}

const driver_registry_entry_t *driver_registry_end(void)
{
    return driver_registry_static + driver_registry_static_count;
}

#endif
