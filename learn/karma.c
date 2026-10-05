/*

MADE BY CLAUDE

— flow around a circular cylinder (Lattice Boltzmann, D2Q9).
 *
- No code in this file says "create a vortex". Each cell only does two things:
- 1. streaming: receives particle distribution fractions flowing toward it from each neighbor
- 2. collision: relaxes its own distribution toward local equilibrium
- When the Reynolds number is large enough, the flow behind the cylinder loses symmetry and separates into
- two alternating rows of vortices (Karman vortex street), the period of which is measured by the Strouhal number
-      St = f * D / U     (experimental around Re = 100: about 0.16 - 0.19)
 *
- Each cell keeps 9 values f[i] = amount of particles moving in direction i. Index i
- matches the neighbor index in c[9]:
-      5 1 6        1 = up      2 = right   3 = down    4 = left
-      4 0 2        5..8 = diagonals, 0 = stationary
-      8 3 7
- f[i] of this cell is taken from the neighboring cell in the opposite direction (opp[i]).
 *
- All cells only write to n[0], so this is a pure cellular automaton.
 *
- Boundaries: left = inflow with fixed velocity; top/bottom = walls moving at
- flow velocity; right = outflow (copied from the cell to the left); cylinder = bounce-back.
 */

#include "hayyoth.h"
#include "hcell.h"
#include <stdio.h>

#define W 440
#define H 120
#define D 20       /* cylinder diameter (cells) */
#define CX 100     /* cylinder center x */
#define CY (H / 2 + 1) /* offset by 1 cell to break symmetry, vortex forms faster */

#define U0 0.08f   /* inflow velocity (lattice units) */
#define RE 100.0f  /* Reynolds number = U0 * D / nu */
#define NU (U0 * D / RE)
#define TAU (3.0f * NU + 0.5f) /* BGK relaxation time */

#define STEPS_PER_FRAME 16
#define CURL_SCALE 0.025f      /* vorticity corresponding to the darkest color */

typedef struct {
  float f[9];          /* particle distribution along 9 directions */
  float ux, uy;        /* velocity (for calculating vorticity and measurement) */
  float curl;          /* vorticity, for visualization only */
  unsigned char solid; /* cell belongs to the cylinder */
} Cell;

static const int ex[9] = {0, 0, 1, 0, -1, -1, 1, 1, -1};
static const int ey[9] = {0, -1, 0, 1, 0, -1, -1, 1, 1};
static const int opp[9] = {0, 3, 4, 1, 2, 7, 8, 5, 6};
static const float wt[9] = {4.f / 9,  1.f / 9,  1.f / 9,  1.f / 9, 1.f / 9,
                            1.f / 36, 1.f / 36, 1.f / 36, 1.f / 36};

static float feq(int i, float rho, float ux, float uy) {
  float eu = ex[i] * ux + ey[i] * uy;
  float uu = ux * ux + uy * uy;
  return wt[i] * rho * (1.0f + 3.0f * eu + 4.5f * eu * eu - 1.5f * uu);
}

static void setEquilibrium(Cell *c, float rho, float ux, float uy) {
  for (int i = 0; i < 9; i++)
    c->f[i] = feq(i, rho, ux, uy);
  c->ux = ux;
  c->uy = uy;
}

static void init(void *raw, size_t h, size_t w) {
  Cell *s = raw;

  for (size_t y = 0; y < h; y++)
    for (size_t x = 0; x < w; x++) {
      Cell *c = &s[y * w + x];
      int dx = (int)x - CX, dy = (int)y - CY;
      if (dx * dx + dy * dy <= (D / 2) * (D / 2)) {
        c->solid = 1;
        setEquilibrium(c, 1.0f, 0.0f, 0.0f);
      } else {
        setEquilibrium(c, 1.0f, U0, 0.0f);
      }
    }
}

static void rule(void *c[9], void *n[9]) {
  Cell *self = c[0];
  Cell *out = n[0];

  /* Left, top, bottom boundaries: free stream flow with velocity U0 */
  if (!c[4] || !c[1] || !c[3]) {
    setEquilibrium(out, 1.0f, U0, 0.0f);
    out->curl = 0;
    return;
  }

  /* Right boundary: outflow, copied from the cell to the left */
  if (!c[2]) {
    const Cell *l = c[4];
    for (int i = 0; i < 9; i++)
      out->f[i] = l->f[i];
    out->ux = l->ux;
    out->uy = l->uy;
    out->curl = 0;
    return;
  }

  /* 1. Streaming: get particles flowing toward this cell from neighbors */
  float f[9];
  for (int i = 0; i < 9; i++)
    f[i] = ((const Cell *)c[opp[i]])->f[i];

  /* Cylinder: particles hit it and bounce back */
  if (self->solid) {
    for (int i = 0; i < 9; i++)
      out->f[i] = f[opp[i]];
    out->ux = out->uy = out->curl = 0;
    return;
  }

  /* 2. Collision: relax toward local equilibrium (BGK) */
  float rho = 0, mx = 0, my = 0;
  for (int i = 0; i < 9; i++) {
    rho += f[i];
    mx += f[i] * ex[i];
    my += f[i] * ey[i];
  }
  float ux = mx / rho, uy = my / rho;

  for (int i = 0; i < 9; i++)
    out->f[i] = f[i] - (f[i] - feq(i, rho, ux, uy)) / TAU;
  out->ux = ux;
  out->uy = uy;

  /* Vorticity from velocity of the previous step, for display only */
  const Cell *r = c[2], *l = c[4], *dn = c[3], *up = c[1];
  out->curl = 0.5f * ((r->uy - l->uy) - (dn->ux - up->ux));
}

static HrdView view(const void *raw) {
  const Cell *c = raw;

  if (c->solid)
    return (HrdView){.r = 110, .g = 110, .b = 110};

  float t = c->curl / CURL_SCALE;
  if (t > 1.0f)
    t = 1.0f;
  if (t < -1.0f)
    t = -1.0f;

  if (t >= 0) /* one-way vortex: orange-red */
    return (HrdView){.r = (unsigned char)(12 + 243 * t),
                     .g = (unsigned char)(12 + 100 * t),
                     .b = 25};
  t = -t; /* opposite-way vortex: cyan */
  return (HrdView){.r = 12,
                   .g = (unsigned char)(12 + 170 * t),
                   .b = (unsigned char)(25 + 230 * t)};
}

int main(void) {
  HcellConf cells = {.width = W, .height = H, .cellSize = sizeof(Cell)};
  HrdConf render = {.fps = 60, .scale = 2, .title = "Karman vortex street"};
  if (!hSetup(cells, render))
    return 1;

  hDataInit(init);

  printf("Re = %.0f   tau = %.3f   (experimental Strouhal around 0.16 - 0.19)\n",
         RE, TAU);

  /* Measure vortex shedding frequency: probe point on the flow centerline, behind the cylinder.
- Each time the vertical velocity changes sign from negative to positive is one cycle. */
  const Cell *grid = hcellState();
  const Cell *probe = &grid[(H / 2) * W + CX + 3 * D];
  long step = 0, last = -1;
  long periods[8];
  int nPeriods = 0;
  float prev = 0;

  while (hRun()) {
    for (int k = 0; k < STEPS_PER_FRAME; k++) {
      hCell(rule);
      step++;

      float v = probe->uy;
      if (step > 6000 && prev < 0 && v >= 0) {
        if (last >= 0)
          periods[nPeriods++ % 8] = step - last;
        last = step;
      }
      prev = v;
    }
    hRender(view);

    if (step % 4000 == 0 && nPeriods > 0) {
      int cnt = nPeriods < 8 ? nPeriods : 8;
      double T = 0;
      for (int i = 0; i < cnt; i++)
        T += periods[i];
      T /= cnt;
      printf("step %6ld   vortex shedding period %.0f steps   St = %.3f\n", step, T,
             D / (T * U0));
    }
  }
  return 0;
}