#ifndef HAYYOTH_H
#define HAYYOTH_H

#include "hcell.h"
#include "hrender.h"

#include <stddef.h>

int hSetup(HcellConf, HrdConf);

typedef void (*hInit)(void *state, size_t height, size_t width);
void hDataInit(hInit init);

int hRun(void);

void hCell(hcellRule);
void hRender(hrdView);

int hRand(int, int);

#endif
