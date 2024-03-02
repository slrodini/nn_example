#ifdef __cplusplus
extern "C" {
#endif

#ifndef ADAM_H
#define ADAM_H

#include <default.h>

typedef struct {
   size_t max_it, change_learn;
   double alpha, beta1, beta2, df_toll;
} nn_adam_parameters_t;

double nn_adamax(double min_var[], size_t n_par, void *extra_par, void (*fgrad)(double[], size_t, void *, double *, double *),
                 nn_adam_parameters_t *restrict adam_par);
double nn_adam2(double min_var[], size_t n_par, void *extra_par, void (*fgrad)(double[], size_t, void *, double *, double *),
                nn_adam_parameters_t *restrict adam_par);

#endif
#ifdef __cplusplus
}
#endif
