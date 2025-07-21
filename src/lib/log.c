#include <stdio.h>
#include <stdarg.h>
#include "lib/log.h"

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stdout, "\033[39m[INFO ]\033[0m ");
    vfprintf(stdout, format, args);
    va_end(args);
}

void log_warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[33m[WARN ]\033[0m ");
    vfprintf(stderr, format, args);
    va_end(args);
}

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[31m[ERROR]\033[0m ");
    vfprintf(stderr, format, args);
    va_end(args);
}