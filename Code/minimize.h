#ifdef __cplusplus
extern "C" {
#endif

#include <default.h>
#include <adam.h>
#include <annealing.h>

double minimize(double *par, int32_t nPar, void *addPar, void (*fgrad)(double[], size_t, void *, double *, double *));
double minimize2(double *par, int32_t nPar, void *addPar, double (*fnc)(double *, size_t, void *),
                 void (*fgrad)(double[], size_t, void *, double *, double *));

#ifdef __cplusplus
}
#endif
