CC ?= cc
CFLAGS ?= -O3 -DNDEBUG -flto -fno-strict-aliasing -fvisibility=hidden -flto=auto
LDFLAGS ?= -O3 -flto -lm
NET_CFLAGS = -DENABLE_NET

DYNCALL_DIR = libs/dyncall

CFLAGS += -I$(DYNCALL_DIR)/dyncall -I$(DYNCALL_DIR)/dynload -I$(DYNCALL_DIR)/portasm -I$(DYNCALL_DIR)/dyncallback

DYNCALL_C_SRCS = \
	$(DYNCALL_DIR)/dynload/dynload.c \
	$(DYNCALL_DIR)/dynload/dynload_syms.c \
	$(DYNCALL_DIR)/dyncall/dyncall_api.c \
	$(DYNCALL_DIR)/dyncall/dyncall_callvm.c \
	$(DYNCALL_DIR)/dyncall/dyncall_callvm_base.c \
	$(DYNCALL_DIR)/dyncall/dyncall_vector.c \
	$(DYNCALL_DIR)/dyncall/dyncall_aggregate.c \
	$(DYNCALL_DIR)/dyncallback/dyncall_callback.c \
	$(DYNCALL_DIR)/dyncallback/dyncall_args.c \
	$(DYNCALL_DIR)/dyncallback/dyncall_alloc_wx.c \
	$(DYNCALL_DIR)/dyncallback/dyncall_thunk.c

ifeq ($(OS),Windows_NT)
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		UNAME_M ?= x86_64
	else ifeq ($(PROCESSOR_ARCHITECTURE),ARM64)
		UNAME_M ?= arm64
	else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		UNAME_M ?= i386
	else
		UNAME_M ?= $(shell uname -m 2>/dev/null || echo x86_64)
	endif
else
	UNAME_M ?= $(shell uname -m 2>/dev/null || echo x86_64)
endif

ifneq ($(filter x86_64 amd64 AMD64,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_x64.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_x64.S
else ifneq ($(filter i386 i686 x86,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_x86.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_x86.S
else ifneq ($(filter aarch64 arm64 ARM64,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_arm64.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_arm64.S
else ifneq ($(filter armv% arm%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_arm32_arm.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_arm32_arm_gas.S
else ifneq ($(filter riscv64,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_riscv64.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_riscv64.S
else ifneq ($(filter ppc64 ppc64le powerpc64%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_ppc64.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_ppc64.S
else ifneq ($(filter ppc ppc32 powerpc%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_ppc32.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_ppc32.S
else ifneq ($(filter mips64%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_mips_n64.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_mips_n64.S
else ifneq ($(filter mips%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_mips_o32.S
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_mips_o32.S
else ifneq ($(filter sparc64%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_sparc64.s
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_sparc64.s
else ifneq ($(filter sparc%,$(UNAME_M)),)
	DYNCALL_CALL_ASM = $(DYNCALL_DIR)/dyncall/dyncall_call_sparc.s
	DYNCALL_CALLBACK_ASM = $(DYNCALL_DIR)/dyncallback/dyncall_callback_sparc32.s
else
	$(error Unsupported architecture: $(UNAME_M))
endif

DYNCALL_ASM_SRCS = $(DYNCALL_CALL_ASM) $(DYNCALL_CALLBACK_ASM)

C_SOURCES = sl.c sl_lang.c $(DYNCALL_C_SRCS)
ASM_SOURCES = $(DYNCALL_ASM_SRCS)

ASM_OBJ_NET = $(patsubst %.s,%.net.o,$(patsubst %.S,%.net.o,$(ASM_SOURCES)))
ASM_OBJ_NONET = $(patsubst %.s,%.nonet.o,$(patsubst %.S,%.nonet.o,$(ASM_SOURCES)))

OBJECTS_NET = $(C_SOURCES:.c=.net.o) $(ASM_OBJ_NET)
OBJECTS_NONET = $(C_SOURCES:.c=.nonet.o) $(ASM_OBJ_NONET)

DIST_DIR ?= package
DIST_NAME ?= sl-release
DIST_FILES = README.md LICENSE.md examples docs

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

.PHONY: all nonetwork debug debug-nonetwork musl musl-nonetwork test distribute clean valgrind

all: sl$(EXT)

nonetwork: sl-nonetwork$(EXT)

debug: CFLAGS := $(filter-out -O3 -DNDEBUG -flto -flto=auto,$(CFLAGS)) -g -O0
debug: LDFLAGS := $(filter-out -O3 -flto,$(LDFLAGS))
debug: sl$(EXT)

debug-nonetwork: CFLAGS := $(filter-out -O3 -DNDEBUG -flto -flto=auto,$(CFLAGS)) -g -O0
debug-nonetwork: LDFLAGS := $(filter-out -O3 -flto,$(LDFLAGS))
debug-nonetwork: sl-nonetwork$(EXT)

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

$(patsubst %.S,%.net.o,$(filter %.S,$(ASM_SOURCES))): %.net.o: %.S
	$(CC) $(CFLAGS) $(NET_CFLAGS) -c $< -o $@

$(patsubst %.S,%.nonet.o,$(filter %.S,$(ASM_SOURCES))): %.nonet.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(patsubst %.s,%.net.o,$(filter %.s,$(ASM_SOURCES))): %.net.o: %.s
	$(CC) $(CFLAGS) $(NET_CFLAGS) -c $< -o $@

$(patsubst %.s,%.nonet.o,$(filter %.s,$(ASM_SOURCES))): %.nonet.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

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
	rm -f $(OBJECTS_NET) $(OBJECTS_NONET) sl$(EXT) sl-nonetwork$(EXT)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl$(EXT) $(filter-out $@,$(MAKECMDGOALS))

%:
	@:
