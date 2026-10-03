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
    return (HrdView){.r = 255, .g = 255, .b = 255, .a = 255, .s = "x"};

  return (HrdView){.r = 0, .g = 0, .b = 0, .a = 255, .s = "  "};
}

int main(void) {
  HcellConf cellConf = {.width = 300, .height = 300, .cellSize = sizeof(Cell)};
  HrdConf rdConf = {.fps = 15, .scale = 2};
  if (!hSetup(cellConf, rdConf))
    return 1;

  hDataInit(init);
  while (hRun()) {
    hCell(rule);
    hRender(view);
  }
  return 0;
}