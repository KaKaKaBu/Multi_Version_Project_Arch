#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#if defined(PLATFORM_MCS51)
#include "board_config_mcs51.h"
#else
#error "SGTZ_001 is currently configured as an MCS51 polling project."
#endif

#endif
