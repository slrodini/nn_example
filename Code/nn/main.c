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

typedef enum { ACT_DEFAULT = 0, ACT_POWER = 1, ACT_RELU = 2, ACT_COS = 3, ACT_ID = 4, ACT_ABS = 5 } activation_e;

typedef struct {
   size_t n_layer;
   size_t *nodes_per_layer;
   activation_e *act;
} input_parameters_t;

void supply_default_parameters(input_parameters_t *in_par)
{
   in_par->n_layer = 3;
   in_par->nodes_per_layer = (size_t *)calloc(in_par->n_layer, sizeof(size_t));
   in_par->act = (activation_e *)calloc(in_par->n_layer, sizeof(activation_e));

   // in_par->nodes_per_layer = (int32_t[]){1, 10, 1};
   in_par->nodes_per_layer[0] = 1;
   in_par->nodes_per_layer[1] = 10;
   in_par->nodes_per_layer[2] = 1;

   in_par->act[0] = ACT_DEFAULT;
   in_par->act[1] = ACT_DEFAULT;
   in_par->act[2] = ACT_DEFAULT;
}

static inline activation_e which_activation(char *act_buff)
{
   if (strcmp(act_buff, "ACT_DEFAULT") == 0) {
      return ACT_DEFAULT;
   } else if (strcmp(act_buff, "ACT_POWER") == 0) {
      return ACT_POWER;
   } else if (strcmp(act_buff, "ACT_RELU") == 0) {
      return ACT_RELU;
   } else if (strcmp(act_buff, "ACT_COS") == 0) {
      return ACT_COS;
   } else if (strcmp(act_buff, "ACT_ID") == 0) {
      return ACT_ID;
   } else if (strcmp(act_buff, "ACT_ABS") == 0) {
      return ACT_ABS;
   } else {
      nn_log(NN_WARNING, "Invalid activation function specification. Setting it to default.");
      return ACT_DEFAULT;
   }
}

input_parameters_t *init_input_parameters(const char fn[])
{
   input_parameters_t *in_par = (input_parameters_t *)calloc(1, sizeof(input_parameters_t));
   in_par->nodes_per_layer = NULL;
   supply_default_parameters(in_par);

   nn_conf_append_option("n_layer", U_LONG);
   nn_conf_append_option("nodes_per_layer", STRING);
   nn_conf_append_option("activation", STRING);

   void *(par_local[3]);

   par_local[0] = (void *)&in_par->n_layer;

   // All strings have a hard cutoff in the read_config file to this value. The '+1' is for the \0 character
   char NperL[BASEFOLDER_MAX_L + 1] = "";
   char act_buff[BASEFOLDER_MAX_L + 1] = "";

   par_local[1] = (void *)NperL;
   par_local[2] = (void *)act_buff;

   nn_read_configuration_file_sstream(par_local, fn);

   if (in_par->n_layer <= 2) nn_log(NN_ERROR, "Too few layers, at least specify three = {input, hidden, output}.");

   in_par->nodes_per_layer = realloc(in_par->nodes_per_layer, in_par->n_layer * sizeof(size_t));
   in_par->act = realloc(in_par->act, in_par->n_layer * sizeof(activation_e));
   for (size_t i = 0; i < in_par->n_layer; i++) {
      in_par->act[i] = ACT_DEFAULT;
   }

   char delimiter[] = "|\n";
   char *token;

   size_t count = 0;

   token = strtok(NperL, delimiter);
   sscanf(token, "%ld", &in_par->nodes_per_layer[count]);
   count++;

   // using loop to get the rest of the token
   while (token != NULL && count < in_par->n_layer) {
      token = strtok(NULL, delimiter);
      if (token) {
         sscanf(token, "%ld", &in_par->nodes_per_layer[count]);
         count++;
      }
   }
   token = strtok(NULL, delimiter);

   if (((token == NULL) && count < in_par->n_layer) || (token != NULL && count >= in_par->n_layer)) {
      nn_log(NN_WARNING, "An error occurred in reading the number of nodes per layer.");
      nn_log(NN_WARNING, "Please ensure that the number of entries is consistent with the declared n_layer. I read:");
      for (size_t j = 0; j < count; j++)
         nn_log(NN_WARNING, "%d", in_par->nodes_per_layer[j]);
      nn_log(NN_ERROR, "Aborting...");
   }
   if (strlen(act_buff) != 0) {
      count = 0;
      token = strtok(act_buff, delimiter);
      in_par->act[count] = which_activation(token);
      count++;

      // using loop to get the rest of the token
      while (token != NULL && count < in_par->n_layer) {
         token = strtok(NULL, delimiter);
         if (token) {
            in_par->act[count] = which_activation(token);
            count++;
         }
      }
      token = strtok(NULL, delimiter);

      if (((token == NULL) && count < in_par->n_layer) || (token != NULL && count >= in_par->n_layer)) {
         nn_log(NN_WARNING, "An error occurred in reading the activation functions.");
         nn_log(NN_WARNING, "Please ensure that the number of entries is consistent with the declared n_layer. I read:");
         for (size_t j = 0; j < count; j++)
            nn_log(NN_WARNING, "%d", in_par->act[j]);
         nn_log(NN_ERROR, "Aborting...");
      }
   }
   // clear the read_config to be ready for anything else that mught be needed
   nn_reset_read_config();

   return in_par;
}

static double cos_loc(double x) { return cos(x); }
static double sin_loc(double x) { return -sin(x); }

static double power_loc(double x) { return x * x * x; }
static double power_d_loc(double x) { return 3 * x * x; }

static double loc_abs(double x) { return fabs(x); }
static double loc_abs_d(double x) { return x > 0 ? 1 : -1; }

static double relu(double x) { return x > 0 ? x : 0; }
static double relu_d(double x) { return x > 0 ? 1 : 0; }

static double dsign(double x) { return (0 < x) - (x < 0); }

static double one_o_x_loc(double x) { return 1 / (fabs(x) + 1e-10); }
static double one_o_x_d_loc(double x) { return -dsign(x) / pow(fabs(x) + 1e-10, 2); }

int main_2(void);
int main()
{
   main_2();
   fprintf(stderr, "Finished...");
   return 0;
}

// ACT_DEFAULT = 0, ACT_POWER = 1, ACT_RELU = 2, ACT_COS = 3, ACT_ID = 4

void set_activations_from_parameters(multilayer_t *net, input_parameters_t *in_par)
{

   for (size_t j = 1; j < net->nL - 1; j++) {
      switch (in_par->act[j]) {
      case ACT_DEFAULT: {
         if (j == 0 || j == net->nL - 1) {
            multil_set_act_layer(j, net, &loc_id, &loc_id_d);
         } else {
            multil_set_act_layer(j, net, &loc_sigma, &loc_sigma_d);
         }
         break;
      }
      case ACT_POWER: {
         multil_set_act_layer(j, net, &power_loc, &power_d_loc);
         break;
      }
      case ACT_RELU: {
         multil_set_act_layer(j, net, &relu, &relu_d);
         break;
      }
      case ACT_COS: {
         multil_set_act_layer(j, net, &cos_loc, &sin_loc);
         break;
      }
      case ACT_ABS: {
         multil_set_act_layer(j, net, &loc_abs, &loc_abs_d);
         break;
      }
      case ACT_ID:
      default: {
         multil_set_act_layer(j, net, &loc_id, &loc_id_d);
         break;
      }
      }
   }
   return;
}

int main_2(void)
{
   input_parameters_t *in_par = init_input_parameters("config.in");

   multilayer_t net = multil_init_net(in_par->n_layer, in_par->nodes_per_layer);

   set_activations_from_parameters(&net, in_par);

   nn_log(NN_INFO, "n_layer: %d", net.nL);
   for (size_t j = 0; j < net.nL; j++)
      nn_log(NN_INFO, "layer: %d\t# of nodes: %d", j, net.arch[j]);

   nn_log(NN_INFO, "%d", net.nPar);

   nn_log(NN_INFO, "Continue? (y)/n");
   char dummy[255] = "";
   int foo = fscanf(stdin, "%s", dummy);
   nn_log(NN_INFO, "%s %s", dummy,  "y");
   (void)foo;
   if (strcmp(dummy, "y") != 0 && strcmp(dummy, "Y") != 0 && strcmp(dummy, "yes") != 0 && strcmp(dummy, "Yes") != 0) {
      nn_log(NN_WARNING, "aborting ...");
      exit(0);
   }

   FILE *fp2 = fopen("../lecture_1/data_heat_eq.dat", "r");
   if (!fp2) exit(1);
   char buffer[1024] = "";
   size_t n_data = 0;

   while (fgets(buffer, 1024, fp2)) {
      n_data++;
   }
   rewind(fp2);

   double f[n_data];
   double x[n_data];
   double y[n_data];
   size_t counter = 0;
   while (fgets(buffer, 1024, fp2)) {
      sscanf(buffer, "%le %le %le", x + counter, y + counter, f + counter);
      counter++;
   }
   //    printf("%lf\n", y[0]);
   // exit(0);
   // for (size_t i = 0; i < n_data; i++) {
   //    x[i] = (double)i / ((double)n_data);
   //    y[i] = (double)i / ((double)n_data);
   // }
   // for (size_t i = 0; i < n_data; i++) {
   //    for (size_t j = 0; j < n_data; j++) {
   //       f[i + j * n_data] = sin(3 * x[i] * y[j] * 2 * M_PI);
   //    }
   // }

   local_data_t loc_d = {.data = f, .x = x, .y = y, .nn = &net, .n = n_data};

   // double **cov;

   // double chimin = leastsq(net.par, net.nPar, NDATA, (void *)&loc_d, 1, d_chi2_gsl, &cov);
   (void)minimize2(net.par, net.nPar, (void *)&loc_d, chi, chi_d_chi);

   FILE *fp = fopen("nn.dat", "w");
   double in[2] = {0, 0};

   for (size_t i = 0; i < n_data; i++) {
      in[0] = x[i];
      in[1] = y[i];
      multil_Evaluate(&net, in);
      fprintf(fp, "%le\t%le\t%le\t%le\n", x[i], y[i], f[i], multil_get(0, &net));
   }

   fclose(fp);

   return 0;
}
