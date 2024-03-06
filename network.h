#ifdef __cplusplus
extern "C" {
#endif
#include <default.h>

#ifndef NETWORK_H
#define NETWORK_H

typedef double (*multiL_act)(double);

typedef struct {
   size_t nI, nH, nO, nL, nPar;
   // architecture, arch[0]=nInput arch[back] = nOutput
   size_t *arch;
   size_t *offsetW, *offsetB;
   // hidden layers (just the linear comb + the activated neuron)

   double **s_li, **sd_li;

   // double *res;
   double *par;
   double *parGrad;
   multiL_act *act_fun;
   multiL_act *d_act_fun;
   size_t maxNH;

   double *input;

   double *sigma_vec;
   double *sigmaTemp_vec;

} multilayer_t;

// multilayer_t functions

size_t multil_getNpar(size_t nL, size_t *arch);
multilayer_t multil_init_net(size_t nL, size_t *arch);
void multil_set_act(multilayer_t *net, multiL_act *activ, multiL_act *d_activ);
void multil_set_act_layer(size_t layer, multilayer_t *net, multiL_act activ, multiL_act d_activ);
void multil_save_net(multilayer_t *net, const char fileName[]);
void multil_load_net(multilayer_t *net, const char fileName[]);
void multil_free_net(multilayer_t *net);

void multil_Evaluate(multilayer_t *net, double *x);
void multil_EvaluateParGradient(multilayer_t *net, double *x);
void multil_FullEvaluate(multilayer_t *net, double *x);

inline double multil_get_grad(size_t out, size_t par, multilayer_t *net) { return net->parGrad[out * net->nPar + par]; }
inline double multil_get(size_t out, multilayer_t *net) { return net->s_li[net->nL - 1][out]; }

#endif

#ifdef __cplusplus
}
#endif