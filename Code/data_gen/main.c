#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// u_t = u_xx - l (u^4 - v^4)

typedef struct {
   double *k1, *k2, *k3, *k4;
   double *u_xx, dx, l, h, t, *v;
   size_t n;
} rk_temp_t;

void compute_hamiltonian(double *in, double *v, double k, size_t n, double h, double dx, double *out)
{
   double dxm2 = 1.0 / (dx * dx);
   out[0] = 0.0;     //-k * (pow(in[0], 4) - pow(v[0], 4));
   out[n - 1] = 0.0; //-k * (pow(in[n - 1], 4) - pow(v[n - 1], 4));

   for (size_t i = 1; i < n - 1; i++) {
      out[i] = h * (in[i + 1] - 2 * in[i] + in[i - 1]) * dxm2; // - k * (pow(in[i], 4) - pow(v[i], 4)) * h;
   }
}
void compute_hamiltonian_2(double *in, double *in2, double *v, double k, size_t n, double h, double dx, double *out)
{
   double dxm2 = 1.0 / (dx * dx);
   out[0] = 0.0;     // -k * (pow(in[0] + in2[0], 4) - pow(v[0], 4));
   out[n - 1] = 0.0; // -k * (pow(in[n - 1] + in2[n - 1], 4) - pow(v[n - 1], 4));

   for (size_t i = 1; i < n - 1; i++) {
      out[i] =
          h * (in[i + 1] + in2[i + 1] - 2 * in[i] - 2 * in2[i] + in[i - 1] + in2[i - 1]) * dxm2; //- k * (pow(in[i] + in2[i], 4) - pow(v[i], 4)) * h;
   }
}

// k1 = h/2 H(u)
// k2 = h/2 H(u+k1)
// k3 = h H(u+k2)
// k4 = h/2 H(u+k3)
// u += 1/3 (k1 + 2k2 + k3 + k4)

void rkstep(rk_temp_t *rk, double *u)
{
   compute_hamiltonian(u, rk->v, rk->l, rk->n, rk->h * 0.5, rk->dx, rk->k1);

   compute_hamiltonian_2(u, rk->k1, rk->v, rk->l, rk->n, rk->h * 0.5, rk->dx, rk->k2);

   compute_hamiltonian_2(u, rk->k2, rk->v, rk->l, rk->n, rk->h, rk->dx, rk->k3);
   compute_hamiltonian_2(u, rk->k3, rk->v, rk->l, rk->n, rk->h * 0.5, rk->dx, rk->k4);

   for (size_t i = 0; i < rk->n; i++)
      u[i] += (rk->k1[i] + 2 * rk->k2[i] + rk->k3[i] + rk->k4[i]) / 3.0;

   // for (size_t i = 0; i < rk->n; i++)
   //    u[i] += (rk->k1[i]) * 2.0;

   rk->t += rk->h;
}

double initial(double x) { return exp(-x * x) * sin(M_PI * x / 3); }

double generic_t(double x, double t) { return exp(-x * x / (1 + 4 * t)) / sqrt(1 + 4 * t); }

int main()
{
   size_t n = 1e+3;
   rk_temp_t rk = {
       .n = n,
       .dx = 0.006,
       .h = 0.00001,
       .k1 = calloc(n, sizeof(double)),
       .k2 = calloc(n, sizeof(double)),
       .k3 = calloc(n, sizeof(double)),
       .k4 = calloc(n, sizeof(double)),
       .u_xx = calloc(n, sizeof(double)),
       .v = calloc(n, sizeof(double)),
       .l = 0,
       .t = 0,
   };

   double *u = calloc(n, sizeof(double));
   for (int32_t i = 0; i < (int32_t)n; i++) {
      double x = -3.0 + 6 * (double)i / (double)(n - 1);
      u[i] = initial(x);
   }

   for (size_t t = 0; t <= 100000; t++) {
      rkstep(&rk, u);
      if (t % 10000 == 0) {
         char buff[1024];
         sprintf(buff, "res_t_%ld.dat", t / 10000);
         FILE *fp = fopen(buff, "w");

         for (int32_t i = 0; i < (int32_t)n; i++) {
            double x = -3.0 + 6 * (double)i / (double)(n - 1);
            fprintf(fp, "%.16e\t%.16e\n", x, u[i]);
         }
         fclose(fp);
      }
   }

  

   return 0;
}