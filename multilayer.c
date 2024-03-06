
#include <network.h>
#include <ran2.h>

static double loc_sigma(double x) { return fabs(x); }
static double loc_sigma_d(double x) { return (0 < x) - (x < 0); }

static double loc_id(double x) { return x; }
static double loc_id_d(double x)
{
   (void)x;
   return 1.0;
}

size_t multil_getNpar(size_t nL, size_t *arch)
{
   size_t nPar = 0;
   for (size_t i = nL - 1; i >= 1; i--) {
      nPar += arch[i] * (arch[i - 1] + 1);
   }
   return nPar;
}

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

multilayer_t multil_init_net(size_t nL, size_t *arch)
{
   int64_t idum = -2;
   multilayer_t net;
   net.nI = arch[0];
   net.nO = arch[nL - 1];
   net.nL = nL;

   // here I create a local copy of the architecture instead of copying the
   // reference
   net.arch = (size_t *)calloc(nL, sizeof(size_t));
   net.offsetW = (size_t *)calloc(nL, sizeof(size_t));
   net.offsetB = (size_t *)calloc(nL, sizeof(size_t));
   net.maxNH = 0;
   for (size_t i = 0; i < nL; i++) {
      net.arch[i] = arch[i];
      if (net.arch[i] > net.maxNH) net.maxNH = arch[i];
   }
   net.sigma_vec = (double *)calloc(net.maxNH, sizeof(double));
   net.sigmaTemp_vec = (double *)calloc(net.maxNH, sizeof(double));

   net.offsetW[0] = 0;
   net.offsetB[0] = arch[1] * arch[0];
   for (size_t i = 1; i < nL - 1; i++) {
      net.offsetW[i] = net.offsetB[i - 1] + arch[i];
      net.offsetB[i] = net.offsetW[i] + arch[i] * arch[i + 1];
   }

   net.nPar = multil_getNpar(nL, arch);
   // net.res = (double *)calloc(nO, sizeof(double));
   net.par = (double *)calloc(net.nPar, sizeof(double));
   net.parGrad = (double *)calloc(net.nPar * net.nO, sizeof(double));
   for (size_t i = 0; i < net.nPar; i++) {
      net.par[i] = 2.0 * nn_ran2(&idum) - 1.0;
   }

   // init the activation functions to default
   net.act_fun = (multiL_act *)malloc(sizeof(multiL_act) * nL);
   net.d_act_fun = (multiL_act *)malloc(sizeof(multiL_act) * nL);
   // input and output layers with the identity function
   net.act_fun[0] = loc_id;
   net.d_act_fun[0] = loc_id_d;
   for (size_t i = 1; i < nL - 1; i++) {
      net.act_fun[i] = loc_sigma;
      net.d_act_fun[i] = loc_sigma_d;
   }
   net.act_fun[nL - 1] = loc_id;
   net.d_act_fun[nL - 1] = loc_id_d;

   net.s_li = (double **)malloc(sizeof(double *) * nL);
   net.sd_li = (double **)malloc(sizeof(double *) * nL);
   for (size_t i = 0; i < nL; i++) {
      net.s_li[i] = (double *)malloc(arch[i] * sizeof(double));
      net.sd_li[i] = (double *)malloc(arch[i] * sizeof(double));
   }
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

static inline double Wjk(size_t j, size_t k, size_t L, multilayer_t *net) { return net->par[net->offsetW[L - 1] + k + (net->arch[L - 1]) * j]; }

static inline double Bj(size_t j, size_t L, multilayer_t *net) { return net->par[net->offsetB[L - 1] + j]; }

void multil_Evaluate(multilayer_t *net, double *x)
{
   for (size_t i = 0; i < net->nI; i++) {
      net->s_li[0][i] = net->act_fun[0](x[i]);
      net->sd_li[0][i] = net->d_act_fun[0](x[i]);
   }
   double temp = 0;
   for (size_t L = 1; L < net->nL; L++) {
      for (size_t j = 0; j < net->arch[L]; j++) {
         temp = Bj(j, L, net);
         for (size_t k = 0; k < net->arch[L - 1]; k++) {
            temp += net->s_li[L - 1][k] * Wjk(j, k, L, net);
         }
         net->s_li[L][j] = net->act_fun[L](temp);
         net->sd_li[L][j] = net->d_act_fun[L](temp);
      }
   }
}

void multil_EvaluateParGradient(multilayer_t *net, double *x)
{
   // size_t count = 0;
   (void)x;
   double *sigma_vec = net->sigma_vec;
   double *sigmaTemp_vec = net->sigmaTemp_vec;

   size_t *arch = net->arch;

   for (size_t i = 0; i < net->nO; i++) {
      size_t out_offset = i * net->nPar;

      memset(sigma_vec, 0, sizeof(double) * net->maxNH);
      sigma_vec[i] = 1.0;
      for (size_t L = net->nL - 1; L > 0; L--) {
         for (size_t j = 0; j < arch[L]; j++) {
            size_t index = net->offsetB[L - 1] + out_offset + j;
            net->parGrad[index] = sigma_vec[j] * net->sd_li[L][j];

            size_t temp = net->offsetW[L - 1] + out_offset + (arch[L - 1]) * j;
            for (size_t k = 0; k < arch[L - 1]; k++)
               net->parGrad[k + temp] = sigma_vec[j] * net->sd_li[L][j] * net->s_li[L - 1][k];
         }

         for (size_t l = 0; l < arch[L - 1]; l++) {
            sigmaTemp_vec[l] = 0;
            for (size_t j = 0; j < arch[L]; j++) {
               sigmaTemp_vec[l] += sigma_vec[j] * Wjk(j, l, L, net) * net->sd_li[L][j];
            }
         }

         memcpy(sigma_vec, sigmaTemp_vec, sizeof(double) * arch[L - 1]);
      }
   }
}

void multil_FullEvaluate(multilayer_t *net, double *x)
{
   multil_Evaluate(net, x);
   multil_EvaluateParGradient(net, x);
}
