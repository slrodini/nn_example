#include <minimize.h>

static inline double run_adam(double *par, int32_t nPar, void *addPar, void (*fgrad)(double[], size_t, void *, double *, double *))
{
   // Set up multiple pass of ADAM (using nn_adam2 in this case)
   // To speed up the first iterations and then refine the result
   nn_adam_parameters_t aP;
   aP.alpha = 0.1;
   aP.beta1 = 0.9;
   aP.beta2 = 0.92;
   aP.max_it = 1e+3;
   aP.df_toll = 1e-6;
   aP.change_learn = 400;
   (void)nn_adam2(par, nPar, addPar, fgrad, &aP);

   aP.alpha = 0.01;
   aP.beta1 = 0.9;
   aP.beta2 = 0.99;
   aP.max_it = 1e+4;
   aP.df_toll = 1e-6;
   aP.change_learn = 1000;

   (void)nn_adam2(par, nPar, addPar, fgrad, &aP);

   aP.alpha = 0.001;
   aP.beta1 = 0.9;
   aP.beta2 = 0.999;
   aP.max_it = 1e+4;
   aP.change_learn = 5000;

   aP.df_toll = 1e-6;
   return nn_adam2(par, nPar, addPar, fgrad, &aP);
}

double minimize(double *par, int32_t nPar, void *addPar, void (*fgrad)(double[], size_t, void *, double *, double *))
{
   double adamRes = run_adam(par, nPar, addPar, fgrad);
   return adamRes;
}

double minimize2(double *par, int32_t nPar, void *addPar, double (*fnc)(double *, size_t, void *),
                 void (*fgrad)(double[], size_t, void *, double *, double *))
{
   nn_annealing_par_t ann_par = {.dt = 0.92, .nIter = 100, .Tin = 1000, .Tfin = 0.001};
   double chifoo = annealing(par, nPar, NULL, addPar, fnc, &ann_par, NN_RM_NORMAL);
   // return chifoo;
   nn_adam_parameters_t aP;

   aP = (nn_adam_parameters_t){.alpha = 0.03, .beta1 = 0.9, .beta2 = 0.95, .max_it = 2 * 1e+4, .change_learn = 2500, .df_toll = 1e-6};
   (void)nn_adam2(par, nPar, addPar, fgrad, &aP);
   aP = (nn_adam_parameters_t){.alpha = 0.001, .beta1 = 0.9, .beta2 = 0.999, .max_it = 1e+4, .change_learn = 5000, .df_toll = 1e-6};

   return nn_adam2(par, nPar, addPar, fgrad, &aP);
}
