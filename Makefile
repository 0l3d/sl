CC ?= cc
CFLAGS ?= -O3 -DNDEBUG -flto -fno-strict-aliasing -fvisibility=hidden -flto=auto
# CFLAGS ?= -g
LDFLAGS ?= -O3 -flto -lm
# LDFLAGS ?= -lm
NET_CFLAGS = -DENABLE_NET

DYNCALL_DIR = libs/dyncall

CFLAGS += -I$(DYNCALL_DIR)/dyncall -I$(DYNCALL_DIR)/dynload -I$(DYNCALL_DIR)/portasm

DYNCALL_C_SRCS = \
	$(DYNCALL_DIR)/dynload/dynload.c \
	$(DYNCALL_DIR)/dynload/dynload_syms.c \
	$(DYNCALL_DIR)/dyncall/dyncall_api.c \
	$(DYNCALL_DIR)/dyncall/dyncall_callvm.c \
	$(DYNCALL_DIR)/dyncall/dyncall_callvm_base.c \
	$(DYNCALL_DIR)/dyncall/dyncall_vector.c \

UNAME_M := $(shell uname -m 2>/dev/null || echo x86_64)

ifeq ($(filter %64,$(UNAME_M)),)
	ifeq ($(filter arm%,$(UNAME_M)),)
		DYNCALL_ASM_SRCS = $(DYNCALL_DIR)/dyncall/dyncall_call_x86.S
	else
		DYNCALL_ASM_SRCS = $(DYNCALL_DIR)/dyncall/dyncall_call_arm32_arm.S
	endif
else
	ifneq ($(filter %arm64 aarch64,$(UNAME_M)),)
		DYNCALL_ASM_SRCS = $(DYNCALL_DIR)/dyncall/dyncall_call_arm64.S
	else
		DYNCALL_ASM_SRCS = $(DYNCALL_DIR)/dyncall/dyncall_call_x64.S
	endif
endif

C_SOURCES = sl.c sl_lang.c $(DYNCALL_C_SRCS)
ASM_SOURCES = $(DYNCALL_ASM_SRCS)

OBJECTS_NET = $(C_SOURCES:.c=.net.o) $(ASM_SOURCES:.S=.net.o)
OBJECTS_NONET = $(C_SOURCES:.c=.nonet.o) $(ASM_SOURCES:.S=.nonet.o)

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
	
	ifeq ($(UNAME_S),Linux)
		LDFLAGS += -ldl
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

%.net.o: %.S
	$(CC) $(CFLAGS) $(NET_CFLAGS) -c $< -o $@

%.nonet.o: %.S
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
	rm -f *.o libs/dyncall/*/*.o sl$(EXT) sl-nonetwork$(EXT)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl$(EXT) $(filter-out $@,$(MAKECMDGOALS))

%:
	@:
