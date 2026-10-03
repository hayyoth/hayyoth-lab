#include "hayyoth.h"
#include <stdio.h>

typedef float Cell;

void init(void *state, size_t height, size_t width) {
  Cell *cells = state;

  for (size_t y = 0; y < height; y++)
    for (size_t x = 0; x < width; x++)
      cells[y * width + x] = 0;

  cells[(height / 2) * width + width / 2] = 100;
}

void rule(void *c[9], void *n[9]) {
  float sum = 0;
  int count = 0;

  for (int i = 1; i < 9; i++) {
    if (c[i]) {
      sum += *(Cell *)c[i];
      count++;
    }
  }

  if (count)
    *(Cell *)n[0] = *(Cell *)c[0] + 0.25f * (sum / count - *(Cell *)c[0]);
  else
    *(Cell *)n[0] = *(Cell *)c[0];
}

HrdView view(const void *cell) {
  const Cell *c = cell;
  static char buf[16];
  snprintf(buf, sizeof(buf), "%4.0f ", *c);

  return (HrdView){.s = buf};
}

int main(void) {
  HcellConf cellConf = {.width = 20, .height = 10, .cellSize = sizeof(Cell)};
  HrdConf rdConf = {.fps = 5, .isTerm = 1};

  if (!hSetup(cellConf, rdConf))
    return 1;

  hDataInit(init);

  while (hRun()) {
    hCell(rule);
    hRender(view);
  }

  return 0;
}