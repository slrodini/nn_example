#ifdef __cplusplus
extern "C" {
#endif

#include <adam.h>

double nn_adamax(double min_var[], size_t n_par, void *extra_par, void (*fgrad)(double[], size_t, void *, double *, double *),
                 nn_adam_parameters_t *restrict adam_par)
{

   double alpha = adam_par->alpha;
   double beta1 = adam_par->beta1;
   double beta2 = adam_par->beta2;
   size_t max_it = adam_par->max_it;
   double grad_toll = adam_par->df_toll;

   double mt[n_par], ut[n_par], gt[n_par];

   for (size_t i = 0; i < n_par; i++) {
      mt[i] = 0;
      ut[i] = 0;
      gt[i] = 0;
   }

   size_t t = 0;
   double old_chi = 0.0;
   // fgrad(min_var, n_par, extra_par, &old_chi, gt);

   double grad_norm = 0;

   while (1) {
      t++;
      if (t % adam_par->change_learn == 0) {
         alpha /= 1.5;
         beta2 = pow(beta2, 2.0 / 3.0);
      }

      fgrad(min_var, n_par, extra_par, &old_chi, gt);

      grad_norm = 0.0;
      for (size_t i = 0; i < n_par; i++) {
         grad_norm += fabsl(gt[i]);
      }

      // fprintf(stderr, "Iteration: %d\tcurrent_chi2: %.4e\tcurrent_grad: %.4e\n", (size_t)t, old_chi, grad_norm);

      double temp = alpha / (1.0 - pow(beta1, t));
      for (size_t i = 0; i < n_par; i++) {
         mt[i] = beta1 * mt[i] + (1.0 - beta1) * gt[i];
         ut[i] = (beta2 * ut[i] > fabsl(gt[i])) ? beta2 * ut[i] : fabsl(gt[i]);
         double change = -temp * mt[i] / (ut[i] + EPS_ADAM);
         min_var[i] = min_var[i] + change;
      }

      if (t >= max_it || grad_norm <= grad_toll) break;
   }

   return old_chi;
}

double nn_adam2(double min_var[], size_t n_par, void *extra_par, void (*fgrad)(double[], size_t, void *, double *, double *),
                nn_adam_parameters_t *restrict adam_par)
{

   double alpha = adam_par->alpha;
   double beta1 = adam_par->beta1;
   double beta2 = adam_par->beta2;
   double beta1Bar = 1 - beta1;
   double beta2Bar = 1 - beta2;

   double beta1T = 1, beta2T = 1;
   size_t max_it = adam_par->max_it;
   // double grad_toll = adam_par->df_toll;

   double mt[n_par], vt[n_par], gt[n_par], gt2[n_par];

   memset(mt, 0, n_par * sizeof(double));
   memset(vt, 0, n_par * sizeof(double));
   memset(gt, 0, n_par * sizeof(double));
   memset(gt2, 0, n_par * sizeof(double));

   // size_t t = 0;
   double old_chi = 0.0;
   // fgrad(min_var, n_par, extra_par, &old_chi, gt);

   // double grad_norm = 0;
   size_t chunks = max_it / adam_par->change_learn;
   printf("%ld\n", chunks);
   for (size_t i_chunk = 0; i_chunk < chunks; i_chunk++) {
      for (size_t t = i_chunk + 1; t < (1 + i_chunk) * adam_par->change_learn; t++) {
         beta1T *= beta1;
         beta2T *= beta2;
         fgrad(min_var, n_par, extra_par, &old_chi, gt);

         // grad_norm = 0.0;

         // grad_norm = sqrt(grad_norm);
         fprintf(stderr, "adam2 Iter: %4ld  cur_chi2: %.4e \n", (size_t)t, old_chi);

         double temp1 = 1.0 / (1.0 - beta1T);
         double temp2 = 1.0 / (1.0 - beta2T);

         for (size_t i = 0; i < n_par; i++) {

            mt[i] = beta1 * mt[i] + beta1Bar * gt[i];
            vt[i] = beta2 * vt[i] + beta2Bar * gt[i] * gt[i];
            double mhat = mt[i] * temp1;
            double vhat = vt[i] * temp2;

            min_var[i] -= alpha * mhat / (sqrt(fabs(vhat)) + EPS_ADAM);
         }

         // if (t >= max_it || grad_norm <= grad_toll) break;
      }
      alpha /= 1.5;
      beta2 = pow(beta2, 2.0 / 3.0);
      // beta2T = pow(beta2, (1 + i_chunk) * adam_par->change_learn);
      beta2Bar = 1.0 - beta2;
   }

   return old_chi;
}

#ifdef __cplusplus
}
#endif
