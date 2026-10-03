#include "hayyoth.h"

typedef unsigned char Cell;

void init(void *state, size_t height, size_t width) {
  Cell *cells = state;

  for (size_t y = 0; y < height; y++)
    for (size_t x = 0; x < width; x++)
      cells[y * width + x] = hRand(0, 9) == 0;
}

void rule(void *c[9], void *n[9]) {
  int alive = 0;
  for (int i = 1; i < 9; i++)
    if (c[i])
      alive += *(Cell *)c[i];

  *(Cell *)n[0] = alive == 3 || (alive == 2 && *(Cell *)c[0]);
}

HrdView view(const void *cell) {
  const Cell *c = cell;
  if (*c)
    return (HrdView){.s = "x"};

  return (HrdView){.s = " "};
}

int main(void) {
  HcellConf cellConf = {.width = 100, .height = 20, .cellSize = sizeof(Cell)};
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