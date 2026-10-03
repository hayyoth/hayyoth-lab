#include "hayyoth.h"
#include "hcell.h"
#include "hrender.h"

#include <stdlib.h>
#include <time.h>

int hSetup(HcellConf cellConf, HrdConf rdConf) {
  if (!cellConf.width || !cellConf.height || !cellConf.cellSize)
    return 0;
  rdConf.width = cellConf.width;
  rdConf.height = cellConf.height;
  rdConf.cellSize = cellConf.cellSize;

  srand((unsigned)time(NULL));

  if (!hcellSetup(cellConf))
    return 0;

  if (!hrdSetup(rdConf)) {
    hcellFree();
    return 0;
  }

  return 1;
}

void hDataInit(hInit init) {
  if (!init) return;
  HcellConf conf = hcellConf();
  init(hcellState(), conf.height, conf.width);
  hcellSync();
}

int hRun(void) {
  if (!hrdRun()) {
    hrdFree();
    hcellFree();
    return 0;
  }
  return 1;
}

void hRender(hrdView view) {
  if(!view) return;
  hrdRender(hcellState(), view); 
}

void hCell(hcellRule rule) { 
  if(!rule) return;
  hcellRun(rule); 
}

int hRand(int min, int max) {
  if (min > max)
    return min;

  return min + rand() % (max - min + 1);
}