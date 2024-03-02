#ifdef __cplusplus
extern "C" {
#endif
#include <default.h>

#ifndef NETWORK_H
#define NETWORK_H

typedef double (*multiL_act)(double);

typedef struct {
   int32_t nI, nH, nO, nL, nPar;
   // architecture, arch[0]=nInput arch[back] = nOutput
   int32_t *arch;
   int32_t *offsetW, *offsetB;
   // hidden layers (just the linear comb + the activated neuron)
   double **li;
   double **s_li, **sd_li;

   // double *res;
   double *par;
   double *parGrad;
   multiL_act *act_fun;
   multiL_act *d_act_fun;
   int32_t maxNH;

   double *input;

} multilayer_t;

// multilayer_t functions

int32_t multil_getNpar(int32_t nL, int32_t *arch);
multilayer_t multil_init_net(int32_t nL, int32_t *arch);
void multil_set_act(multilayer_t *net, multiL_act *activ, multiL_act *d_activ);
void multil_set_act_layer(int32_t layer, multilayer_t *net, multiL_act activ, multiL_act d_activ);
void multil_save_net(multilayer_t *net, const char fileName[]);
void multil_load_net(multilayer_t *net, const char fileName[]);
void multil_free_net(multilayer_t *net);

void multil_Evaluate(multilayer_t *net, double *x);
void multil_EvaluateParGradient(multilayer_t *net, double *x);
void multil_FullEvaluate(multilayer_t *net, double *x);

double multil_get_grad(int32_t out, int32_t par, multilayer_t *net);
double multil_get(int32_t out, multilayer_t *net);

#endif

#ifdef __cplusplus
}
#endif