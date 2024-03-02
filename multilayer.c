
#include <network.h>
#include <ran2.h>

static double loc_sigma(double x) { return tanh(x * 0.5); }
static double loc_sigma_d(double x) { return 1.0 / (1.0 + cosh(x)); }

static double loc_id(double x) { return x; }
static double loc_id_d(double x) { return 1.0; }

int32_t multil_getNpar(int32_t nL, int32_t *arch)
{
   int32_t nPar = 0;
   for (int32_t i = nL - 1; i >= 1; i--) {
      nPar += arch[i] * (arch[i - 1] + 1);
   }
   return nPar;
}

void multil_set_act(multilayer_t *net, multiL_act *activ, multiL_act *d_activ)
{
   for (int32_t i = 0; i < net->nL; i++) {
      if (activ[i] == NULL || d_activ[i] == NULL) {
         net->act_fun[i] = loc_sigma;
         net->d_act_fun[i] = loc_sigma_d;
      } else {
         net->act_fun[i] = activ[i];
         net->d_act_fun[i] = d_activ[i];
      }
   }
}

void multil_set_act_layer(int32_t layer, multilayer_t *net, multiL_act activ, multiL_act d_activ)
{
   if (activ == NULL || d_activ == NULL) {
      net->act_fun[layer] = loc_sigma;
      net->d_act_fun[layer] = loc_sigma_d;
   } else {
      net->act_fun[layer] = activ;
      net->d_act_fun[layer] = d_activ;
   }
}

multilayer_t multil_init_net(int32_t nL, int32_t *arch)
{
   int64_t idum = -2;
   multilayer_t net;
   net.nI = arch[0];
   net.nO = arch[nL - 1];
   net.nL = nL;

   // here I create a local copy of the architecture instead of copying the
   // reference
   net.arch = (int32_t *)calloc(nL, sizeof(int32_t));
   net.offsetW = (int32_t *)calloc(nL, sizeof(int32_t));
   net.offsetB = (int32_t *)calloc(nL, sizeof(int32_t));
   net.maxNH = 0;
   for (int32_t i = 0; i < nL; i++) {
      net.arch[i] = arch[i];
      if (net.arch[i] > net.maxNH) net.maxNH = arch[i];
   }

   net.offsetW[0] = 0;
   net.offsetB[0] = arch[1] * arch[0];
   for (int32_t i = 1; i < nL - 1; i++) {
      net.offsetW[i] = net.offsetB[i - 1] + arch[i];
      net.offsetB[i] = net.offsetW[i] + arch[i] * arch[i + 1];
   }

   net.nPar = multil_getNpar(nL, arch);
   // net.res = (double *)calloc(nO, sizeof(double));
   net.par = (double *)calloc(net.nPar, sizeof(double));
   net.parGrad = (double *)calloc(net.nPar * net.nO, sizeof(double));
   for (int32_t i = 0; i < net.nPar; i++) {
      net.par[i] = 2.0 * nn_ran2(&idum) - 1.0;
   }

   // init the activation functions to default
   net.act_fun = (multiL_act *)malloc(sizeof(multiL_act) * nL);
   net.d_act_fun = (multiL_act *)malloc(sizeof(multiL_act) * nL);
   // input and output layers with the identity function
   net.act_fun[0] = loc_id;
   net.d_act_fun[0] = loc_id_d;
   for (int32_t i = 1; i < nL - 1; i++) {
      net.act_fun[i] = loc_sigma;
      net.d_act_fun[i] = loc_sigma_d;
   }
   net.act_fun[nL - 1] = loc_id;
   net.d_act_fun[nL - 1] = loc_id_d;

   net.li = (double **)malloc(sizeof(double *) * nL);
   net.s_li = (double **)malloc(sizeof(double *) * nL);
   net.sd_li = (double **)malloc(sizeof(double *) * nL);
   for (int32_t i = 0; i < nL; i++) {
      net.li[i] = (double *)malloc(arch[i] * sizeof(double));
      net.s_li[i] = (double *)malloc(arch[i] * sizeof(double));
      net.sd_li[i] = (double *)malloc(arch[i] * sizeof(double));
   }
   return net;
}

void multil_save_net(multilayer_t *net, const char fileName[])
{
   FILE *fp = fopen(fileName, "w");
   int32_t nPar = net->nPar;
   for (int32_t i = 0; i < nPar; i++) {
      fprintf(fp, "%lf\n", net->par[i]);
   }
   fclose(fp);
}

void multil_load_net(multilayer_t *net, const char fileName[])
{
   FILE *fp = fopen(fileName, "r");
   int32_t nPar = net->nPar;
   for (int32_t i = 0; i < nPar; i++) {
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

   for (int32_t i = 0; i < net->nL; i++) {
      free(net->li[i]);
      free(net->s_li[i]);
      free(net->sd_li[i]);
   }
   free(net->li);
   free(net->s_li);
   free(net->sd_li);
}

static inline double Wjk(int32_t j, int32_t k, int32_t L, multilayer_t *net) { return net->par[net->offsetW[L - 1] + k + (net->arch[L - 1]) * j]; }

static inline double Bj(int32_t j, int32_t L, multilayer_t *net) { return net->par[net->offsetB[L - 1] + j]; }

void multil_Evaluate(multilayer_t *net, double *x)
{
   for (int32_t i = 0; i < net->nI; i++) {
      net->li[0][i] = x[i];
      net->s_li[0][i] = net->act_fun[0](x[i]);
      net->sd_li[0][i] = net->d_act_fun[0](x[i]);
   }
   for (int32_t L = 1; L < net->nL; L++) {
      for (int32_t j = 0; j < net->arch[L]; j++) {
         net->li[L][j] = 0.0;
         for (int32_t k = 0; k < net->arch[L - 1]; k++) {
            net->li[L][j] += net->s_li[L - 1][k] * Wjk(j, k, L, net);
         }
         net->li[L][j] += Bj(j, L, net);
         net->s_li[L][j] = net->act_fun[L](net->li[L][j]);
         net->sd_li[L][j] = net->d_act_fun[L](net->li[L][j]);
      }
   }
}
void multil_EvaluateParGradient(multilayer_t *net, double *x)
{
   int32_t count = 0;
   double sigma_vec[net->maxNH];
   double sigmaTemp_vec[net->maxNH];
   for (int32_t i = 0; i < net->nO; i++) {
      memset(sigma_vec, 0, sizeof(double) * net->maxNH);
      // for (int32_t f = 0; f < net->maxNH; f++) {
      //    sigma_vec[f] = 0.0;
      // }
      sigma_vec[i] = 1.0;

      for (int32_t L = net->nL - 1; L > 0; L--) {

         for (int32_t j = 0; j < net->arch[L]; j++) {
            int32_t index = net->offsetB[L - 1] + i * (net->nPar) + j;
            net->parGrad[index] = sigma_vec[j] * net->sd_li[L][j];

            // net->parGrad[index] = net->parGrad[index] > 0 ? 1.0 : 1.0;
            // count++;
            for (int32_t k = 0; k < net->arch[L - 1]; k++) {
               index = net->offsetW[L - 1] + i * (net->nPar) + k + (net->arch[L - 1]) * j;
               net->parGrad[index] = sigma_vec[j] * net->sd_li[L][j] * net->s_li[L - 1][k];
               // net->parGrad[index] = net->parGrad[index] > 0 ? 1.0 : 1.0;
               // count++;
            }
         }

         for (int32_t l = 0; l < net->arch[L - 1]; l++) {
            sigmaTemp_vec[l] = 0.0;
            for (int32_t j = 0; j < net->arch[L]; j++) {
               sigmaTemp_vec[l] += sigma_vec[j] * Wjk(j, l, L, net) * net->sd_li[L][j];
            }
         }
         // for (int32_t j = 0; j < net->arch[L - 1]; j++) {
         //    sigma_vec[j] = sigmaTemp_vec[j];
         // }
         memcpy(sigma_vec, sigmaTemp_vec, sizeof(double) * net->arch[L - 1]);
      }
   }
}

void multil_FullEvaluate(multilayer_t *net, double *x)
{
   multil_Evaluate(net, x);
   multil_EvaluateParGradient(net, x);
}

double multil_get_grad(int32_t out, int32_t par, multilayer_t *net) { return net->parGrad[out * net->nPar + par]; }
double multil_get(int32_t out, multilayer_t *net) { return net->s_li[net->nL - 1][out]; }
