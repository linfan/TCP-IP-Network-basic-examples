#include <stdarg.h>      // va_list
#include <stdio.h>
#include <stdlib.h>

// Print error information
int errexit(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

