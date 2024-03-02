#ifdef __cplusplus
extern "C" {
#endif

#include <annealing.h>
#include <default.h>

static inline double ran2U(int64_t *id) { return 2.0 * nn_ran2(id) - 1.0; }

double annealing(double *par, size_t n_par, double *parErr, void *addPar, double (*fEn)(double *, size_t, void *), nn_annealing_par_t *anPar,
                 nn_ran_mode_e ran_mode)
{
   int64_t idum = -2;
   double T = anPar->Tin;
   double Tfin = anPar->Tfin;
   int32_t nIter = anPar->nIter; // Iteration at fixed T
   double dt = anPar->dt;

   double best_x[n_par];
   double pErr[n_par];
   if (parErr == NULL) {
      for (int32_t i = 0; i < n_par; i++)
         pErr[i] = 1.0;
   } else {
      for (int32_t i = 0; i < n_par; i++)
         pErr[i] = parErr[i];
   }
   double E, new_E, best_E;

   E = fEn(par, n_par, addPar);

   // copy_state(par, best_x, n_par);
   memcpy(best_x, par, n_par * sizeof(double));

   best_E = E;

   int32_t count = 0;

   double ranVal[nIter], ranValVar[nIter];
   int32_t indexes[nIter];
   double (*rFunc[n_par])(int64_t *);

   int32_t save = 0;

   if (NN_RM_UNIFORM == ran_mode) {
      for (int32_t i = 0; i < n_par; i++) {
         rFunc[i] = ran2U;
      }
   } else {
      for (int32_t i = 0; i < n_par; i++) {
         rFunc[i] = nn_ran2N;
      }
   }

   while (1) {
      for (int32_t i = 0; i < nIter; i++) {
         indexes[i] = (int32_t)(n_par * nn_ran2(&idum));
         ranVal[i] = nn_ran2(&idum);
         ranValVar[i] = rFunc[indexes[i]](&idum) * pErr[indexes[i]];
      }
      printf("Iteration: %d Temperature: %.6e Energy: %.6e\n", count, T, E);

      for (int32_t i = 0; i < nIter; i++) {
         int32_t index = indexes[i];
         double u = ranValVar[i];
         par[index] = u + par[index];

         new_E = fEn(par, n_par, addPar);
         if (new_E <= best_E) {
            memcpy(best_x, par, n_par * sizeof(double));

            best_E = new_E;
         }
         if (new_E == E) {
            par[index] = par[index] - u;
         } else if (new_E < E || ranVal[i] < exp((-new_E + E) / (T))) {
            E = new_E;
         } else {
            par[index] = par[index] - u;
         }
      }
      count++;

      T = T * dt;
      if (T < Tfin) break;
   }
   // copy_state(best_x, par, n_par);
   memcpy(par, best_x, n_par * sizeof(double));

   return best_E;
}

#ifdef __cplusplus
}
#endif