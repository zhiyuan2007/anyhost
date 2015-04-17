#ifndef _H_DIG_ERROR_H_
#define _H_DIG_ERROR_H_ 1

#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    FAILED  = -1,
    SUCCEED = 0
} result_t;

const char *    result_to_str(result_t res);

#endif /* _H_DIG_ERROR_H_ */
