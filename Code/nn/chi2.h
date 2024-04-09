#ifdef __cplusplus
extern "C" {
#endif

#ifndef CHI2_H
#define CHI2_H

#include <default.h>
#include <useful_constants.h>
#include <minimize.h>
#include <network.h>
#include <read_config.h>
#include <adam.h>
#include <ran2.h>
#include <error_codes.h>
#include <annealing.h>

typedef struct {
   double *data, *x, *y;
   size_t n;
   multilayer_t *nn;
} local_data_t;

void chi_d_chi(double *par, size_t npar, void *extrapar, double *chi, double *grad);
double chi(double *par, size_t npar, void *extrapar);

#endif // CHI2_h

#ifdef __cplusplus
}
#endif