
/*
 * Type definitions for error functions.
 */
extern int errors;
extern int warnings;
void fatal(char *fmt, ...);
void error(char *fmt, ...);
void warning(char *fmt, ...);
void debug(char *fmt, ...);
