#include "hayyoth.h"

#define ANTS 40
#define FOODS 40

typedef struct {
  unsigned char ant;
  unsigned char food;
} Cell;

static void init(void *raw, size_t h, size_t w) {
  Cell *s = raw;

  for (int i = 0; i < ANTS; i++) {
    int x = hRand(0, w - 1);
    int y = hRand(0, h - 1);
    s[y * w + x].ant = 1;
  }

  for (int i = 0; i < FOODS; i++) {
    int x = hRand(0, w - 1);
    int y = hRand(0, h - 1);
    s[y * w + x].food = 1;
  }
}

static void rule(void *c[9], void *n[9]) {
  Cell *self = c[0];

  if (!self || !self->ant)
    return;

  Cell *current = n[0];
  Cell *next = n[hRand(1, 8)];

  if (!next || next->ant)
    return;

  *current = (Cell){0};
  *next = *self;
  next->food = 0;
}

static HrdView view(const void *raw) {
  const Cell *c = raw;

  if (c->ant)
    return (HrdView){.r = 255, .g = 80, .b = 80};
  if (c->food)
    return (HrdView){.r = 255, .g = 180, .b = 0};
  return (HrdView){.r = 0, .g = 0, .b = 0};
}

int main(void) {
  HcellConf cell = {.width = 80, .height = 80, .cellSize = sizeof(Cell)};
  HrdConf render = {.fps = 15, .scale = 7};
  if (!hSetup(cell, render))
    return 1;

  hDataInit(init);

  while (hRun()) {
    hCell(rule);
    hRender(view);
  }
}