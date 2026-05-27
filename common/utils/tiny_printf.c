#include "tiny_printf.h"

#define TINY_PRINTF_MAX_FLOAT_PREC 9U

typedef struct tiny_fmt_spec {
    unsigned char width;
    unsigned char precision;
    unsigned char precision_set;
    unsigned char zero_pad;
} tiny_fmt_spec_t;

static unsigned int tiny_append_char(char *buffer, unsigned int size, unsigned int pos, char ch)
{
    if ((buffer != 0) && (pos + 1U < size)) {
        buffer[pos] = ch;
    }

    return pos + 1U;
}

static unsigned int tiny_append_string(char *buffer, unsigned int size, unsigned int pos, const char *text)
{
    if (text == 0) {
        text = "(null)";
    }

    while (*text != '\0') {
        pos = tiny_append_char(buffer, size, pos, *text);
        ++text;
    }

    return pos;
}

static unsigned int tiny_append_uint_core(char *buffer, unsigned int size, unsigned int pos,
                                        unsigned long value, unsigned char base, unsigned char uppercase)
{
    char digits[16];
    unsigned char count = 0U;
    unsigned char i;

    if (value == 0UL) {
        return tiny_append_char(buffer, size, pos, '0');
    }

    while (value > 0UL) {
        unsigned long digit = value % (unsigned long)base;
        digits[count++] = (char)((digit < 10UL) ? ('0' + digit)
                                               : ((uppercase != 0U) ? ('A' + (digit - 10UL))
                                                                    : ('a' + (digit - 10UL))));
        value /= (unsigned long)base;
    }

    for (i = count; i > 0U; --i) {
        pos = tiny_append_char(buffer, size, pos, digits[i - 1U]);
    }

    return pos;
}

static unsigned int tiny_append_uint_padded(char *buffer, unsigned int size, unsigned int pos,
                                          unsigned long value, unsigned char base, unsigned char uppercase,
                                          const tiny_fmt_spec_t *spec)
{
    char digits[16];
    unsigned char count = 0U;
    unsigned char i;
    unsigned char pad;
    unsigned long temp = value;

    if (temp == 0UL) {
        count = 1U;
        digits[0] = '0';
    } else {
        while (temp > 0UL) {
            unsigned long digit = temp % (unsigned long)base;
            digits[count++] = (char)((digit < 10UL) ? ('0' + digit)
                                                   : ((uppercase != 0U) ? ('A' + (digit - 10UL))
                                                                        : ('a' + (digit - 10UL))));
            temp /= (unsigned long)base;
        }
    }

    if ((spec != 0) && (spec->width > count)) {
        pad = (unsigned char)(spec->width - count);
        for (i = 0U; i < pad; ++i) {
            pos = tiny_append_char(buffer, size, pos, (spec->zero_pad != 0U) ? '0' : ' ');
        }
    }

    for (i = count; i > 0U; --i) {
        pos = tiny_append_char(buffer, size, pos, digits[i - 1U]);
    }

    return pos;
}

static unsigned int tiny_append_int(char *buffer, unsigned int size, unsigned int pos, long value)
{
    if (value < 0L) {
        pos = tiny_append_char(buffer, size, pos, '-');
        value = -value;
    }

    return tiny_append_uint_core(buffer, size, pos, (unsigned long)value, 10U, 0U);
}

static unsigned int tiny_append_float(char *buffer, unsigned int size, unsigned int pos, double value,
                                    unsigned char precision)
{
    long whole;
    unsigned long frac;
    unsigned long scale = 1UL;
    unsigned char i;

    if (precision > TINY_PRINTF_MAX_FLOAT_PREC) {
        precision = TINY_PRINTF_MAX_FLOAT_PREC;
    }

    for (i = 0U; i < precision; ++i) {
        scale *= 10UL;
    }

    if (value < 0.0) {
        pos = tiny_append_char(buffer, size, pos, '-');
        value = -value;
    }

    whole = (long)value;
    frac = (unsigned long)((value - (double)whole) * (double)scale + 0.5);
    if (frac >= scale) {
        whole += 1L;
        frac = 0UL;
    }

    pos = tiny_append_int(buffer, size, pos, whole);
    if (precision > 0U) {
        unsigned long pad = scale / 10UL;

        pos = tiny_append_char(buffer, size, pos, '.');
        while (pad > 0UL) {
            if (frac < pad) {
                pos = tiny_append_char(buffer, size, pos, '0');
            }
            pad /= 10UL;
        }
        pos = tiny_append_uint_core(buffer, size, pos, frac, 10U, 0U);
    }

    return pos;
}

static void tiny_parse_spec(const char **fmt, tiny_fmt_spec_t *spec)
{
    spec->width = 0U;
    spec->precision = 0U;
    spec->precision_set = 0U;
    spec->zero_pad = 0U;

    if ((**fmt == '0') && ((*fmt)[1] >= '0') && ((*fmt)[1] <= '9')) {
        spec->zero_pad = 1U;
    }

    while ((**fmt >= '0') && (**fmt <= '9')) {
        spec->width = (unsigned char)(spec->width * 10U + (unsigned char)(**fmt - '0'));
        ++(*fmt);
    }

    if (**fmt == '.') {
        ++(*fmt);
        spec->precision_set = 1U;
        while ((**fmt >= '0') && (**fmt <= '9')) {
            spec->precision = (unsigned char)(spec->precision * 10U + (unsigned char)(**fmt - '0'));
            ++(*fmt);
        }
    }
}

int tiny_vsnprintf(char *buffer, unsigned int size, const char *fmt, va_list args)
{
    unsigned int pos = 0U;
    tiny_fmt_spec_t spec;

    if ((buffer == 0) || (size == 0U) || (fmt == 0)) {
        return 0;
    }

    while (*fmt != '\0') {
        if (*fmt != '%') {
            pos = tiny_append_char(buffer, size, pos, *fmt);
            ++fmt;
            continue;
        }

        ++fmt;

        if (*fmt == '%') {
            pos = tiny_append_char(buffer, size, pos, '%');
            ++fmt;
            continue;
        }

        tiny_parse_spec(&fmt, &spec);

        switch (*fmt) {
        case 's':
            pos = tiny_append_string(buffer, size, pos, va_arg(args, const char *));
            break;
        case 'c':
            pos = tiny_append_char(buffer, size, pos, (char)va_arg(args, int));
            break;
        case 'u':
            pos = tiny_append_uint_padded(buffer, size, pos, va_arg(args, unsigned int), 10U, 0U, &spec);
            break;
        case 'd':
        case 'i':
            pos = tiny_append_int(buffer, size, pos, va_arg(args, int));
            break;
        case 'x':
            pos = tiny_append_uint_padded(buffer, size, pos, va_arg(args, unsigned int), 16U, 0U, &spec);
            break;
        case 'X':
            pos = tiny_append_uint_padded(buffer, size, pos, va_arg(args, unsigned int), 16U, 1U, &spec);
            break;
        case 'f':
            if (spec.precision_set == 0U) {
                spec.precision = 1U;
            }
            pos = tiny_append_float(buffer, size, pos, va_arg(args, double), spec.precision);
            break;
        case 'g':
        case 'G':
            if (spec.precision_set == 0U) {
                spec.precision = 6U;
            }
            if (spec.precision > TINY_PRINTF_MAX_FLOAT_PREC) {
                spec.precision = TINY_PRINTF_MAX_FLOAT_PREC;
            }
            pos = tiny_append_float(buffer, size, pos, va_arg(args, double), spec.precision);
            break;
        default:
            pos = tiny_append_char(buffer, size, pos, *fmt);
            break;
        }

        ++fmt;
    }

    if (pos < size) {
        buffer[pos] = '\0';
    } else {
        buffer[size - 1U] = '\0';
    }

    return (int)pos;
}

int tiny_snprintf(char *buffer, unsigned int size, const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);
    length = tiny_vsnprintf(buffer, size, fmt, args);
    va_end(args);

    return length;
}
