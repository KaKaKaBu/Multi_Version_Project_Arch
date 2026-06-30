/**
 * @file syscalls.c
 * @brief Newlib syscall stubs for bare-metal STM32 (stdio routed to debug UART).
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "debug_uart.h"

/**
 * @brief Writes bytes to stdout or stderr via the debug UART.
 * @param file File descriptor (1 = stdout, 2 = stderr).
 * @param ptr Buffer containing data to transmit.
 * @param len Number of bytes to write.
 * @return Number of bytes written on success, or len when the request is ignored.
 */
int _write(int file, char *ptr, int len)
{
    if ((ptr != 0) && (len > 0) && ((file == 1) || (file == 2))) {
        return debug_uart_write(ptr, len);
    }

    (void)file;
    (void)ptr;
    return len;
}

/**
 * @brief Stub read syscall; no input device is attached.
 * @param file File descriptor (unused).
 * @param ptr Receive buffer (unused).
 * @param len Maximum bytes to read (unused).
 * @return Always 0 (no data available).
 */
int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/**
 * @brief Stub close syscall.
 * @param file File descriptor (unused).
 * @return Always 0.
 */
int _close(int file)
{
    (void)file;
    return 0;
}

/**
 * @brief Stub seek syscall.
 * @param file File descriptor (unused).
 * @param ptr Offset (unused).
 * @param dir Seek origin (unused).
 * @return Always 0.
 */
int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

/**
 * @brief Stub fstat syscall; reports a character device.
 * @param file File descriptor (unused).
 * @param st Stat structure to populate.
 * @return Always 0.
 */
int _fstat(int file, struct stat *st)
{
    (void)file;
    if (st != 0) {
        st->st_mode = S_IFCHR;
    }
    return 0;
}

/**
 * @brief Stub isatty syscall; all fds are treated as TTYs.
 * @param file File descriptor (unused).
 * @return Always 1.
 */
int _isatty(int file)
{
    (void)file;
    return 1;
}

/**
 * @brief Stub getpid syscall.
 * @return Always 1.
 */
int _getpid(void)
{
    return 1;
}

/**
 * @brief Stub kill syscall; always fails with EINVAL.
 * @param pid Target process ID (unused).
 * @param sig Signal number (unused).
 * @return Always -1.
 */
int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/**
 * @brief Terminates the process in an infinite loop (no return to caller).
 * @param status Exit code (unused).
 */
void _exit(int status)
{
    (void)status;
    for (;;) {
    }
}
