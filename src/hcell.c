#include "hcell.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  HcellConf conf;
  void *state;
  void *next;
} Store;

static Store store;

void *hcellState(void) { return store.state; }
void *hcellNext(void) { return store.next; }
HcellConf hcellConf(void) { return store.conf; }

static size_t getWidth(void) { return hcellConf().width; }
static size_t getHeight(void) { return hcellConf().height; }
static size_t getSize(void) { return hcellConf().size; }
static size_t getCellSize(void) { return hcellConf().cellSize; }

void hcellFree(void) {
  free(hcellState());
  free(hcellNext());
  store = (Store){0};
}

int hcellSetup(HcellConf conf) {
  store.conf = conf;
  store.conf.size = conf.width * conf.height * conf.cellSize;

  store.state = calloc(1, getSize());
  store.next = calloc(1, getSize());

  if (!hcellState() || !hcellNext()) {
    hcellFree();
    return 0;
  }

  return 1;
}

void hcellSync(void) { memcpy(hcellNext(), hcellState(), getSize()); }

static void *cell_at(void *buffer, int x, int y) {
  if (x < 0 || y < 0)
    return NULL;

  if ((size_t)x >= getWidth() || (size_t)y >= getHeight())
    return NULL;

  return (char *)buffer + (y * getWidth() + x) * getCellSize();
}

static const int dx[] = {0, 0, 1, 0, -1, -1, 1, 1, -1};
static const int dy[] = {0, -1, 0, 1, 0, -1, -1, 1, 1};
/*
c[5]  c[1]  c[6]
c[4]  c[0]  c[2]
c[8]  c[3]  c[7]
*/
void hcellRun(hcellRule rule) {
  if (!rule)
    return;
  for (size_t y = 0; y < getHeight(); y++) {
    for (size_t x = 0; x < getWidth(); x++) {
      void *c[9];
      void *n[9];

      for (size_t k = 0; k < 9; k++) {
        c[k] = cell_at(hcellState(), (int)x + dx[k], (int)y + dy[k]);
        n[k] = cell_at(hcellNext(), (int)x + dx[k], (int)y + dy[k]);
      }
      rule(c, n);
    }
  }

  memcpy(hcellState(), hcellNext(), getSize());
}
