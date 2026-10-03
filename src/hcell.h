#ifndef HCELL_H
#define HCELL_H

#include <stddef.h>

typedef struct {
  size_t width;
  size_t height;
  size_t cellSize;
  size_t size;
} HcellConf;

int hcellSetup(HcellConf);

/*
c[5]  c[1]  c[6]
c[4]  c[0]  c[2]
c[8]  c[3]  c[7]
*/
typedef void (*hcellRule)(void *c[9], void *n[9]);
void hcellRun(hcellRule);

void hcellSync(void);

void hcellFree(void);

HcellConf hcellConf(void);
void *hcellState(void);
void *hcellNext(void);


#endif
