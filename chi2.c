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
      int32_t ii = i % NDATA;
      int32_t jj = i / NDATA;
      inputs[0] = data->x[ii];
      inputs[1] = data->y[jj];

      multil_FullEvaluate(nn, inputs);

      double fi = multil_get(0, nn) - data->data[i];
      *chi += fi * fi;
      for (size_t j = 0; j < npar; j++) {
         double temp = multil_get_grad(0, j, nn);
         grad[j] += fi * temp;
      }
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
      int32_t ii = i % NDATA;
      int32_t jj = i / NDATA;
      inputs[0] = data->x[ii];
      inputs[1] = data->y[jj];
      multil_Evaluate(nn, inputs);
      double fi = multil_get(0, nn) - data->data[i];
      res += fi * fi;
   }

   return res;
}
