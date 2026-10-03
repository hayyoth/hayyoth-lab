CC = cc

CFLAGS = -Wall -Wextra -pedantic -O2 -fPIC

PKG_CONFIG := $(shell command -v pkg-config 2>/dev/null)

ifeq ($(PKG_CONFIG),)
$(error pkg-config not found. Please install pkg-config)
endif

RAYLIB := $(shell $(PKG_CONFIG) --exists raylib && echo yes)

ifeq ($(RAYLIB),)
$(error raylib not found by pkg-config. Please install raylib)
endif

RAYLIB_CFLAGS = $(shell $(PKG_CONFIG) --cflags raylib)
RAYLIB_LIBS   = $(shell $(PKG_CONFIG) --libs raylib)

PREFIX = /usr/local
BUILD = build

UNAME_S ?= $(shell uname -s)

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    WINDOWS = 1
else ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
    WINDOWS = 1
else ifeq ($(findstring CYGWIN,$(UNAME_S)),CYGWIN)
    WINDOWS = 1
else ifeq ($(OS),Windows_NT)
    WINDOWS = 1
endif

ifdef WINDOWS
    LIB_EXT = dll
    SHARED = -shared
    RPATH =
    LIB_RPATH =

else
    ifeq ($(UNAME_S),Darwin)
        LIB_EXT = dylib
        SHARED = -dynamiclib
        RPATH = -Wl,-rpath,@loader_path/../src
        LIB_RPATH = -Wl,-rpath,@loader_path
    else
        LIB_EXT = so
        SHARED = -shared
        RPATH = -Wl,-rpath,'$$ORIGIN/../src'
        LIB_RPATH = -Wl,-rpath,'$$ORIGIN'
    endif
endif

CELL_LIB    = $(BUILD)/src/libhcell.$(LIB_EXT)
RENDER_LIB  = $(BUILD)/src/libhrender.$(LIB_EXT)
HAYYOTH_LIB = $(BUILD)/src/libhayyoth.$(LIB_EXT)

LAB_SRCS = $(wildcard lab/*.c)
LAB_OUTS = $(patsubst lab/%.c,$(BUILD)/lab/%.out,$(LAB_SRCS))

LEARN_SRCS = $(wildcard learn/*.c)
LEARN_OUTS = $(patsubst learn/%.c,$(BUILD)/learn/%.out,$(LEARN_SRCS))

all: $(BUILD) $(CELL_LIB) $(RENDER_LIB) $(HAYYOTH_LIB) $(LAB_OUTS) $(LEARN_OUTS)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/src:
	mkdir -p $(BUILD)/src

$(BUILD)/lab:
	mkdir -p $(BUILD)/lab

$(BUILD)/learn:
	mkdir -p $(BUILD)/learn

$(BUILD)/src/hcell.o: src/hcell.c src/hcell.h | $(BUILD)/src
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(CELL_LIB): $(BUILD)/src/hcell.o
	$(CC) $(SHARED) -o $@ $<

$(BUILD)/src/hrender.o: src/hrender.c src/hrender.h | $(BUILD)/src
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -Isrc -c $< -o $@

$(RENDER_LIB): $(BUILD)/src/hrender.o
	$(CC) $(SHARED) -o $@ $< $(RAYLIB_LIBS)

$(BUILD)/src/hayyoth.o: src/hayyoth.c src/hayyoth.h | $(BUILD)/src
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(HAYYOTH_LIB): $(BUILD)/src/hayyoth.o $(CELL_LIB) $(RENDER_LIB)
	$(CC) $(SHARED) -o $@ \
		$(BUILD)/src/hayyoth.o \
		-L$(BUILD)/src \
		-lhcell \
		-lhrender \
		$(RAYLIB_LIBS) \
		$(LIB_RPATH)

$(BUILD)/lab/%.out: lab/%.c $(HAYYOTH_LIB) | $(BUILD)/lab
	$(CC) $(CFLAGS) -Isrc $< \
		-L$(BUILD)/src \
		-lhayyoth \
		-o $@ \
		$(RPATH)

$(BUILD)/learn/%.out: learn/%.c $(HAYYOTH_LIB) | $(BUILD)/learn
	$(CC) $(CFLAGS) -Isrc $< \
		-L$(BUILD)/src \
		-lhayyoth \
		-o $@ \
		$(RPATH)

test: all
	@echo "All labs built successfully in $(BUILD)/lab/!"

compile-commands:
	bear -- make clean all

run-%: $(BUILD)/lab/%.out
	./$(BUILD)/lab/$*.out

learn-%: $(BUILD)/learn/%.out
	./$(BUILD)/learn/$*.out

install: all
	mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(CELL_LIB) $(RENDER_LIB) $(HAYYOTH_LIB) $(PREFIX)/lib/
	cp src/hayyoth.h $(PREFIX)/include/

uninstall:
	rm -f $(PREFIX)/lib/libhcell.$(LIB_EXT)
	rm -f $(PREFIX)/lib/libhrender.$(LIB_EXT)
	rm -f $(PREFIX)/lib/libhayyoth.$(LIB_EXT)
	rm -f $(PREFIX)/include/hayyoth.h

clean:
	rm -rf $(BUILD)