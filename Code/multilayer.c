
#include <network.h>
#include <ran2.h>

static double loc_sigma(double x) { return fabs(x); }
static double loc_sigma_d(double x) { return x > 0 ? 1 : -1; }

static double loc_id(double x) { return x; }
static double loc_id_d(double x)
{
   (void)x;
   return 1.0;
}

static inline size_t multil_getNpar(size_t nL, size_t *arch)
{
   size_t nPar = 0;
   for (size_t i = nL - 1; i >= 1; i--) {
      nPar += arch[i] * (arch[i - 1] + 1);
   }
   return nPar;
}

#ifdef __GNUC__
#define generate_s_ld(nL, arch)                                                                                                                      \
   ({                                                                                                                                                \
      double **tmp = (double **)malloc(sizeof(double *) * (nL));                                                                                     \
      for (size_t _i = 0; _i < (nL); _i++)                                                                                                           \
         tmp[_i] = (double *)malloc((arch)[_i] * sizeof(double));                                                                                    \
      tmp;                                                                                                                                           \
   })
#else
static inline double **generate_s_ld(size_t nL, size_t arch)
{
   double **tmp = (double **)malloc(sizeof(double *) * nL);
   for (size_t i = 0; i < nL; i++)
      tmp[i] = (double *)malloc(arch[i] * sizeof(double));
   return tmp;
}
#endif

void multil_set_act(multilayer_t *net, multiL_act *activ, multiL_act *d_activ)
{
   for (size_t i = 0; i < net->nL; i++) {
      if (activ[i] == NULL || d_activ[i] == NULL) {
         net->act_fun[i] = loc_sigma;
         net->d_act_fun[i] = loc_sigma_d;
      } else {
         net->act_fun[i] = activ[i];
         net->d_act_fun[i] = d_activ[i];
      }
   }
}

void multil_set_act_layer(size_t layer, multilayer_t *net, multiL_act activ, multiL_act d_activ)
{
   if (activ == NULL || d_activ == NULL) {
      net->act_fun[layer] = loc_sigma;
      net->d_act_fun[layer] = loc_sigma_d;
   } else {
      net->act_fun[layer] = activ;
      net->d_act_fun[layer] = d_activ;
   }
}

static size_t *copy_vector(size_t *src, size_t n)
{
   size_t *tmp = (size_t *)calloc(n, sizeof(size_t));
   memcpy(tmp, src, n * sizeof(size_t));
   return tmp;
}

static double *generate_random_array(size_t n, int64_t idum)
{
   double *tmp = (double *)malloc(sizeof(double) * n);
   for (size_t i = 0; i < n; i++) {
      tmp[i] = 2.0 * nn_ran2(&idum) - 1.0;
   }
   return tmp;
}

multilayer_t multil_init_net(size_t nL, size_t *arch)
{
   size_t maxNH = 0;
   for (size_t i = 0; i < nL; i++)
      if (arch[i] > maxNH) maxNH = arch[i];

   const size_t nPar = multil_getNpar(nL, arch);

   multilayer_t net = {
       .nI = arch[0],
       .nO = arch[nL - 1],
       .nL = nL,
       .maxNH = maxNH,
       .nPar = nPar,
       .arch = copy_vector(arch, nL),
       .offsetW = (size_t *)calloc(nL - 1, sizeof(size_t)),
       .offsetB = (size_t *)calloc(nL - 1, sizeof(size_t)),
       .sigma_vec = (double *)calloc(maxNH, sizeof(double)),
       .sigmaTemp_vec = (double *)calloc(maxNH, sizeof(double)),
       .par = generate_random_array(nPar, -2L),
       .parGrad = (double *)calloc(nPar * arch[nL - 1], sizeof(double)),
       .act_fun = (multiL_act *)malloc(sizeof(multiL_act) * nL),
       .d_act_fun = (multiL_act *)malloc(sizeof(multiL_act) * nL),
       .s_li = generate_s_ld(nL, arch),
       .sd_li = generate_s_ld(nL, arch),
   };

   net.offsetW[0] = 0;
   net.offsetB[0] = arch[1] * arch[0];
   for (size_t i = 1; i < nL - 1; i++) {
      net.offsetW[i] = net.offsetB[i - 1] + arch[i];
      net.offsetB[i] = net.offsetW[i] + arch[i] * arch[i + 1];
   }

   net.act_fun[0] = loc_id;
   net.d_act_fun[0] = loc_id_d;
   for (size_t i = 1; i < nL - 1; i++) {
      net.act_fun[i] = loc_sigma;
      net.d_act_fun[i] = loc_sigma_d;
   }
   net.act_fun[nL - 1] = loc_id;
   net.d_act_fun[nL - 1] = loc_id_d;

   return net;
}

void multil_save_net(multilayer_t *net, const char fileName[])
{
   FILE *fp = fopen(fileName, "w");
   size_t nPar = net->nPar;
   for (size_t i = 0; i < nPar; i++) {
      fprintf(fp, "%lf\n", net->par[i]);
   }
   fclose(fp);
}

void multil_load_net(multilayer_t *net, const char fileName[])
{
   FILE *fp = fopen(fileName, "r");
   size_t nPar = net->nPar;
   for (size_t i = 0; i < nPar; i++) {
      (void)fscanf(fp, "%lf", (net->par + i));
   }
   fclose(fp);
}

void multil_free_net(multilayer_t *net)
{
   free(net->arch);
   free(net->offsetW);
   free(net->offsetB);
   // free(net->res);
   free(net->par);
   free(net->parGrad);
   free(net->act_fun);
   free(net->d_act_fun);

   for (size_t i = 0; i < net->nL; i++) {
      free(net->s_li[i]);
      free(net->sd_li[i]);
   }
   free(net->s_li);
   free(net->sd_li);
}

// static inline double Wjk(size_t j, size_t k, size_t L, multilayer_t *net) { return net->par[net->offsetW[L - 1] + k + (net->arch[L - 1]) * j]; }

// static inline double Bj(size_t j, size_t L, multilayer_t *net) { return net->par[net->offsetB[L - 1] + j]; }

#define Wjk(j, k, L, net) (net)->par[(net)->offsetW[(L)-1] + (k) + ((net)->arch[(L)-1]) * (j)]
#define Bj(j, L, net) (net)->par[(net)->offsetB[(L)-1] + (j)]

void multil_Evaluate(multilayer_t *net, double *x)
{
   for (size_t i = 0; i < net->nI; i++) {
      net->s_li[0][i] = net->act_fun[0](x[i]);
      net->sd_li[0][i] = net->d_act_fun[0](x[i]);
   }

   double *W, *B, *loc_s_li = net->s_li[0];
   double temp = 0;
   double *next_s_li, *next_sd_li;

   for (size_t L = 1; L < net->nL; L++) {
      next_s_li = net->s_li[L];
      next_sd_li = net->sd_li[L];
      B = net->par + (net->offsetB[L - 1]);
      for (size_t j = 0; j < net->arch[L]; j++) {
         temp = B[j];
         W = net->par + (net->offsetW[L - 1] + j * net->arch[L - 1]);

#pragma omp simd reduction(+ : temp)
         for (size_t k = 0; k < net->arch[L - 1]; k++)
            temp += W[k] * loc_s_li[k];

         next_s_li[j] = net->act_fun[L](temp);
         next_sd_li[j] = net->d_act_fun[L](temp);
      }
      loc_s_li = net->s_li[L];
   }
}

void multil_EvaluateParGradient(multilayer_t *net, double *x)
{
   // size_t count = 0;
   (void)x;
   double *sigma_vec = net->sigma_vec;
   double *sigmaTemp_vec = net->sigmaTemp_vec;
   double value_temp = 0;

   size_t *arch = net->arch;

   double *W, *loc_s_li, *loc_sd_li;

   for (size_t i = 0; i < net->nO; i++) {
      size_t out_offset = i * net->nPar;

      // rest the temp vector
      memset(sigma_vec, 0, sizeof(double) * net->maxNH);
      sigma_vec[i] = 1.0;

      for (size_t L = net->nL - 1; L > 0; L--) {
         loc_sd_li = net->sd_li[L];
         loc_s_li = net->s_li[L - 1];

         memset(sigmaTemp_vec, 0, sizeof(double) * net->maxNH);

         for (size_t j = 0; j < arch[L]; j++) {
            value_temp = sigma_vec[j] * loc_sd_li[j];
            W = net->par + (net->offsetW[L - 1] + j * net->arch[L - 1]);

            net->parGrad[net->offsetB[L - 1] + out_offset + j] = value_temp;

            size_t temp = net->offsetW[L - 1] + out_offset + (arch[L - 1]) * j;
            for (size_t k = 0; k < arch[L - 1]; k++) {
               net->parGrad[k + temp] = value_temp * loc_s_li[k];
               sigmaTemp_vec[k] += value_temp * W[k];
            }
         }

         // memcpy(sigma_vec, sigmaTemp_vec, sizeof(double) * arch[L - 1]);
         // swap the role instead of copying the memory
         double *ptr_temp = sigma_vec;
         sigma_vec = sigmaTemp_vec;
         sigmaTemp_vec = ptr_temp;
      }
   }
}

void multil_FullEvaluate(multilayer_t *net, double *x)
{
   multil_Evaluate(net, x);
   multil_EvaluateParGradient(net, x);
}
