#include <default.h>
#include <useful_constants.h>
#include <minimize.h>
#include <network.h>
#include <read_config.h>
#include <adam.h>
#include <ran2.h>
#include <error_codes.h>
#include <annealing.h>
#include <chi2.h>

typedef struct {
   int32_t n_layer;
   int32_t *nodes_per_layer;
} input_parameters_t;

void supply_default_parameters(input_parameters_t *in_par)
{
   in_par->n_layer = 3;
   in_par->nodes_per_layer = (int32_t *)calloc(in_par->n_layer, sizeof(int32_t));
   // in_par->nodes_per_layer = (int32_t[]){1, 10, 1};
   in_par->nodes_per_layer[0] = 1;
   in_par->nodes_per_layer[1] = 10;
   in_par->nodes_per_layer[2] = 1;
}

input_parameters_t *init_input_parameters(const char fn[])
{
   input_parameters_t *in_par = (input_parameters_t *)calloc(1, sizeof(input_parameters_t));
   in_par->nodes_per_layer = NULL;
   supply_default_parameters(in_par);

   nn_conf_append_option("n_layer", INTEGER);
   nn_conf_append_option("nodes_per_layer", STRING);

   void *(par_local[2]);

   par_local[0] = (void *)&in_par->n_layer;

   // All strings have a hard cutoff in the read_config file to this value. The '+1' is for the \0 character
   char NperL[BASEFOLDER_MAX_L + 1] = "";

   par_local[1] = (void *)NperL;

   nn_read_configuration_file_sstream(par_local, fn);

   if (in_par->n_layer <= 2) nn_log(NN_ERROR, "Too few layers, at least specify three = {input, hidden, output}.");

   in_par->nodes_per_layer = realloc(in_par->nodes_per_layer, in_par->n_layer * sizeof(int32_t));

   char delimiter[] = "|\n";
   char *token;

   int32_t count = 0;

   token = strtok(NperL, delimiter);
   sscanf(token, "%d", &in_par->nodes_per_layer[count]);
   count++;

   // using loop to get the rest of the token
   while (token != NULL && count < in_par->n_layer) {
      token = strtok(NULL, delimiter);
      if (token) {
         sscanf(token, "%d", &in_par->nodes_per_layer[count]);
         count++;
      }
   }
   token = strtok(NULL, delimiter);

   if (((token == NULL) && count < in_par->n_layer) || (token != NULL && count >= in_par->n_layer)) {
      nn_log(NN_WARNING, "An error occurred in reading the number of nodes per layer.");
      nn_log(NN_WARNING, "Please ensure that the number of entries is consistent with the declared n_layer. I read:");
      for (int32_t j = 0; j < count; j++)
         nn_log(NN_WARNING, "%d", in_par->nodes_per_layer[j]);
      nn_log(NN_ERROR, "Aborting...");
   }

   // clear the read_config to be ready for anything else that mught be needed
   nn_reset_read_config();

   return in_par;
}

static double sin_loc(double x) { return sin(x); }
static double cos_loc(double x) { return cos(x); }

static double dsign(double x) { return (0 < x) - (x < 0); }

static double one_o_x_loc(double x) { return 1 / (fabs(x) + 1e-10); }
static double one_o_x_d_loc(double x) { return -dsign(x) / pow(fabs(x) + 1e-10, 2); }

int main()
{
   input_parameters_t *in_par = init_input_parameters("config.in");
   nn_log(NN_INFO, "n_layer: %d", in_par->n_layer);
   for (int32_t j = 0; j < in_par->n_layer; j++)
      nn_log(NN_INFO, "layer: %d\t# of nodes: %d", j, in_par->nodes_per_layer[j]);

   multilayer_t net = multil_init_net(in_par->n_layer, in_par->nodes_per_layer);
   nn_log(NN_INFO, "%d", net.nPar);

   // multil_set_act_layer(4, &net, &sin_loc, &cos_loc);
   multil_set_act_layer(in_par->n_layer - 1, &net, sin_loc, cos_loc);

   size_t n_data = NDATA * NDATA;
   double f[NDATA * NDATA] = {0};
   double x[NDATA] = {0};
   double y[NDATA] = {0};

   for (size_t i = 0; i < NDATA; i++) {
      x[i] = (double)i / ((double)NDATA);
      y[i] = (double)i / ((double)NDATA);
   }
   for (size_t i = 0; i < NDATA; i++) {
      for (size_t j = 0; j < NDATA; j++) {
         f[i + j * NDATA] = sin(3 * x[i] * y[j] * 2 * M_PI);
      }
   }

   local_data_t loc_d = {.data = f, .x = x, .y = y, .nn = &net, .n = n_data};

   double **cov;

   // double chimin = leastsq(net.par, net.nPar, NDATA, (void *)&loc_d, 1, d_chi2_gsl, &cov);
   double chimin = minimize2(net.par, net.nPar, (void *)&loc_d, chi, chi_d_chi);

   FILE *fp = fopen("nn.dat", "w");
   double in[2] = {0, 0};

   for (size_t i = 0; i < NDATA; i++) {
      for (size_t j = 0; j < NDATA; j++) {
         in[0] = x[i];
         in[1] = y[j];
         multil_Evaluate(&net, in);
         fprintf(fp, "%le\t%le\t%le\t%le\n", x[i], y[j], f[i + j * NDATA], multil_get(0, &net));
      }
   }

   fclose(fp);

   return 0;
}