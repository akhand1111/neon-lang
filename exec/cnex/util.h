#ifndef UTIL_H
#define UTIL_H

#define STRING_BUFFER_SIZE      120
#define estr(x)                 #x
#define ENUM(x)                 estr(x)

void exec_error(const char *msg, ...);
void fatal_error(const char *msg, ...);

typedef enum { FALSE, TRUE } BOOL;

#endif
