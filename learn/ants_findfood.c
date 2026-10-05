/*

MADE BY CLAUDE

- an ant colony, a few food samples, pheromones.
 *
- Rules:
- Ants move randomly (with heading inertia).
- Encountering food (adjacent cell or current cell) takes 1 unit, each sample
has 100 units.
- Ants carry food back to the nest, leaving pheromones behind as they walk.
- Searching ants that encounter pheromones follow them, heading away from the
nest.
 *
- How ants know the way home: the nest radiates a "home scent" that gradually
spreads across cells.
- Each cell takes the max scent of neighbors multiplied by a decay coefficient,
so the scent is highest at the nest
- and decreases with distance. Ants returning home go up the slope, ants
following trails go down the slope.
 *
- Recording convention (since rules also write into n[i] of other cells):
- Each cell only records its OWN trail and home (n[0]).
- Ant movement only records ant/carrying/dir into the destination cell.
- food is only subtracted when taken by an ant.
- The destination cell is checked on the next buffer so two ants don't squeeze
into the same cell.
 */

#include "hayyoth.h"
#include <stdio.h>

#define W 100
#define H 100

#define ANTS 80
#define FOOD_SAMPLES 10
#define FOOD_UNITS 100
#define STEPS_PER_FRAME 2

#define HOME_DECAY 0.97f /* home scent decreases per cell */
#define EVAP 0.99f       /* pheromone evaporation per step */
#define DEPOSIT 1.0f     /* amount left behind per step when carrying food */
#define TRAIL_MAX 3.0f
#define TRAIL_MIN 0.05f /* below this level ants cannot "smell" it */

#define KEEP_DIR 85 /* % keep direction when moving randomly */
#define FOLLOW 90   /* % follow trail when smelled */

typedef struct {
  unsigned char ant;      /* has ant */
  unsigned char carrying; /* ant is carrying food */
  unsigned char dir;      /* current heading 1..8 (index in c[]) */
  unsigned char nest;     /* cell belongs to nest */
  unsigned short food;    /* remaining food units */
  float home;             /* home scent */
  float trail;            /* pheromone leading to food */
} Cell;

static unsigned long delivered = 0;

static int chance(int pct) { return hRand(1, 100) <= pct; }

static int canGo(void *c[9], void *n[9], int i) {
  return c[i] && n[i] && !((Cell *)n[i])->ant;
}

static void moveTo(Cell *from, Cell *to, int dir, int carrying) {
  to->ant = 1;
  to->carrying = carrying;
  to->dir = dir;
  from->ant = 0;
  from->carrying = 0;
}

static void init(void *raw, size_t h, size_t w) {
  Cell *s = raw;
  int cx = (int)w / 2, cy = (int)h / 2;

  /* 3x3 nest in the center */
  for (int y = cy - 1; y <= cy + 1; y++)
    for (int x = cx - 1; x <= cx + 1; x++) {
      s[y * w + x].nest = 1;
      s[y * w + x].home = 1.0f;
    }

  /* food samples: at least 12 cells away from the nest */
  for (int k = 0; k < FOOD_SAMPLES;) {
    int x = hRand(1, (int)w - 2), y = hRand(1, (int)h - 2);
    int dx = x - cx, dy = y - cy;
    if (dx * dx + dy * dy < 12 * 12 || s[y * w + x].food)
      continue;
    s[y * w + x].food = FOOD_UNITS;
    k++;
  }

  /* ants spawning around the nest */
  for (int k = 0; k < ANTS;) {
    int x = cx + hRand(-5, 5), y = cy + hRand(-5, 5);
    if (s[y * w + x].ant)
      continue;
    s[y * w + x].ant = 1;
    s[y * w + x].dir = hRand(1, 8);
    k++;
  }
}

static void rule(void *c[9], void *n[9]) {
  Cell *cur = c[0];
  Cell *nxt = n[0];

  /* 1. Cell's part: pheromone evaporates, home scent spreads out */
  nxt->trail = cur->trail * EVAP;
  if (nxt->trail < 0.001f)
    nxt->trail = 0;

  if (!cur->nest) {
    float best = cur->home;
    for (int i = 1; i < 9; i++)
      if (c[i]) {
        float h = ((Cell *)c[i])->home * HOME_DECAY;
        if (h > best)
          best = h;
      }
    nxt->home = best;
  }

  if (!cur->ant)
    return;

  /* 2. Food-carrying ant: return to nest */
  if (cur->carrying) {
    for (int i = 0; i < 9; i++)
      if (c[i] && ((Cell *)c[i])->nest) { /* touched nest: drop */
        nxt->carrying = 0;
        nxt->dir = hRand(1, 8);
        delivered++;
        return;
      }

    nxt->trail += DEPOSIT;
    if (nxt->trail > TRAIL_MAX)
      nxt->trail = TRAIL_MAX;

    int bi = 0;
    float bh = cur->home;
    for (int i = 1; i < 9; i++)
      if (canGo(c, n, i) && ((Cell *)c[i])->home > bh) {
        bh = ((Cell *)c[i])->home;
        bi = i;
      }

    if (!bi || chance(10)) /* occasionally veer off to avoid getting stuck */
      bi = hRand(1, 8);
    if (canGo(c, n, bi))
      moveTo(nxt, n[bi], bi, 1);
    return;
  }

  /* 3. Searching ant: take food if present at current or adjacent cell */
  for (int i = 0; i < 9; i++)
    if (c[i] && n[i] && ((Cell *)n[i])->food > 0) {
      ((Cell *)n[i])->food--;
      nxt->carrying = 1;
      return;
    }

  /* 4. Smelling pheromone, follow it, heading away from the nest */
  if (chance(FOLLOW)) {
    int bi = 0;
    float bt = TRAIL_MIN;
    for (int i = 1; i < 9; i++)
      if (canGo(c, n, i)) {
        Cell *o = c[i];
        if (o->trail > bt && o->home < cur->home) {
          bt = o->trail;
          bi = i;
        }
      }
    if (bi) {
      moveTo(nxt, n[bi], bi, 0);
      return;
    }
  }

  /* 5. Random walk with inertia */
  int d = cur->dir ? cur->dir : hRand(1, 8);
  if (!chance(KEEP_DIR) || !canGo(c, n, d))
    d = hRand(1, 8);
  if (canGo(c, n, d))
    moveTo(nxt, n[d], d, 0);
  else
    nxt->dir = d;
}

static HrdView view(const void *raw) {
  const Cell *c = raw;

  if (c->ant && c->carrying)
    return (HrdView){.r = 255, .g = 255, .b = 120};
  if (c->ant)
    return (HrdView){.r = 255, .g = 70, .b = 70};
  if (c->food) {
    unsigned char g = (unsigned char)(90 + c->food * 165 / FOOD_UNITS);
    return (HrdView){.r = 30, .g = g, .b = 60};
  }
  if (c->nest)
    return (HrdView){.r = 60, .g = 110, .b = 255};
  if (c->trail > TRAIL_MIN) {
    unsigned char v = (unsigned char)(c->trail / TRAIL_MAX * 200.0f);
    return (HrdView){.r = v / 2, .g = 0, .b = v};
  }
  return (HrdView){.r = 0, .g = 0, .b = 0};
}

int main(void) {
  HcellConf cellConf = {.width = W, .height = H, .cellSize = sizeof(Cell)};
  HrdConf rdConf = {.fps = 30, .scale = 7};
  if (!hSetup(cellConf, rdConf))
    return 1;

  hDataInit(init);

  unsigned long step = 0;
  while (hRun()) {
    for (int k = 0; k < STEPS_PER_FRAME; k++) {
      hCell(rule);
      step++;
    }
    hRender(view);

    if (step % 500 == 0)
      printf("step %lu  delivered %lu / %d\n", step, delivered,
             FOOD_SAMPLES * FOOD_UNITS);
  }
  return 0;
}