#include <chi2.h>

void chi_d_chi(double *par, size_t npar, void *extrapar, double *chi, double *grad)
{
   (void)par;
   local_data_t *data = (local_data_t *)extrapar;
   multilayer_t *nn = (multilayer_t *)data->nn;

   double inputs[2] = {0, 0};
   memset(grad, 0, npar * sizeof(double)); // clear grad vector

   *chi = 0;

   for (size_t i = 0; i < data->n; i++) {
      inputs[0] = data->x[i];
      inputs[1] = data->y[i];

      multil_FullEvaluate(nn, inputs);

      double fi = multil_get(0, nn) - data->data[i];
      *chi += fi * fi;
#pragma omp simd
      for (size_t j = 0; j < npar; j++)
         grad[j] += fi * multil_get_grad(0, j, nn);
   }
   for (size_t j = 0; j < npar; j++)
      grad[j] *= 2;

   return;
}

double chi(double *par, size_t npar, void *extrapar)
{
   (void)par;
   (void)npar;
   local_data_t *data = (local_data_t *)extrapar;
   multilayer_t *nn = (multilayer_t *)data->nn;

   double inputs[2] = {0, 0};

   double res = 0;

   for (size_t i = 0; i < data->n; i++) {
      inputs[0] = data->x[i];
      inputs[1] = data->y[i];
      multil_Evaluate(nn, inputs);
      double fi = multil_get(0, nn) - data->data[i];
      res += fi * fi;
   }

   return res;
}
