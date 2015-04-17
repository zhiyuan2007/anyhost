#ifndef _DIG_UTIL_H_
#define _DIG_UTIL_H_

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <time.h>


#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define CHECK(func) \
do {\
    ret = func; \
    if (ret != SUCCESS) \
    {\
        TRACE(DIG_ERROR, #func, ret); \
        return ret;\
    } \
}while (0);

#define DIG_MALLOC(p,size) do {\
    (p) = malloc((size));\
    memset((p), 0, (size));\
}while (0)

#define DIG_REALLOC(p, new_size) do{\
    (p) = realloc((p), new_size);\
    ASSERT((p),"realloc memory failed");\
}while (0)

#define DIG_CALLOC(p, count, size) do {\
    (p) = calloc((count), (size));\
    ASSERT((p),"calloc memory failed");\
}while (0)


#define ASSERT(cond,...)    do{ \
    if (!(cond))  {\
        char str[200];\
        sprintf(str, __VA_ARGS__);\
        printf("%s in file %s at line %d\n", str, __FILE__, __LINE__);\
        exit(1);    \
    }\
}while (0)


#endif
