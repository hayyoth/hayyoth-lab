#include "hrender.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

typedef struct {
  HrdConf conf;
  Texture2D texture;
  Color *pixels;
} Store;

static Store store;

static HrdConf getConf(void) { return store.conf; }
static size_t getWidth(void) { return getConf().width; }
static size_t getHeight(void) { return getConf().height; }
static size_t getCellSize(void) { return getConf().cellSize; }
static size_t isTerm(void) { return getConf().isTerm; }
static size_t getScale(void) { return getConf().scale; }
static size_t getFps(void) { return getConf().fps; }
static const char *getTitle(void) { return getConf().title; }

void hrdFree(void) {
  if (!isTerm()) {
    if (IsTextureValid(store.texture))
      UnloadTexture(store.texture);
    MemFree(store.pixels);
    CloseWindow();
  }
  store = (Store){0};
}

int hrdSetup(HrdConf c) {
  if (!c.title)
    c.title = "Hayyoth";
  if (!c.fps)
    c.fps = 30;
  if (!c.scale)
    c.scale = 5;

  store.conf = c;

  if (isTerm())
    return 1;
  else {
    InitWindow(getWidth() * getScale(), getHeight() * getScale(), getTitle());
    if (!IsWindowReady())
      return 0;

    store.pixels = MemAlloc(getWidth() * getHeight() * sizeof(Color));
    if (!store.pixels) {
      CloseWindow();
      return 0;
    }

    Image image = {.data = store.pixels,
                   .width = getWidth(),
                   .height = getHeight(),
                   .mipmaps = 1,
                   .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    SetTargetFPS(getFps());
    store.texture = LoadTextureFromImage(image);
    if (!IsTextureValid(store.texture)) {
      hrdFree();
      return 0;
    }

    return 1;
  }
}

int hrdRun(void) {
  if (isTerm())
    return 1;
  else if (WindowShouldClose())
    return 0;

  return 1;
}

static void terminal(const void *state, hrdView view) {
  const char *base = state;
  printf("\033[2J\033[H");
  printf("\n");
  for (size_t y = 0; y < getHeight(); y++) {
    for (size_t x = 0; x < getWidth(); x++) {
      size_t index = y * getWidth() + x;
      const void *cell = base + index * getCellSize();
      HrdView res = view(cell);
      if (res.s)
        printf("%s", res.s);
    }
    printf("\n");
  }

  fflush(stdout);
  if (getFps() > 0)
    sleep_ms(1000/getFps());
}

static void raylib(const void *state, hrdView view) {
  const char *base = state;

  for (size_t y = 0; y < getHeight(); y++) {
    for (size_t x = 0; x < getWidth(); x++) {
      size_t index = y * getWidth() + x;
      const void *cell = base + index * getCellSize();
      HrdView res = view(cell);
      if(!res.a) res.a = 255;
      store.pixels[index] =
          (Color){.r = res.r, .g = res.g, .b = res.b, .a = res.a};
    }
  }

  // display
  UpdateTexture(store.texture, store.pixels);
  BeginDrawing();
  ClearBackground(BLACK);
  DrawTexturePro(store.texture,
                 (Rectangle){0, 0, (float)getWidth(), (float)getHeight()},
                 (Rectangle){0, 0, (float)(getWidth() * getScale()),
                             (float)(getHeight() * getScale())},
                 (Vector2){0, 0}, 0, WHITE);
  EndDrawing();
}

void hrdRender(const void *state, hrdView view) {
  if (!view)
    return;
  if (isTerm())
    terminal(state, view);
  else
    raylib(state, view);
}