#include <cstdio>
#include <cstdarg>

#include "timestamp.hpp"

void logging(const char *fmt, ...)
{
    fprintf(stderr, "[LOG] %s: ", time_now());

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}