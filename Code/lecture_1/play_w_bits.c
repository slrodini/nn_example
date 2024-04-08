#include <stdio.h>
#include <stdint.h>

#define S_MASK 0b10000000000000000000000000000000
#define E_MASK 0b01111111100000000000000000000000
#define M_MASK 0b00000000011111111111111111111111
#define _EXIT_()                                                                                                                                     \
   {                                                                                                                                                 \
      int toExit = 0;                                                                                                                                \
      while (1 != toExit) {                                                                                                                          \
         fprintf(stderr, "Do you want to exit? 0: No, 1: Yes, >1: Undefined\n");                                                                     \
         fscanf(stdin, "%d", &toExit);                                                                                                               \
      }                                                                                                                                              \
   }

static void print_bin_int32_for_float(int32_t a)
{
   static char bin_rep[33] = {0};
   for (int32_t i = 0; i < 32; i++) {
      bin_rep[i] = ((a & (1 << (31 - i))) ? '1' : '0');
   }
   // divides sign bit, exponent (8 bits), mantissa (23 bits)
   for (int32_t i = 0; i < 1; i++)
      printf("%c", bin_rep[i]);
   printf("|");
   for (int32_t i = 1; i < 9; i++)
      printf("%c", bin_rep[i]);
   printf("|");
   for (int32_t i = 9; i < 32; i++)
      printf("%c", bin_rep[i]);
   printf("|");
   printf("\n");
}

void print_float(float x)
{
   int32_t a = *(int32_t *)&x;

   int32_t sign = (S_MASK & a) >> (32 - 1);
   int32_t exponent = ((E_MASK & a) >> (32 - 1 - 8)) - 127;
   int32_t mantissa = (M_MASK & a);

   printf("The original number: %f\nThe binary division of the number: ", x);
   print_bin_int32_for_float(a);
   printf("The number representation: %s2^{%d} x (1", sign ? "-" : "+", exponent);
   for (int32_t i = 22; i >= 0; i--) {
      if (mantissa & (1 << (i))) {
         printf(" + 2^{-%d}", 23 - i);
      }
   }
   printf(")\n");
}

int main()
{
   float x = 3;

   fprintf(stderr,"Please input your floating point number: \t");
   fscanf(stdin, "%f", &x);
   
   print_float(x);

   _EXIT_();
   return 0;
}