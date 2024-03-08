#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFAULT_H
#define DEFAULT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <useful_constants.h>
#include <error_codes.h>

#ifdef __GNUC__
#define _max_(a, b)                                                                                                                                  \
   ({                                                                                                                                                \
      __typeof__(a) _a = (a);                                                                                                                        \
      __typeof__(b) _b = (b);                                                                                                                        \
      _a > _b ? _a : _b;                                                                                                                             \
   })
#define _min_(a, b)                                                                                                                                  \
   ({                                                                                                                                                \
      __typeof__(a) _a = (a);                                                                                                                        \
      __typeof__(b) _b = (b);                                                                                                                        \
      _a < _b ? _a : _b;                                                                                                                             \
   })

#define _max3_(a, b, c) _max_((a), _max_((b), (c)))
#define _min3_(a, b, c) _min_((a), _min_((b), (c)))
#elif
#define _max_(a, b) ((a) > (b) ? (a) : (b))
#define _min_(a, b) ((a) < (b) ? (a) : (b))
#define _max3_(a, b, c) (_max_((a), _max_((b), (c))))
#define _min3_(a, b, c) (_min_((a), _min_((b), (c))))
#endif

#define SWAP(a, b, T)                                                                                                                                \
   {                                                                                                                                                 \
      T temp = a;                                                                                                                                    \
      a = b;                                                                                                                                         \
      b = temp;                                                                                                                                      \
   }

// Dynamic array implementation
#define DA_APPEND(str, item)                                                                                                                         \
   {                                                                                                                                                 \
      (str)->content[(str)->count] = (item);                                                                                                         \
      (str)->count++;                                                                                                                                \
      if ((str)->count >= (str)->size) {                                                                                                             \
         (str)->size += DA_INCREMENT;                                                                                                                \
         (str)->content = realloc((str)->content, (str)->size * sizeof((str)->content[0]));                                                          \
         memset((str)->content + (str)->count, 0, ((str)->size - (str)->count) * sizeof((str)->content[0]));                                         \
      }                                                                                                                                              \
   }

#define STR_APPEND_NULL(str) DA_APPEND(str, '\0')

#define DA_APPEND_MANY(str, items, how_many)                                                                                                         \
   {                                                                                                                                                 \
      bool _to_realloc_ = false;                                                                                                                     \
      while ((str)->count + (how_many) >= (str)->size) {                                                                                             \
         (str)->size += DA_INCREMENT;                                                                                                                \
         _to_realloc_ = true;                                                                                                                        \
      }                                                                                                                                              \
      if (_to_realloc_) {                                                                                                                            \
         (str)->content = realloc((str)->content, (str)->size * sizeof((str)->content[0]));                                                          \
         printf("REALLOC\n");                                                                                                                        \
      }                                                                                                                                              \
      memcpy((str)->content + (str)->count, items, how_many);                                                                                        \
      (str)->count += (how_many);                                                                                                                    \
   }

// for (uint64_t j = 0; j < (how_many); j++) {
//    (str)->content[(str)->count + j] = items[j];
// }

#define DA_STATIC_INIT(str, T)                                                                                                                       \
   (str)->content = (T *)calloc(DA_INCREMENT, sizeof(T));                                                                                            \
   (str)->count = 0;                                                                                                                                 \
   (str)->size = DA_INCREMENT;

// Does not contemplate the possibility of calling it with already initialized content pointer!
#define DA_INIT(str, Tstr, T)                                                                                                                        \
   if (NULL == str) {                                                                                                                                \
      (str) = (Tstr *)calloc(1, sizeof(Tstr));                                                                                                       \
      (str)->content = (T *)calloc(DA_INCREMENT, sizeof(T));                                                                                         \
      (str)->count = 0;                                                                                                                              \
      (str)->size = DA_INCREMENT;                                                                                                                    \
   } else {                                                                                                                                          \
      DA_STATIC_INIT(str, T)                                                                                                                         \
   }

// Macromagic to get a string that contains the name of the variable
#define GET_VAR_NAME(var) #var
// Macromagic to concatenate the line number to prefix to generate unique variable name
// #define CONCAT_INTERNAL(prefix, suffix) prefix##suffix
// #define CONCAT_(prefix, suffix) CONCAT_INTERNAL(prefix, suffix)
// #define CONCAT(prefix) CONCAT_(prefix##_, __LINE__)
// #define defer(start, end) for ((int32_t CONCAT(_i_) = 0), start; !(CONCAT(_i_)); (CONCAT(_i_)++), end)
#define TIME_FUNCTION(start, end) for (int32_t _i_ = 0, start = clock(); !_i_; _i_++, end = clock())

// Common stack for various routines. Exposed only to internal translation units
#ifndef _DEFAULT_EXTERNAL_

// 64 MB of custom stack
#define _STACK_SIZE_ (1L << 26)
extern char _stack_[_STACK_SIZE_];
extern const char *_stack_first_;
extern const char *_stack_last_;
extern char *_stack_ptr_;

#define _PUSH_TO_STACK_(value, type)                                                                                                                 \
   {                                                                                                                                                 \
      if (_stack_ptr_ + sizeof(type) > _stack_last_) nn_log(NN_ERR_FULL_STACK, "Stack is full.");                                                    \
      *(type *)_stack_ptr_ = (value);                                                                                                                \
      _stack_ptr_ += sizeof(type);                                                                                                                   \
   }

#define _POP_FROM_STACK_(dest, type)                                                                                                                 \
   {                                                                                                                                                 \
      if (_stack_ptr_ - sizeof(type) < _stack_first_) nn_log(NN_ERR_STACK_TOO_SMALL, "Trying to pop from stack too much memory.");                   \
      _stack_ptr_ -= sizeof(type);                                                                                                                   \
      dest = *((type *)_stack_ptr_);                                                                                                                 \
   }

// A bit faster but less safe
// Good only after full debug and test
/*
// #define _PUSH_TO_STACK_(value, type) \
//    { \
//       *(type *)_stack_ptr_ = (value); \
//       _stack_ptr_ += sizeof(type); \
//    }

// #define _POP_FROM_STACK_(dest, type) \
//    { \
//       _stack_ptr_ -= sizeof(type); \
//       dest = *((type *)_stack_ptr_); \
//    }
*/

#endif //_DEFAULT_EXTERNAL_
// Loggin stuff
typedef enum { PL_NONE, PL_ESSENTIAL, PL_ALL } nn_printout_level_e;

void nn_log(uint8_t errcode, char *msg, ...);
void nn_set_local_error_handling(void (*f)(uint8_t));

double min2(double a, double b);

#endif
#ifdef __cplusplus
}
#endif
