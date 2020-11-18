#include "raler.h"
#include <stdio.h>
#include <stdlib.h>



noreturn void raler (int syserr, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr) perror ("");
    exit (1);
}