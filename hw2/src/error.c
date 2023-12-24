/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int errors;
int warnings;
int dbflag = 1;

void fatal(char* fmt, ...) {
        va_list msg;
        va_start (msg, fmt);

        fprintf(stderr, "\nFatal error: ");
        vfprintf(stderr, fmt, msg);
        fprintf(stderr, "\n");

        va_end(msg);
        exit(1);
}

void error(char* fmt, ...) {
        va_list msg;
        va_start (msg, fmt);

        fprintf(stderr, "\nError: ");
        vfprintf(stderr, fmt, msg);
        fprintf(stderr, "\n");
        errors++;

        va_end(msg);
}

void warning(char* fmt, ...) {
        va_list msg;
        va_start (msg, fmt);

        fprintf(stderr, "\nWarning: ");
        vfprintf(stderr, fmt, msg);
        fprintf(stderr, "\n");
        warnings++;

        va_end(msg);
}

void debug(char* fmt, ...) {
        va_list msg;
        va_start (msg, fmt);

        if(!dbflag) return;
        fprintf(stderr, "\nDebug: ");
        vfprintf(stderr, fmt, msg);
        fprintf(stderr, "\n");

        va_end(msg);
}
