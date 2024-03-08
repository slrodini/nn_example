#ifdef __cplusplus
extern "C" {
#endif

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <stdint.h>

#define NN_OK 0
#define NN_INFO 0
#define NN_WARNING 1
#define NN_ERROR 2
#define NN_ERR_NULL_PTR 2
#define NN_ERR_OUT_OF_BOUND 3
#define NN_NO_FILE 4
#define NN_ERR_WRONG_TYPE 5
#define NN_ERR_FULL_STACK 6
#define NN_ERR_STACK_TOO_SMALL 7
#define NN_ERR_EMPTY_STACK 8
#define NN_ERR_FAILED_READ 9
#define NN_ERR_FAILED_WRITE 10

#endif

#ifdef __cplusplus
}
#endif
