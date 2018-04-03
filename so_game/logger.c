#include <stdio.h>
#include <stdarg.h>

void logger_print(const char* who, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    printf("\n[%s]  ", who);
    vprintf(fmt, args);
    va_end(args);
    putchar((int) '\n');
}
