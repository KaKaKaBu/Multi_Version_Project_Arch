#include "driver_core.h"

#if !defined(DRIVER_REGISTRY_STATIC)

const driver_registry_entry_t *driver_registry_begin(void)
{
    return __driver_list_start;
}

const driver_registry_entry_t *driver_registry_end(void)
{
    return __driver_list_end;
}

const board_device_entry_t *board_device_registry_begin(void)
{
    return __board_device_list_start;
}

const board_device_entry_t *board_device_registry_end(void)
{
    return __board_device_list_end;
}

#endif
