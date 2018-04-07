#include <stdio.h>
#include <stdarg.h>

#include <stdio.h>
#include <stdarg.h>

void logger_verbose(const char* who, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "\n[%s]  ", who);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}


void logger_error(const char* who, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\n[%s]  ", who);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
