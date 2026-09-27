CC ?= cc
CFLAGS ?= -O3 -DNDEBUG -flto -fno-strict-aliasing -fvisibility=hidden -flto=auto
LDFLAGS ?= -O3 -flto -lm
NET_CFLAGS = -DENABLE_NET

SOURCES = sl.c sl_lang.c
OBJECTS_NET = $(SOURCES:.c=.net.o)
OBJECTS_NONET = $(SOURCES:.c=.nonet.o)

DIST_DIR ?= package
DIST_NAME ?= sl-release
DIST_FILES = README.md LICENSE.md examples

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lws2_32
	EXT = .exe
	STRIP_CMD = strip --strip-all
	ARCHIVE_CMD = cd $(DIST_DIR) && zip -r $(DIST_NAME).zip $(DIST_NAME) && rm -rf $(DIST_NAME)
else
	EXT =
	ARCHIVE_CMD = cd $(DIST_DIR) && tar -czf $(DIST_NAME).tar.gz $(DIST_NAME) && rm -rf $(DIST_NAME)
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		STRIP_CMD = strip -x
	else
		STRIP_CMD = strip
	endif
endif

.PHONY: all nonetwork musl musl-nonetwork test distribute clean valgrind

all: sl$(EXT)

nonetwork: sl-nonetwork$(EXT)

musl: CC = musl-gcc
musl: LDFLAGS += -static
musl: sl$(EXT)

musl-nonetwork: CC = musl-gcc
musl-nonetwork: LDFLAGS += -static
musl-nonetwork: sl-nonetwork$(EXT)

sl$(EXT): $(OBJECTS_NET)
	$(CC) -o $@ $(OBJECTS_NET) $(LDFLAGS)

sl-nonetwork$(EXT): $(OBJECTS_NONET)
	$(CC) -o $@ $(OBJECTS_NONET) $(LDFLAGS)

%.net.o: %.c
	$(CC) $(CFLAGS) $(NET_CFLAGS) -c $< -o $@

%.nonet.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	printf '1 + 2\n' | ./sl$(EXT) examples/basic_calculator.sl
	printf '1 + 2\n' | ./sl-nonetwork$(EXT) examples/basic_calculator.sl

distribute:
	$(STRIP_CMD) sl$(EXT) sl-nonetwork$(EXT)
	mkdir -p $(DIST_DIR)/$(DIST_NAME)
	cp -R sl$(EXT) sl-nonetwork$(EXT) $(DIST_FILES) $(DIST_DIR)/$(DIST_NAME)/
	$(ARCHIVE_CMD)

clean:
	rm -f *.o sl$(EXT) sl-nonetwork$(EXT)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl$(EXT) $(filter-out $@,$(MAKECMDGOALS))

%:
	@:
