#include <stdio.h>
#include <stdarg.h>

#include <stdio.h>
#include <stdarg.h>

#define VERBOSE 0
#define ERROR 1

void logger_verbose(const char* who, const char *fmt, ...){
    if(VERBOSE){
        va_list args;
        va_start(args, fmt);
        fprintf(stdout, "\n[%s]  ", who);
        vfprintf(stdout, fmt, args);
        va_end(args);
        fprintf(stdout, "\n");
    }
}


void logger_error(const char* who, const char *fmt, ...){
    if(ERROR){
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "\n[%s]  ", who);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}
