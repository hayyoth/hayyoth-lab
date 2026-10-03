#ifndef HRENDER_H
#define HRENDER_H

#include <stddef.h>

typedef struct {
  const char *title;
  size_t fps;
  size_t scale;
  int isTerm;

  size_t width;
  size_t height;
  size_t cellSize;
} HrdConf;

int hrdSetup(HrdConf);
int hrdRun(void);

typedef struct {
  unsigned char r, g, b, a;
  const char *s; // for terminal
} HrdView;
typedef HrdView (*hrdView)(const void *cell);
void hrdRender(const void *state, hrdView);

void hrdFree(void);

#endif
