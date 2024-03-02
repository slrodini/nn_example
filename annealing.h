#ifdef __cplusplus
extern "C" {
#endif

#ifndef ANNEALING_H
#define ANNEALING_H

#include <default.h>
#include <ran2.h>

typedef struct {
   double Tin, Tfin, dt;
   int32_t nIter;
} nn_annealing_par_t;

typedef enum { NN_RM_UNIFORM, NN_RM_NORMAL } nn_ran_mode_e;

double annealing(double *par, size_t n_par, double *parErr, void *addPar, double (*fEn)(double *, size_t, void *), nn_annealing_par_t *anPar,
                 nn_ran_mode_e ran_mode);
#endif

#ifdef __cplusplus
}
#endif
