#include "hayyoth.h"

// Each cell contains the concentration of 2 chemical substances U and V
typedef struct {
  float u;
  float v;
} Cell;

// Reaction-diffusion configuration parameters (creates cell division / spotted
// patterns)
const float Du = 1.0f;
const float Dv = 0.5f;
const float F = 0.0367f;
const float K = 0.0649f;

void init(void *state, size_t h, size_t w) {
  Cell *cells = state;

  // Initially, the entire grid is filled with substance U (u = 1.0), with no
  // substance V (v = 0.0)
  for (size_t y = 0; y < h; y++) {
    for (size_t x = 0; x < w; x++) {
      cells[y * w + x].u = 1.0f;
      cells[y * w + x].v = 0.0f;
    }
  }

  // Drop a few drops of substance V into the center of the grid to trigger the
  // reaction
  size_t cx = w / 2;
  size_t cy = h / 2;
  for (size_t y = cy - 10; y < cy + 10; y++) {
    for (size_t x = cx - 10; x < cx + 10; x++) {
      if (y < h && x < w && hRand(0, 9) < 6) {
        cells[y * w + x].u = 0.5f;
        cells[y * w + x].v = 0.25f;
      }
    }
  }
}

void rule(void *c[9], void *n[9]) {
  Cell *self = c[0];
  if (!self)
    return;

  // Calculate discrete Laplacian operator from 8 neighboring cells to simulate
  // diffusion Weights: orthogonal cells (top, bottom, left, right) = 0.2,
  // diagonal cells = 0.05, center = -1.0
  float lap_u = -1.0f * self->u;
  float lap_v = -1.0f * self->v;

  int ortho[4] = {1, 3, 4, 2};
  for (int i = 0; i < 4; i++) {
    int idx = ortho[i];
    if (c[idx]) {
      lap_u += 0.2f * ((Cell *)c[idx])->u;
      lap_v += 0.2f * ((Cell *)c[idx])->v;
    } else {
      lap_u += 0.2f * self->u;
      lap_v += 0.2f * self->v;
    }
  }

  int diag[4] = {5, 6, 8, 7};
  for (int i = 0; i < 4; i++) {
    int idx = diag[i];
    if (c[idx]) {
      lap_u += 0.05f * ((Cell *)c[idx])->u;
      lap_v += 0.05f * ((Cell *)c[idx])->v;
    } else {
      lap_u += 0.05f * self->u;
      lap_v += 0.05f * self->v;
    }
  }

  float u = self->u;
  float v = self->v;
  float uvv = u * v * v;

  // Gray-Scott differential equations
  float du = Du * lap_u - uvv + F * (1.0f - u);
  float dv = Dv * lap_v + uvv - (F + K) * v;

  Cell next;
  next.u = u + du;
  next.v = v + dv;

  // Clamp values within the range [0, 1]
  if (next.u < 0.0f)
    next.u = 0.0f;
  if (next.u > 1.0f)
    next.u = 1.0f;
  if (next.v < 0.0f)
    next.v = 0.0f;
  if (next.v > 1.0f)
    next.v = 1.0f;

  if (n[0]) {
    *(Cell *)n[0] = next;
  }
}

HrdView view(const void *raw) {
  const Cell *cell = raw;
  float v = cell->v;

  // Map the concentration of substance V into vibrant graphic colors
  // (from dark background to turquoise/bright white)
  unsigned char val = (unsigned char)(v * 255.0f * 3.5f);
  if (val > 255)
    val = 255;

  return (HrdView){.r = val / 5, .g = val, .b = 255, .a = 255};
}

int main(void) {
  HcellConf cells = {.width = 200, .height = 200, .cellSize = sizeof(Cell)};

  HrdConf render = {.fps = 60, .scale = 3};

  if (!hSetup(cells, render))
    return 1;

  hDataInit(init);

  while (hRun()) {
    // Run multiple simulation steps per frame to make the reaction spread
    // faster and smoother
    for (int i = 0; i < 8; i++) {
      hCell(rule);
    }
    hRender(view);
  }

  return 0;
}