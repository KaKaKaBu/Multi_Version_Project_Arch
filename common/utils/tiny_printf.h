/**
 * @file tiny_printf.h
 * @brief 轻量级 snprintf/vsnprintf 子集（无 libc 浮点/宽字符依赖时可单独链接）。
 *
 * 支持格式：%s %c %d %i %u %x %X %f %g %G %%，以及宽度/零填充/浮点精度。
 * 输出始终有界：最多写入 size-1 字符并补 '\\0'。
 */

#ifndef TINY_PRINTF_H
#define TINY_PRINTF_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 使用 printf 格式符子集将字符串格式化到有限缓冲区。
 * @param[out] buffer 输出缓冲区。
 * @param[in] size 缓冲区容量（字节）。
 * @param[in] fmt 格式字符串（支持 %s、%c、%d、%i、%u、%x、%X、%f、%g、%G、%%）。
 * @param[in] args 可变参数列表。
 * @return 写入的字符数（不含 null）；参数无效时返回 0。
 */
int tiny_vsnprintf(char *buffer, unsigned int size, const char *fmt, va_list args);

/**
 * @brief 将字符串格式化到有限缓冲区（可变参数包装）。
 * @param[out] buffer 输出缓冲区。
 * @param[in] size 缓冲区容量（字节）。
 * @param[in] fmt 格式字符串。
 * @param[in] ... 与 fmt 对应的可变参数。
 * @return 写入的字符数（不含 null）；参数无效时返回 0。
 */
int tiny_snprintf(char *buffer, unsigned int size, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
