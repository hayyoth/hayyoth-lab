#include "hayyoth.h"

typedef unsigned char Cell;

#define EMPTY 0
#define TREE 1
#define FIRE 2

void init(void *state, size_t h, size_t w) {
  Cell *cells = state;

  for (size_t y = 0; y < h; y++) {
    for (size_t x = 0; x < w; x++) {
      // Initially, there is a 40% probability of having a tree, otherwise empty
      // land
      cells[y * w + x] = (hRand(0, 99) < 40) ? TREE : EMPTY;
    }
  }
}

void rule(void *c[9], void *n[9]) {
  Cell *self = c[0];
  if (!self)
    return;

  Cell next = *self;

  if (*self == EMPTY) {
    // Empty land has a small probability of growing a new tree (e.g., 1%)
    if (hRand(0, 999) < 10) {
      next = TREE;
    }
  } else if (*self == TREE) {
    int neighbor_on_fire = 0;

    // Check the 8 neighboring cells to see if any are on fire
    for (int i = 1; i < 9; i++) {
      if (c[i] && *(Cell *)c[i] == FIRE) {
        neighbor_on_fire = 1;
        break;
      }
    }

    // If a neighbor is on fire OR random lightning strikes (extremely small
    // probability 0.05%) -> catch fire
    if (neighbor_on_fire || hRand(0, 19999) == 0) {
      next = FIRE;
    }
  } else if (*self == FIRE) {
    // If burning, it becomes empty land in the next turn
    next = EMPTY;
  }

  if (n[0]) {
    *(Cell *)n[0] = next;
  }
}

HrdView view(const void *raw) {
  const Cell *cell = raw;

  switch (*cell) {
  case TREE: // Green tree
    return (HrdView){.r = 34, .g = 139, .b = 34};
  case FIRE: // Burning: Bright orange/red color
    return (HrdView){.r = 255, .g = 69, .b = 0};
  default: // Empty land: Dark background
    return (HrdView){.r = 20, .g = 20, .b = 20};
  }
}

int main(void) {
  HcellConf cells = {.width = 250, .height = 250, .cellSize = sizeof(Cell)};
  HrdConf render = {.fps = 30, .scale = 2};
  if (!hSetup(cells, render))
    return 1;

  hDataInit(init);
  while (hRun()) {
    hCell(rule);
    hRender(view);
  }
  return 0;
}