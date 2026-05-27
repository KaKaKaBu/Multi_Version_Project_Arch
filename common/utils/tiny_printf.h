#ifndef TINY_PRINTF_H
#define TINY_PRINTF_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int tiny_vsnprintf(char *buffer, unsigned int size, const char *fmt, va_list args);
int tiny_snprintf(char *buffer, unsigned int size, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
