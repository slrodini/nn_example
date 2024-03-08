#ifdef __cplusplus
extern "C" {
#endif

#include <default.h>

static void local_error_handling(uint8_t);

void (*nn_error_handling)(uint8_t) = local_error_handling;

static void nn_print_current_error_code(uint8_t errcode)
{
   switch (errcode) {
   case NN_ERROR: {
      fprintf(stderr, "[ERROR] code: %d => NULL pointer\n", errcode);
      break;
   }
   default:
      break;
   }
}

static void local_error_handling(uint8_t errcode)
{
   static uint8_t current_error_code = NN_OK;
   current_error_code = errcode;
   nn_print_current_error_code(current_error_code);
}

void nn_set_local_error_handling(void (*f)(uint8_t)) { nn_error_handling = f; }

void nn_log(uint8_t errcode, char *msg, ...)
{
   static char const messages[3][255] = {"[INFO]", "[WARNING]", "[ERROR]"};
   static char const colors[3][255] = {"\033[0;32m", "\033[0;33m", "\033[0;31m"};

   static char buffer[1024] = "";
   va_list argptr;
   va_start(argptr, msg);
   vsprintf(buffer, msg, argptr);
   va_end(argptr);

   size_t level = errcode > 1 ? 2 : errcode;
   bool to_exit = level == 2;

   // if (to_exit) nn_error_handling(errcode);

   fprintf(stderr, "%s", colors[level]);
   fprintf(stderr, "%s  ", messages[level]);
   fprintf(stderr, "%s", "\033[0m"); // Default color
   fprintf(stderr, "%s\n", buffer);
   // fprintf(stderr, "%s %s\n", messages[level], buffer);
   if (to_exit) exit(errcode);
}

double min2(double a, double b) { return (a < b ? a : b); }

#ifdef __cplusplus
}
#endif