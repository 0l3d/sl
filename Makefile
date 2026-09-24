CC = cc
# MUSL 
# CC      = musl-gcc
SOURCES = sl.c sl_lang.c
OBJECTS = $(SOURCES:.c=.o)
TARGET  = sl

# TESTED On:
# MSYS2 Clang 
# MSYS2 Gcc
# Debian Clang 
# Debian Gcc
#					 
#  DEBUG 
#					 
# CFLAGS = -g -O0 # DEBUG 
#
#  WITHOUT NETWORK STACK MUSL AND STANDARD CFLAGS
# 
# CFLAGS  := -O3 -DNDEBUG -flto -fno-strict-aliasing -fvisibility=hidden 
# 
#  WITH NETWORK STACK MUSL AND STANDARD
#
CFLAGS  := -O3 -DNDEBUG -DENABLE_NET -flto -fno-strict-aliasing -fvisibility=hidden -flto=auto
# 
# LDFLAGS
# 
LDFLAGS := -O3 -flto -lm # STANDARD
#
# LDFLAGS := -O3 -flto -lm -static # MUSL
# 


ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
endif

.PHONY: all clean valgrind

all: $(TARGET)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./sl $(filter-out $@,$(MAKECMDGOALS))

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)



