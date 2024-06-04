#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <math.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define BUFSIZE 2048

#define MAKE_BUF \
  int ph_width = fmt_width - arg_width; \
  int buf_width = fmt_width>arg_width?fmt_width:arg_width; \
  char buf[buf_width]; \
  for (int _i = ph_width-1; _i >= 0 ; _i--) \
    buf[_i] = placeholder; \
  char *buf_start = buf+buf_width-arg_width

#define OUT_BUF \
  int real_width = arg_width<n?arg_width:n; \
  memcpy(out, buf, real_width); \
  out += real_width; \
  n -= real_width


static int width_int(int64_t n, int base) {
    int ret = n < 0 + (base==8?1:(base==16?2:0));
    do ++ret; while ((n /= base) != 0);
    return ret;
}

static int width_uint(uint64_t n, int base) {
    int ret = base==8?1:(base==16?2:0);
    do ++ret; while ((n /= base) != 0);
    return ret;
}

static void out_int(char *out, int64_t n, int base, int width) {
    int prefix = 0;
    if (n < 0) {
        out[prefix] = '-';
        n = -n;
        ++prefix;
    }
    if (base == 8) {
        out[prefix++] = '0';
    } else if (base == 16) {
        out[prefix++] = '0';
        out[prefix++] = 'x';
    }
    prefix += base==8?1:(base==16?2:0);
    for (int i = width-1; i >= prefix; i--, n /= base) {
        int digit = n % base;
        out[i] = (digit>=10)?('a'+digit-10):('0'+digit);
    }
}

static void out_uint(char *out, uint64_t n, int base, int width) {
    int prefix = 0;
    if (base == 8) {
        out[prefix++] = '0';
    } else if (base == 16) {
        out[prefix++] = '0';
        out[prefix++] = 'x';
    }
    for (int i = width-1; i >= prefix; i--, n /= base) {
        int digit = n % base;
        out[i] = (digit>=10)?('a'+digit-10):('0'+digit);
    }
}

int printf(const char *fmt, ...) {
  char buffer[BUFSIZE];
  va_list pArgs;
  va_start(pArgs, fmt);
  int ret = vsprintf(buffer, fmt, pArgs);
  va_end(pArgs);
  for (int i = 0; i < ret; i++) putch(buffer[i]);
  return ret;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list pArgs;
  va_start(pArgs, fmt);
  int ret = vsprintf(out, fmt, pArgs);
  va_end(pArgs);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list pArgs;
  va_start(pArgs, fmt);
  int ret = vsnprintf(out, n, fmt, pArgs);
  va_end(pArgs);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, UINT32_MAX, fmt, ap);
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    char *start = out;
    bool ps = false; // has meet percent sign '%'
    --n; // for '\0'
    char placeholder = ' ';
    int fmt_width = 0;

    for (; n && *fmt; ++fmt) {
        if (!ps) {
            if (*fmt == '%') ps = true;
            else {
                *out++ = *fmt;
                --n;
            }
            continue;
        }
        switch (*fmt) {
            case '%': *out++ = *fmt; --n; break;
            case 'd': {
                int64_t arg = va_arg(ap, int64_t);
                int arg_width = width_int(arg, 10);
                MAKE_BUF;
                out_int(buf_start, arg, 10, arg_width);
                OUT_BUF;
                break;
            }
            case 'u': {
                uint64_t arg = va_arg(ap, uint64_t);
                int arg_width = width_uint(arg, 10);
                MAKE_BUF;
                out_int(buf_start, arg, 10, arg_width);
                OUT_BUF;
                break;
            }
            case 'l': { 
                continue;
            }
            case 'x': case 'p': {
                uint64_t arg = va_arg(ap, uint64_t);
                int arg_width = width_uint(arg, 16);
                MAKE_BUF;
                out_uint(buf_start, arg, 16, arg_width);
                OUT_BUF;
                break;
            }
            case 's': {
                char *arg = va_arg(ap, char*);
                int arg_width = strlen(arg);
                MAKE_BUF;
                strcpy(buf_start, arg);
                OUT_BUF;
                break;
            }
            case 'c': {
                char arg = va_arg(ap, int);
                *out++ = arg;
                --n;
                break;
            }
            default: {
                char c = *fmt;
                if (c >= '0' && c <= '9') { // e.g. %09d, %9d, %6s
                    placeholder = c=='0'?'0':' ';
                    fmt_width = 0;
                    if (placeholder == '0') ++fmt;
                    while (*fmt >= '0' && *fmt <= '9') {
                        fmt_width *= 10;
                        fmt_width += *fmt-'0';
                        ++fmt;
                    }
                    --fmt;
                }
                continue;
            }
        }
        ps = false;
        fmt_width = 0;
    }
    *out++ = '\0';
    return out-start;
}

int puts(const char * str) {
  for (const char *c = str; *c; c++) putch(*c);
  putch('\n');
  return 0;
}

#endif
